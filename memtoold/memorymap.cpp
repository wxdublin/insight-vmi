/*
 * memorymap.cpp
 *
 *  Created on: 27.08.2010
 *      Author: chrschn
 */

#include "memorymap.h"
#include <QTime>
#include <unistd.h>
#include "symfactory.h"
#include "variable.h"
#include "virtualmemory.h"
#include "virtualmemoryexception.h"
#include "array.h"
#include "shell.h"
#include "varsetter.h"
#include "memorymapbuilder.h"
#include "debug.h"




// static variable
StringSet MemoryMap::_names;


const QString& MemoryMap::insertName(const QString& name)
{
    StringSet::const_iterator it = _names.constFind(name);
    if (it == _names.constEnd())
        it = _names.insert(name);
    return *it;
}


//------------------------------------------------------------------------------


MemoryMap::MemoryMap(const SymFactory* factory, VirtualMemory* vmem)
    : _factory(factory), _vmem(vmem), _isBuilding(false),
      _shared(new BuilderSharedState)
{
	clear();
}


MemoryMap::~MemoryMap()
{
    clear();
}


void MemoryMap::clear()
{
    for (NodeList::iterator it = _roots.begin(); it != _roots.end(); ++it) {
        delete *it;
    }
    _roots.clear();

    _pointersTo.clear();
    _typeInstances.clear();
    _pmemMap.clear();
    _vmemMap.clear();
    _vmemAddresses.clear();

#ifdef DEBUG
	degPerGenerationCnt = 0;
	degForUnalignedAddrCnt = 0;
	degForUserlandAddrCnt = 0;
	degForInvalidAddrCnt = 0;
	degForNonAlignedChildAddrCnt = 0;
	degForInvalidChildAddrCnt = 0;
#endif
}


VirtualMemory* MemoryMap::vmem()
{
    return _vmem;
}


const VirtualMemory* MemoryMap::vmem() const
{
    return _vmem;
}


bool MemoryMap::fitsInVmem(quint64 addr, quint64 size) const
{
    if (_vmem->memSpecs().arch == MemSpecs::x86_64)
        return addr + size > addr;
    else
        return addr + size <= (1UL << 32);
}


