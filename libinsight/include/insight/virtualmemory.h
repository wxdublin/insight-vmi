/*
 * virtualmemory.h
 *
 *  Created on: 15.06.2010
 *      Author: chrschn
 */

#ifndef VIRTUALMEMORY_H_
#define VIRTUALMEMORY_H_

// Enabling the TLB is actually slower than without due to the used QCache
//#define ENABLE_TLB 1
#undef ENABLE_TLB

#ifdef ENABLE_TLB
#include <QCache>
#endif

#include <QIODevice>
#include <QMutex>
#include <genericexception.h>
#include "memspecs.h"

/**
 * Error code returned by VirtualMemory::virtualToPhysical() in case address
 * translation failes and exceptions are disabled.
 */
static const quint64 PADDR_ERROR = 0xFFFFFFFFFFFFFFFFULL;

// Used to initialize MemSpecs::highMemory in case no "high_memory" instance
// exists in 64-bit mode
static const quint64 HIGH_MEMORY_FAILSAFE_X86_64 = 0xffffc7ffffffffffULL;

/**
 * Struct to store the page table entries in case they are requested.
 * Entries that are invalid should be set to PADDR_ERROR. Unused entries
 * are set to 0.
 */
struct PageTableEntries {
    PageTableEntries() : pgd(0), pud(0), pmd(0), pte(0) {}

    /**
     * Reset all entries to zero.
     */
    void reset();

    /**
     * Is the page referenced by the structure a large page
     * or a 4kB page?
     */
    bool isLargePage() const;

    /**
     * Is the page referenced by the structure present?
     */
    bool isPresent() const;

    /**
     * Is the page referenced by this structure executable?
     *
     * Notice that this function only can be used if the NX
     * flag is used by the OS. This is usually the case if
     * PAE or IA-32 paging is used. The function will return
     * false in case the page that this entry references is
     * not present.
     */
    bool isExecutable() const;

    /**
     * Is the page referenced by this structure a supervisor page?
     *
     * Notice that this function will return false in case the page
     * that this entry references is not present.
     */
    bool isSupervisor() const;

    /**
     * Is the page referenced by this structure a writeable?
     *
     * Notice that this function will return false in case the page
     * that this entry references is not present.
     */
    bool isWriteable() const;

    /**
     * Get the offset to the next page that can follow the page
     * referenced by this entry.
     */
    quint64 nextPageOffset(const MemSpecs &specs) const;

    quint64 pgd;
    quint64 pud;
    quint64 pmd;
    quint64 pte;
};

/**
 * This class provides read access to a virtual address space and performs
 * the virtual to physical address translation.
 */
class VirtualMemory: protected QIODevice
{
public:
    enum PageTableFlags {
        Present = 0x1,              ///< If set the page is present
        ReadWrite = 0x2,            ///< 0 = Read only, 1 = Read & Write
        Supervisor = 0x4,           ///< 0 = Supervisor, 1 = User
        NX = 0x8000000000000000     ///< 0 = Execute, 1 = Non-Executable
    };

    VirtualMemory(const MemSpecs& specs, QIODevice* physMem, int memDumpIndex);
    virtual ~VirtualMemory();

    // Re-implementations of QIODevice
    virtual bool isSequential() const;
    virtual bool open (OpenMode mode);
    virtual qint64 pos() const;
    virtual bool reset();
    virtual qint64 size() const;

    /**
     * Reads up to \a maxSize bytes starting from offset \a pos and stores the
     * data in buffer \a data. If thread-safety is enabled (see
     * setThreadSafety()), then the whole operation is performed atomically.
     * @param pos offset to read data from
     * @param data buffer to write the data to
     * @param maxlen number of bytes to read
     * @return number of bytes actually read, or -1 in case of errors
     * \sa setThreadSafety(), isThreadSafe()
     */
    qint64 readAtomic(qint64 pos, char * data, qint64 maxlen);

//    /**
//     * Configures this instance to work on the user-land part of the memory only.
//     * Reset with setKernelSpace
//     *
//     * Note: to prevent logical error, VirtualMemory either works on
//     * user-land or kernel space, it is up to the programmer to
//     * switch manually.
//     *
//     * setUserLand() locks this instance for the calling Thread. After calling
//     * setUserLand(), the caller MUST IN ANY CASE call setKernelSpace()
//     * to unlock this VirtualMemory for other Threads. A not executed
//     * setKernelSpace() is the first place to search for, if you look for
//     * deadlocks.
//     *
//     * @param pgd the Page-Global-Directory of the current user process, most likely
//     * the cr3 register content. This value cannot be provided by InSight.
//     */
//    void setUserLand(qint64 pgd);

//    /**
//     * Configures this instance to work on the kernel memory space only.
//     * This is the default behavior.
//     */
//    void setKernelSpace();

