//pageTable.h
#ifndef PAGETABLE_H
#define PAGETABLE_H

typedef struct {
// type definition for a page table entry.
    unsigned int frameIndex;
    unsigned int reference;
    unsigned int protection;
    unsigned int dirtyBit;
    unsigned int valid;

} entry_t;

#endif