void MemoryMap::build()
{
    // Set _isBuilding to true now and to false later
    VarSetter<bool> building(&_isBuilding, true, false);

    // Clean up everything
    clear();
    _shared->reset();

    QTime timer;
    timer.start();
    qint64 prev_queue_size = 0;

    // NON-PARALLEL PART OF BUILDING PROCESS

    // How many threads to create?
    if (QThread::idealThreadCount() <= 0)
        _shared->threadCount = 1;
    else if (QThread::idealThreadCount() > MAX_BUILDER_THREADS)
        _shared->threadCount = MAX_BUILDER_THREADS;
    else
        _shared->threadCount = QThread::idealThreadCount();

    debugmsg("Building reverse map with " << _shared->threadCount << " threads.");

    // Go through the global vars and add their instances to the queue
    for (VariableList::const_iterator it = _factory->vars().constBegin();
            it != _factory->vars().constEnd(); ++it)
    {
        try {
            Instance inst = (*it)->toInstance(_vmem, BaseType::trLexical);
            if (!inst.isNull() && fitsInVmem(inst.address(), inst.size())) {
                MemoryMapNode* node = new MemoryMapNode(this, inst);
                _roots.append(node);
                _vmemMap.insertMulti(node->address(), node);
                _vmemAddresses.insert(node->address());
                _shared->queue.insert(node->probability(), node);
            }
        }
        catch (GenericException e) {
            debugerr("Caught exception for variable " << (*it)->name()
                    << " at " << e.file << ":" << e.line << ":" << e.message);
        }
    }

    // PARALLEL PART OF BUILDING PROCESS

    // Enable thread safety for VirtualMemory object
    bool wasThreadSafe = _vmem->setThreadSafety(_shared->threadCount > 1);

    // Create the builder threads
    MemoryMapBuilder* threads[_shared->threadCount];
    for (int i = 0; i < _shared->threadCount; ++i) {
        threads[i] = new MemoryMapBuilder(this, i);
        threads[i]->start();
    }

    bool firstLoop = true;

    // Let the builders do the work, but regularly output some statistics
    while (!shell->interrupted() && !_shared->queue.isEmpty()) {

        if (firstLoop || timer.elapsed() > 1000) {
            firstLoop = false;
            QChar indicator = '=';
            qint64 queue_size = _shared->queue.size();
            if (prev_queue_size < queue_size)
                indicator = '+';
            else if (prev_queue_size > queue_size)
                indicator = '-';

            timer.restart();
            MemoryMapNode* node = _shared->lastNode;
            debugmsg("Processed " << _shared->processed << " instances"
                    << ", vmemAddr = " << _vmemAddresses.size()
                    << ", vmemMap = " << _vmemMap.size()
                    << ", pmemMap = " << _pmemMap.size()
                    << ", queue = " << queue_size << " " << indicator
                    << ", probability = " << (node ? node->probability() : 1.0));
            prev_queue_size = queue_size;
        }

//#ifdef DEBUG
//        // emergency stop
//        if (_shared->processed >= 50000) {
//            debugmsg(">>> Breaking revmap generation <<<");
//            break;
//        }
//#endif

        // Sleep for 100ms
        usleep(100*1000);
    }

    // Interrupt all threads, doesn't harm if they are not running anymore
    for (int i = 0; i < _shared->threadCount; ++i)
        threads[i]->interrupt();
    // Now wait for all threads and free them again
    for (int i = 0; i < _shared->threadCount; ++i) {
        if (threads[i]->isRunning())
            threads[i]->wait();
        delete threads[i];
    }

    // Restore previous value
    _vmem->setThreadSafety(wasThreadSafe);

    // Gather some statistics about the memory map
    int nonAligned = 0;
    QMap<int, PointerNodeMap::key_type> keyCnt;
    for (ULongSet::iterator it = _vmemAddresses.begin();
            it != _vmemAddresses.end(); ++it)
    {
        ULongSet::key_type addr = *it;
        if (addr % 4)
            ++nonAligned;
        int cnt = _vmemMap.count(addr);
        keyCnt.insertMulti(cnt, addr);
        while (keyCnt.size() > 100)
            keyCnt.erase(keyCnt.begin());
    }

    debugmsg("Processed " << _shared->processed << " instances");
    debugmsg(_vmemMap.size() << " nodes at "
            << _vmemAddresses.size() << " virtual addresses (" << nonAligned
            << " not aligned)");
    debugmsg(_pmemMap.size() << " nodes at " << _pmemMap.uniqueKeys().size()
            << " physical addresses");
    debugmsg(_pointersTo.size() << " pointers to "
            << _pointersTo.uniqueKeys().size() << " addresses");
    debugmsg("stack.size() = " << _shared->queue.size());

    // calculate average type size
    qint64 totalTypeSize = 0;
    qint64 totalTypeCnt = 0;
    QList<IntNodeHash::key_type> tkeys = _typeInstances.uniqueKeys();
    for (int i = 0; i < tkeys.size(); ++i) {
        int cnt = _typeInstances.count(tkeys[i]);
        totalTypeSize += cnt * _typeInstances.value(tkeys[i])->size();
        totalTypeCnt += cnt;
    }

    debugmsg("Total of " << tkeys.size() << " types found, average size: "
            << QString::number(totalTypeSize / (double) totalTypeCnt, 'f', 1)
            << " byte");

    debugmsg("degForInvalidAddrCnt         = " << degForInvalidAddrCnt);
    debugmsg("degForInvalidChildAddrCnt    = " << degForInvalidChildAddrCnt);
    debugmsg("degForNonAlignedChildAddrCnt = " << degForNonAlignedChildAddrCnt);
    debugmsg("degForUnalignedAddrCnt       = " << degForUnalignedAddrCnt);
    debugmsg("degForUserlandAddrCnt        = " << degForUserlandAddrCnt);
    debugmsg("degPerGenerationCnt          = " << degPerGenerationCnt);


/*
    QMap<int, PointerNodeMap::key_type>::iterator it;
    for (it = keyCnt.begin(); it != keyCnt.end(); ++it) {
        QString s;
        PointerNodeMap::iterator n;
        QList<PointerNodeMap::mapped_type> list = _vmemMap.values(*it);
//        QList<int> idList;
        for (int i = 0; i < list.size(); ++i) {
            int id = list[i]->type()->id();
//            idList += list[i]->type()->id();
//        }
//        qSort(idList);
//        for (int i = 0; i < idList.size(); ++i) {
//            if (i > 0)
//                s += ", ";
            s += QString("\t%1: %2\n")
                    .arg(id, 8, 16)
                    .arg(list[i]->fullName());
        }

        debugmsg(QString("List for address 0x%1 has %2 elements")
                .arg(it.value(), _vmem->memSpecs().sizeofUnsignedLong << 1, 16, QChar('0'))
                .arg(it.key(), 4));
    }
*/
}