    /**
     * Seeks to the virtual memory position \a pos without throwing an exception
     * if the virtual address translation fails.
     * @param pos address to seek to
     * @return \c true if the seek succeeded, \c false otherwise
     */
    bool safeSeek(qint64 pos);

    /**
     * @return the device containing the physical memory (const version)
     */
    const QIODevice* physMem() const;

    /**struct page_table_entries *ptEntries
     * @return the device containing the physical memory
     */
    QIODevice* physMem();

    /**
     * Sets the device containing the physical memory
     * @param physMem new device containing physical memory
     */
    void setPhysMem(QIODevice* physMem);

    /**
     * @return the memory specifications of the virtual memory
     */
    const MemSpecs& memSpecs() const;

    /**
     * @return the index of the underlying memory dump within the array of
     * dumps
     */
    int memDumpIndex() const;

    /**
     * Translates a virtual kernel address to a physical address, either by
     * linear translation (* \a pageSize == -1) or by page table look-up,
     * depending on the address.
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned, or -1,
     * if the address is within linear space
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @param ptEntries if specified the page table entries that are obtained
     * during address resolution will be written to the structure.
     * Invalid/Unresolved entries will be set to PADDR_ERROR
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     * \exception VirtualMemoryException
     */
    quint64 virtualToPhysical(quint64 vaddr, int* pageSize,
            bool enableExceptions = true, struct PageTableEntries *ptEntries = 0);

    quint64 virtualToPhysicalUserLand(quint64 vaddr, int* pageSize, quint64 pgd,
                bool enableExceptions = true);

    /**
     * @return \c true if thread safety is turned on, \c false otherwise
     * \sa setThreadSafety()
     */
    bool isThreadSafe() const;

    /**
     * Activates or deactivates thread safety in multi-threaded envirmonments.
     * If active, all accesses to reentrant data structures will be guarded
     * by a QMutex, thus becoming thread-safe. Deactivate this to increase
     * performance if no multi-threading is used.
     * @param safe \c true enables thread safety, \c disables it.
     * @return \c true if thread safety was already enabled, \c false otherwise
     * \sa isThreadSafe()
     */
    bool setThreadSafety(bool safe);

#ifdef ENABLE_TLB
    /**
     * Removes all cache entries from the TLB.
     */
    void flushTlb();
#endif

    /**
     * Checks if the given address lies within an executable page. The check is
     * based on the flage of the page table entries that reference the given
     * address.
     * \note in case of a \sa VirtualMemoryExecption e.g. the address is not mapped
     * this function will return \c false.
     * @param vaddr the virtual address which should be checked for executability
     * @return \c true if the address is marked as executable, \c false otherwise
     */
    bool isExecutable(quint64 vaddr);

    /**
     * Obtain the page flags for the given virtual address.
     * @param vaddr the virtual address to get the page flags for
     * @return the consolidated page flags of the page table entries that map
     * the given virtual address
     */
    quint64 getFlags(quint64 vaddr);

protected:
    // Re-implementations of QIODevice
    virtual bool seek (qint64 pos);

    // Pure virtual functions of QIODevice
    virtual qint64 readData (char* data, qint64 maxSize);
    virtual qint64 writeData (const char* data, qint64 maxSize);

private:
#ifdef ENABLE_TLB
    /**
     * Looks up a virtual address in the translation look-aside buffer (TLB)
     * and returns the physical address. This is independent of the architecture
     * (32 or 64 bit mode).
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @return physical address, if TLB hit, \c 0 otherwise.
     */
    quint64 tlbLookup(quint64 vaddr, int* pageSize);
#endif

    /**
     * Looks up a virtual address in the x86_64 page table and returns the
     * physical address.
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @param ptEntries if the given pointer is not NULL the page table entries
     * that are obtained during the address resolution will be written to this
     * struct. Invalid/Unresolved entries will be set to PADDR_ERROR
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     */
    quint64 pageLookup64(quint64 vaddr, int* pageSize,
            bool enableExceptions, struct PageTableEntries *ptEntries);

