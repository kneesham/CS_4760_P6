//pageTable.h
#ifndef PAGETABLE_H
#define PAGETABLE_H

typedef struct {
// type definition for a page table entry.

    int * physicalPageNbr;      // once loaded an integer will be set using the bits.c file
    int needsLoaded;            // needs loaded will be 0 if physical page location is on disk.

} physical_t;

#endif