bool MemoryMap::addressIsWellFormed(quint64 address) const
{
    // Make sure the address is within the virtual address space
    if ((_vmem->memSpecs().arch & MemSpecs::i386) &&
        address > 0xFFFFFFFFUL)
        return false;
    else {
        // Is the 64 bit address in canonical form?
        quint64 high_bits = address >> 47;
        if (high_bits != 0 && high_bits != 0x1FFFFUL)
        	return false;
    }

//    // Throw out user-land addresses
//    if (address < _vmem->memSpecs().pageOffset)
//        return false;

    return address > 0;
}


bool MemoryMap::addressIsWellFormed(const Instance& inst) const
{
    return addressIsWellFormed(inst.address()) &&
            fitsInVmem(inst.address(), inst.size());
}


bool MemoryMap::objectIsSane(const Instance& inst,
        const MemoryMapNode* parent)
{
    // How far "to the left" should we look for overlapping objects?
    static const quint64 max_obj_size = 4096;

    // We consider a difference in probability of 10% or more to be significant
    static const float prob_significance_delta = 0.1;

    // Don't add null pointers
    if (inst.isNull())
        return false;

    if (_vmemMap.isEmpty())
        return true;

    // Increase the reading counter
    _shared->vmemReadingLock.lock();
    _shared->vmemReading++;
    _shared->vmemReadingLock.unlock();

    // Check if the list contains an object at the same address with a
    // significantly higher probability
    PointerNodeMap::const_iterator it = _vmemMap.upperBound(inst.address());

    bool isSane = true;

    do {
        // Decrement iterator by at least one, until we get a valid element
        while (--it == _vmemMap.constEnd());

        PointerNodeMap::key_type otherAddr = it.key();
        MemoryMapNode* otherNode = it.value();

        // Look for overlapping objects "to the left", but not farther then
        // max_obj_size bytes
        if (otherAddr + max_obj_size < inst.address())
            break;

        // Is the the same object already contained?
        if (otherNode->address() == inst.address() &&
                otherNode->type() && inst.type() &&
                otherNode->type()->hash() == inst.type()->hash())
            isSane = false;
        // Is this an overlapping object with a significantly higher
        // probability?
        else if (otherNode->address() + otherNode->size() >= inst.address()) {
            float instProb =
                    calculateNodeProbability(&inst, parent->probability());
            if (instProb + prob_significance_delta <= otherNode->probability())
                isSane = false;
        }

    } while (isSane && it != _vmemMap.constBegin());

    // Decrease the reading counter again
    _shared->vmemReadingLock.lock();
    _shared->vmemReading--;
    _shared->vmemReadingLock.unlock();
    // Wake up any sleeping thread
    _shared->vmemReadingDone.wakeAll();

    return isSane;
}


bool MemoryMap::addChildIfNotExistend(const Instance& inst,
        MemoryMapNode* parent, int threadIndex)
{
    static const int interestingTypes =
            BaseType::trLexical |
            BaseType::rtArray |
            BaseType::rtFuncPointer |
            BaseType::rtPointer |
            BaseType::rtStruct |
            BaseType::rtUnion;

    // Dereference, if required
    const Instance i = (inst.type()->type() & BaseType::trLexical) ?
            inst.dereference(BaseType::trLexical) : inst;

    bool result = false;

    if (!i.isNull() && i.type() && (i.type()->type() & interestingTypes))
    {
        // Is any other thread currently searching for the same address?
        _shared->currAddressesLock.lock();
        int idx = 0;
        while (idx < _shared->threadCount) {
            if (_shared->currAddresses[idx] == i.address()) {
                // Another thread searches for the same address, so release the
                // currAddressesLock and wait for it to finish
                _shared->currAddressesLock.unlock();
//                _shared->threadDone[idx].wait(&_shared->perThreadLock[idx]);
                _shared->perThreadLock[idx].lock();
                _shared->perThreadLock[idx].unlock();
                _shared->currAddressesLock.lock();
                idx = 0;
            }
            else
                ++idx;
        }
        // No conflicts anymore, so we lock our current address
        _shared->currAddresses[threadIndex] = i.address();
        _shared->perThreadLock[threadIndex].lock();

        _shared->currAddressesLock.unlock();

        if (objectIsSane(i, parent)) {
            _shared->mapNodeLock.lock();
            MemoryMapNode* child = parent->addChild(i);
            _shared->mapNodeLock.unlock();

            // Acquire the writing lock
            _shared->vmemWritingLock.lock();

            // Wait until there is no more reader and we hold the read lock
            _shared->vmemReadingLock.lock();
            while (_shared->vmemReading > 0) {
                _shared->vmemReadingDone.wait(&_shared->vmemReadingLock);
                // Keep the reading lock until we have finished writing
            }

            _vmemAddresses.insert(child->address());
            _vmemMap.insertMulti(child->address(), child);

            // Release the reading and the writing lock
            _shared->vmemReadingLock.unlock();
            _shared->vmemWritingLock.unlock();

            // Insert the new node into the queue
            _shared->queueLock.lock();
            _shared->queue.insert(child->probability(), child);
            _shared->queueLock.unlock();

            result = true;
        }

        // Done, release our current address (no need to hold currAddressesLock)
        _shared->currAddresses[threadIndex] = 0;
        _shared->perThreadLock[threadIndex].unlock();
//        // Wake up a waiting thread (if it exists)
//        _shared->threadDone[threadIndex].wakeOne();

    }

    return result;
}


