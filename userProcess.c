/*********************************
Description: 
 **********************************/

#include <stdio.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <time.h>
#include "messageType.h"
#include "sharedIds.h"
#include "constants.h"

void sigquit_handler (int sig);
void attachShmMsg();
void rcvMsg();
void sendMsg(int, const char *);
int getRandBetween(int ,int );
int getRequestType();
int getProbability();

int msqid, activeCounterId, sharedClockId;
int * sharedIds;
int * activeCounter, * sharedClock;

int main( int argc, char * argv[]) {

    int address;
   //pcb_t pcb;
   

    signal(SIGQUIT, sigquit_handler);
    // signal handler to catch sigquit sent from oss.
    
    attachShmMsg();
    // setup the message queue and the shared memory.

    sleep(1);

    if(getProbability()) {
        // address should be valid 75% of the time.

        address = getRandBetween(0, 32768);
        // will be an integer between 0 and the limit of process memory (2^15)

    } else {
        // page fault should happen
        address = 999999999;
        // guarentee a page fault will happen since this process only has 2^15 valid frame locations

    }
      
    if( getRequestType()) {  // 50/50 chance of being either a request or a write.
       // handle a read request here
        char msg[MAX_MSG];

        sprintf(msg, "Process P%d is requesting to read address %d at time %d:%d", getpid(), address, 1, 2);
        sendMsg(REQ_READ,  msg);
       
    } else {
        // handle a write here.
        char msg[MAX_MSG];
        sprintf(msg, "Process P%d is requesting write of address %d at time %d:%d", getpid(), address,sharedClock[CLOCK_SEC], sharedClock[CLOCK_NS]);
        sendMsg(REQ_WRITE,  msg);

    }


    activeCounter[0]--;
    // decrement the counter after the program terminates.

    //rcvMsg();
    // wait to receive a message before terminating.





    shmdt(activeCounter);
    shmdt(sharedClock);
    return 0;
}


int getRequestType() {
    // 50% chance of a 0 or a 1
    // 1 indicates you should send a request and 0 a write.
    return rand() & 1;
}

int getProbability() {
    // probability will either be 0 or 1.
    // The chance that a 1 will be sent is 75% and 0 is 25%
    // 1 indicates a to get a valid, and 0 indicates a an invalid address.
    srand(time(NULL) * getpid());
    return (rand() & 1) | (rand() & 1);
}

void rcvMsg() {
        struct msg_t messageData;

        msgrcv(msqid, &messageData, sizeof(messageData), GRANTED, MSG_NOERROR);
        // wait for a message to come back from a child proces.
} 

void sendMsg(int msgType, const char * message) {
    // wrapper function to send a message to oss.

    struct msg_t messageData; 

    messageData.msgType = msgType;
    // assign the type of message to be sent.

    strcpy(messageData.message, message);
    // cpy the message to the structure.
    if ( msgsnd(msqid, (void*)&messageData, MAX_MSG, 0) == -1) {
        // check that the message sent.
        perror("User_proc: sendMsg");
        if(errno == EIDRM) printf("errno = EIDRM");
        activeCounter[0]--;
        exit(0);
    }
}

void sigquit_handler (int sig){
    // handle cleanup section by removing any remaining shared memory, or message queues.
    activeCounter[0]--;
    // decrement the counter after the program terminates.

    shmdt(activeCounter);
    shmdt(sharedClock);
    exit(0);
}


void attachShmMsg() {

    key_t sharedIdKey;
    int sharedIdsId;

    sharedIdKey = ftok("userProcess.c", 'Z');
    sharedIdsId = shmget(sharedIdKey, 5* sizeof(int), IPC_CREAT | 0644);
    sharedIds = (int * ) shmat(sharedIdsId, NULL, 0);
    // setup shared memory.

    if(sharedIdKey == -1) {
        // check for all valid keys.
        perror("Error: User Process: ftok failed ");
        exit(1);
    }

    if(sharedIdsId == -1){
        // check that all ids were valid.
        perror("Error: User Process: invalid shared id ");
        exit(1);
    } 

    if  (sharedIds == NULL){
        // check for errors attaching to shared memory.
        perror("Error: User Process: not attached to valid shared memory. ");
        exit(1);
    }

    msqid = sharedIds[MSGQ_IND];
    activeCounterId = sharedIds[ACTIVE_IND];
    sharedClockId = sharedIds[CLOCK_IND];
    // set globals ids from shared ids.

    activeCounter = (int*) shmat(activeCounterId, NULL, 0);

    if(activeCounter == NULL) perror("active counter is null:");
    activeCounter[0]++;
    // increment the counter by one to show that there is a running process.

    sharedClock = (int*) shmat(sharedClockId, NULL, 0);
    // get the active counter shared memory.


}

int getRandBetween(int min, int max) {
    srand(getpid()*max);
    return (rand() % (max - min + 1)) + min;
}

