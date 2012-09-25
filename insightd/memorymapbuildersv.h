/*
 * memorymapbuildersv.h
 *
 *  Created on: 13.10.2010
 *      Author: chrschn
 */

#ifndef MEMORYMAPBUILDERSV_H_
#define MEMORYMAPBUILDERSV_H_

#include <QMutex>

#include "memorymapbuilder.h"
#include "memorymapnodesv.h"
#include "referencingtype.h"

// Forward declaration
class Instance;
class MemoryMapNode;

/**
 * Threading class for building the memory map in parallel, Sebastian's version.
 */
class MemoryMapBuilderSV: public MemoryMapBuilder
{
public:
    MemoryMapBuilderSV(MemoryMap* map, int index);
    virtual ~MemoryMapBuilderSV();

    /**
     * Calculates the probability for the given Instance \a inst and
     * \a parentProbabilty.
     * @param inst Instance to calculate probability for
     * @return calculated probability
     */
    virtual float calculateNodeProbability(const Instance *inst,
                                           float parentProbability = 0) const;

protected:
    virtual void run();

private:
    void addMembers(const Instance *inst, MemoryMapNodeSV *node);

    /**
     * Is there already a node that represents the given instance.
     * @param inst the instance to check for
     * @returns a pointer to the node if a node for this instance already exists,
     * null otherwise
     */
    MemoryMapNodeSV* existsNode(Instance &inst);

    /**
     * If we encounter a list we process the complete list before we continue.
     * This function will iterate through the given list and add all member to the
     * queue.
     * @param listHead the pointer to the list_head
     * @param firstMember the instance of the first member of the list where the
     * listHead.next points to. All members of the list will have the same type and
     * offset as the first member.
     */
    void processList(MemoryMapNodeSV *listHead, Instance &firstListMember);

    void processListHead(MemoryMapNodeSV *node, Instance *inst);
    void processCandidates(Instance *inst, const ReferencingType *ref);
    void processPointer(MemoryMapNodeSV *node, Instance *inst, const ReferencingType *ref);
    void processArray(MemoryMapNodeSV *node, Instance *inst);
    void processStruct(MemoryMapNodeSV *node, Instance *inst);
    void processUnion(MemoryMapNodeSV *node, Instance *inst);
    void processNode(MemoryMapNodeSV *node, Instance *inst = NULL,
                     const ReferencingType *ref = NULL);

    static QMutex builderMutex;
    static bool statisticsShown;
};

#endif /* MEMORYMAPBUILDERSV_H_ */
