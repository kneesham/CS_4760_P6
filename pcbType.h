#include "pageTable.h"
#ifndef PCBTYPE_H
#define PCBTYPE_H
#define MAX_PAGES 32

typedef enum  {ready, running, done, blocked} process_state_t;
// using an enum as the state of a process.

typedef struct pcb_t {
    // definition of the pcb.

    pid_t  pid;  // actual pid of a process
    int simpid;  // simulated pid given by oss

    process_state_t state;
    // which state the process is in.

    entry_t pageTable[MAX_PAGES];
    // page table for each process

} pcb_t;

#endif
