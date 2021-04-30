#ifndef MESSAGETYPE_H
#define MESSAGETYPE_H

#define  MAX_MSG 200           // max size of the message to be sent.
#define  REQ_READ  2            // process is requestion to read.
#define  REQ_WRITE  3           // Process is requesting to write.
#define  PROC_WRITING  4        // Sent if a process is to write to a frame.
#define  SWAP_FRAME  5          // type sent if a frame needs to be swapped
#define  PAGE_MOD  6            // set the dirty bit if a page is modified.
#define  PAGE_FAULT 7           // indicates a page fault
#define  GRANTED 8

// some of these macros may not be used.

typedef struct msg_t {
    long int msgType;
    char message[MAX_MSG];
} msg_t;

#endif
