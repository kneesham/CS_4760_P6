//frames.h
#ifndef FRAMES_H
#define FRAMES_H

typedef struct {
// type definition for a page table entry.
    unsigned int referenceNbr;
    unsigned int dirtyBit;          // wheather or not the page will be replaced.
    unsigned int isEmpty;           // indicates wheather a frame has a reference;
    unsigned int age;               // how long a frame has been in the main mem.

} frame_t;

#endif