float MemoryMap::calculateNodeProbability(const Instance* inst,
        float parentProbability) const
{
    // Degradation of 1% per parent-child relation.
    // Starting from 1.0, this means that the 69th generation will have a
    // probability < 0.5, the 230th generation will be < 0.1.
    const float degPerGeneration = 0.99;

    // Degradation of 20% for address of this node not being aligned at 4 byte
    // boundary
    const float degForUnalignedAddr = 0.8;

    // Degradation of 5% for address begin in userland
    const float degForUserlandAddr = 0.95;

    // Degradation of 90% for an invalid address of this node
    const float degForInvalidAddr = 0.1;

    // Degradation of 5% for each non-aligned pointer the type of this node
    // has
    const float degForNonAlignedChildAddr = 0.95;

    // Degradation of 10% for each invalid pointer the type of this node has
    const float degForInvalidChildAddr = 0.9;

    float prob = parentProbability < 0 ?
                 1.0 :
                 parentProbability * degPerGeneration;

    if (parentProbability >= 0)
        degPerGenerationCnt++;

    // Check userland address
    if (inst->address() < _vmem->memSpecs().pageOffset) {
        prob *= degForUserlandAddr;
        degForUserlandAddrCnt++;
    }
    // Check validity
    if (! _vmem->safeSeek((qint64) inst->address()) ) {
        prob *= degForInvalidAddr;
        degForInvalidAddrCnt++;
    }
    // Check alignment
    else if (inst->address() & 0x3UL) {
        prob *= degForUnalignedAddr;
        degForUnalignedAddrCnt++;
    }

    // Find the BaseType of this instance, dereference any lexical type(s)
    const BaseType* instType = inst->type() ?
            inst->type()->dereferencedBaseType(BaseType::trLexical) : 0;

    // If this a union or struct, we have to consider the pointer members
    if ( instType &&
         (instType->type() & BaseType::trStructured) )
    {
        const Structured* structured =
                dynamic_cast<const Structured*>(instType);
        // Check address of all descendant pointers
        for (int i = 0; i < structured->members().size(); ++i) {
            const BaseType* m_type =
                    structured->members().at(i)->refTypeDeep(BaseType::trLexical);

            if (m_type && m_type->type() & BaseType::rtPointer) {
                try {
                    quint64 m_addr = inst->address() + structured->members().at(i)->offset();
//                    quint64 m_addr = inst->memberAddress(i);
                    // Try a safeSeek first to avoid costly throws of exceptions
                    if (_vmem->safeSeek(m_addr)) {
                        m_addr = (quint64)m_type->toPointer(_vmem, m_addr);
                        // Check validity
                        if (! _vmem->safeSeek((qint64) m_addr) ) {
                            prob *= degForInvalidChildAddr;
                            degForInvalidChildAddrCnt++;
                        }
                        // Check alignment
                        else if (m_addr & 0x3UL) {
                            prob *= degForNonAlignedChildAddr;
                            degForNonAlignedChildAddrCnt++;
                        }
                    }
                    else {
                        // Address was invalid
                        prob *= degForInvalidChildAddr;
                        degForInvalidChildAddrCnt++;
                    }
                }
                catch (MemAccessException) {
                    // Address was invalid
                    prob *= degForInvalidChildAddr;
                    degForInvalidChildAddrCnt++;
                }
                catch (VirtualMemoryException) {
                    // Address was invalid
                    prob *= degForInvalidChildAddr;
                    degForInvalidChildAddrCnt++;
                }
            }
        }
    }
    return prob;
}


const NodeList& MemoryMap::roots() const
{
    return _roots;
}


const PointerNodeMap& MemoryMap::vmemMap() const
{
    return _vmemMap;
}


const PointerIntNodeMap& MemoryMap::pmemMap() const
{
    return _pmemMap;
}


const PointerNodeHash& MemoryMap::pointersTo() const
{
    return _pointersTo;
}


bool MemoryMap::isBuilding() const
{
    return _isBuilding;
}