    /**
     * Looks up a virtual address in the i386 page table and returns the
     * physical address.
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @param ptEntries if the given pointer is not NULL the page table entries
     * that are obtained during the address resolution will be written to this
     * struct. Invalid/Unresolved entries will be set to PADDR_ERROR
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     */
    quint64 pageLookup32(quint64 vaddr, int* pageSize,
            bool enableExceptions, struct PageTableEntries *ptEntries);

    /**
     * i386 specific translation
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @param ptEntries if the given pointer is not NULL the page table entries
     * that are obtained during the address resolution will be written to this
     * struct. Invalid/Unresolved entries will be set to PADDR_ERROR
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     */
    quint64 virtualToPhysical32(quint64 vaddr, int* pageSize,
            bool enableExceptions, struct PageTableEntries *ptEntries);

    /**
     * x86_64 specific translation
     * @param vaddr virtual address
     * @param pageSize here the size of the belonging page is returned
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if address translation fails, including detailed information about the
     * error, if \c false then a return value of PADDR_ERROR indicates an error
     * in the address translation
     * @param ptEntries if the given pointer is not NULL the page table entries
     * that are obtained during the address resolution will be written to this
     * struct. Invalid/Unresolved entries will be set to PADDR_ERROR
     * @return physical address in case of success, PADDR_ERROR in case of
     * address resolution failure
     */
    quint64 virtualToPhysical64(quint64 vaddr, int* pageSize,
            bool enableExceptions, struct PageTableEntries *ptEntries);

    /**
     * Reads a value of type \a T from memory and returns it.
     * @param physaddr the physical address to read from
     * @param enableExceptions if \c true, a VirtualMemoryException is thrown
     * if reading from memory fails, if \c false then no exception is thrown,
     * but \a ok is set accordingly
     * @param ok this value is set to \c true in case the operation succeeds,
     * and to \c false otherwise
     * @return the read value
     * \exception VirtualMemoryError, if value could not be read
     */
    template <class T>
    inline T extractFromPhysMem(quint64 physaddr, bool enableExceptions,
            bool* ok);

#ifdef ENABLE_TLB
    struct TLBEntry {
        TLBEntry(quint64 addr = 0, int size = 0)
            : addr(addr), size(size) {}
        quint64 addr;
        int size;
    };

    QCache<quint64, TLBEntry> _tlb;
    QMutex _tlbMutex;
#endif

    /**
     * Update the given flags using the given page table entry.
     * \note that this function currently only updates the following flags:
     * P, R/W, S/U, NX
     * @param currentFlags the currently valid flags
     * @param entry the page table entry that is used to update the flags
     * @return the flags that are valid after consideration of the given entry
     */
    quint64 updateFlags(quint64 currentFlags, quint64 entry);

    QIODevice* _physMem;
    qint64 _physMemSize;
    // This must be a reference, not an object, since MemoryDump::init() might
    // change values later on
    const MemSpecs& _specs;
    quint64 _pos;
    int _memDumpIndex;
    bool _threadSafe;
    QMutex _physMemMutex;
    QMutex _virtMemMutex;

    bool _userland; // <! switch to change change from kernelspace reading to userland reading.
    quint64 _userPGD;

    // when someone stes this to userland, we block untill it is set back to kernelMode
//    QMutex _userlandMutex;
};


inline int VirtualMemory::memDumpIndex() const
{
    return _memDumpIndex;
}


inline QIODevice* VirtualMemory::physMem()
{
    return _physMem;
}


inline const QIODevice* VirtualMemory::physMem() const
{
    return _physMem;
}


inline const MemSpecs& VirtualMemory::memSpecs() const
{
    return _specs;
}


inline bool VirtualMemory::isThreadSafe() const
{
    return _threadSafe;
}


inline bool VirtualMemory::setThreadSafety(bool safe)
{
    bool old = _threadSafe;
    _threadSafe = safe;
    return old;
}

inline qint64 VirtualMemory::size() const
{
    if (_specs.arch & MemSpecs::ar_i386)
        return VADDR_SPACE_X86;
    else if (_specs.arch & MemSpecs::ar_x86_64)
        return VADDR_SPACE_X86_64;
    // Fallback
    return 0;
}

#endif /* VIRTUALMEMORY_H_ */
