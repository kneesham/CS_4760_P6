/************************************************
 * cs4760_project4
 * Author: Theodore Nesham
 * Date:  04/27/2021
 * Assignment: 6
 *************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>        
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <limits.h>

#include "messageType.h"     // Structure for the message queues
#include "sharedIds.h"      
#include "constants.h"
#include "pcbType.h"
#include "bits.h"
#include "pageTable.h"
#include "frames.h"

#define MAX_FRAMES 256

void printHelp();
void sendMsg(int, const char *);
void timerHandler(int);
void sigHandler(int);
void setupShmMsg();
void launchProcess(int, FILE *);
int getRandBetween(int, int);
int getUserProcessCount(int, char**);
int getIntFromString(char *,int);
int secondChance(int);

// Functions

int sharedClockId, msqid, sharedIdsId, activeId, pcbsId;
unsigned int * sharedClock;
int * activeProcesses;
int * sharedIds;

pcb_t * pcbs;                           // process control blocks.
frame_t  frames[MAX_FRAMES];            // actual frames to run second chance algorithm

int parentPid, clearedFrame;
//globals for signal functions to use.

int main(int argc, char * argv[]) {

    FILE * fp;
    struct msg_t messageData;
    int numActive, status, maxSpecifiedProcesses = getUserProcessCount(argc, argv);
    char * logfileName = "logfile";

    int  * physicalLocations[MAX_FRAMES];   // physical locations on the frames.

    int z = 0;
    for(; z< MAX_FRAMES; z++) initbv(&physicalLocations[z], ENTRY_SIZE ); 
    // entry size is 1k, MAX_FRAMES is 256, so we will have 256k main memory.
    // initialize the main memory

    setupShmMsg();
    // setting up shared memory and message queues.

    signal(SIGINT, sigHandler);
    // Setup signal handler to catch any inturrupts.

    if(remove(logfileName) == 1) printf("OSS: Deleting old logfile\n");
    // remove the old log if it exists. If it dosn't this won't do anything.
    fp = fopen("logfile", "w+");
    // attach to the logfile.

    numActive = 0;
    // set the number of active processes to 0.

    int currentProcessCount = 0;
    // a counter to check the number of launched processes for the termination criteria
    // if  current process count is greater than 40 no more processes should be created.

    while( 1 ) {
        // get input in an infinite loop until user types end.
        // used for scheduling and may get blocked by child process.

        if( waitpid(-1, &status, WNOHANG) > 0 ) numActive--;
        // if any child processes terminated decrement the numActive. (this does not block).

        if( numActive < maxSpecifiedProcesses  && currentProcessCount < MAX_NUM_CREATED_PROC ) {
            // handle launching a process.

            if(fork() == 0) {
                // fork a child process
                printf("launching %d\n", currentProcessCount);
                launchProcess(currentProcessCount, fp);
            }
            currentProcessCount++;
            numActive++;
        }

        msgrcv(msqid, &messageData, sizeof(messageData), 0, MSG_NOERROR);
        // wait for a message to come back from a child proces.

        switch((int)messageData.msgType) {
            // write a different message to the logfile depending on what kind of message is sent back.

            case REQ_WRITE: 
                {
                    int frameIndex = 0;
                    int location = 0;
                    int reqPid = -1; // will appear as -1 in the log file if fails.
                    char temp[MAX_MSG];
                    // used to keep the original message intact.

                    fflush(fp);
                    fprintf(fp,"%s\n", messageData.message);
                    // output the message the logfile

                    strcpy(temp, messageData.message);

                    location = getIntFromString(temp, 8);
                    // location of the read will be the 8th word in the message.

                    strcpy(temp, messageData.message);
                    reqPid = getIntFromString(temp, 2);

                    frameIndex = location >> 10;
                    // get the location to read from.

                    if(location == 999999999) {
                        // pagefault, this number is my 'guarenteed' pagefault.
                        fprintf(fp, "Process generated a pagefalut when requesting memory location %d\n",location);
                        sendMsg(PAGE_FAULT,"");
                        strcpy(messageData.message, "");
                        messageData.msgType = 0;
                        break;
                    }


                    // perform the second chance algorithm here.
                    if(secondChance(frameIndex)) {
                        // there was a pagefault here, all the dirty bits should be reset to 0.

                        printf("there were no page faults\n");

                    } else {

                        printf("there were a page fault\n");
                    }


                    if(locationAllocated(physicalLocations[frameIndex], location)){
                        // grant the read request right away since the location is set.
                        fprintf(fp, "Process generated a pagefault when trying to write to an existing memory location %d\n",location);
                        sendMsg(PAGE_FAULT,"");
                        strcpy(messageData.message, "");
                        messageData.msgType = 0;
                        break;

                    } else {
                        // location in frame is not in memory we need to advance the clock simulating bringing it from disc, then grant.

                        sharedClock[CLOCK_NS]  += getRandBetween(0, 500);
                        // increment the logical clock to simulate reading from disk.

                        fprintf(fp, "Address in frame %d, giving data to pid: %d\n", frameIndex, reqPid);

                        setbv(physicalLocations[frameIndex], location);
                        // put the location into memory.
                        char frameIndexStr[3];
                        sprintf(frameIndexStr, "%d", frameIndex);

                        sendMsg(GRANTED, frameIndexStr);

                    }

                    strcpy(messageData.message, "");
                    messageData.msgType = 0;
                    break;
                }

            case REQ_READ: 
                {

                    int frameIndex = 0;
                    int location = 0;
                    char temp[MAX_MSG];
                    // used to keep the original message intact.

                    fflush(fp);
                    fprintf(fp,"%s\n", messageData.message);

                    strcpy(temp, messageData.message);

                    location = getIntFromString(messageData.message, 8);
                    // location of the read will be the 8th word in the message.

                    frameIndex = location >> 10;
                    // get the location to read from.

                    if(location == 999999999) {
                        // pagefault, this number is my 'guarenteed' pagefault.
                        fprintf(fp, "Process generated a pagefalut when requesting memory location %d\n",location);
                        sendMsg(PAGE_FAULT,"");
                        strcpy(messageData.message, "");
                        messageData.msgType = 0;

                        break;
                    }

                    if(locationAllocated(physicalLocations[frameIndex], location)){
                        // grant the read request right away since the location is set.
                        printf("data read from location %d\n", location);
                    } else {
                        // location in frame is not in memory we need to advance the clock simulating bringing it from disc, then grant.

                        sharedClock[CLOCK_NS]  += getRandBetween(0, 500);
                        // increment the logical clock to simulate reading from disk.

                        setbv(physicalLocations[frameIndex], location);
                        // put the location into memory.
                    }

                    sendMsg(GRANTED, "");
                    strcpy(messageData.message, "");
                    messageData.msgType = 0;
                    break;
                }
        }

        if((currentProcessCount >= MAX_NUM_CREATED_PROC )) {
            // Terminate the OSS since there are no active processesand the termination criterion has been met.
            printf("num Processes  created: %d\n", currentProcessCount);
            kill(-getpid(), 2);
            break;
        }
    }

    fclose(fp);
    // close the file pointer

    shmdt(sharedIds);
    shmdt(activeProcesses);
    shmdt(sharedClock);
    // detach all from shared memory.

    shmctl(sharedIdsId, IPC_RMID,NULL);
    shmctl(sharedClockId, IPC_RMID,NULL);
    shmctl(activeId, IPC_RMID, NULL);
    shmctl(pcbsId,IPC_RMID,NULL);
    msgctl(msqid, IPC_RMID, NULL);

    // destroy any shared memeory, or message queues.
    printf("see 'logfile' for details\n");

    return 0;
}

int secondChance(int reference) {
    // check if a frame reference is in memory.
    // if no frame in memory just put the reference in that location
    // if there's no free spots in memory replace a pageframe using second chance.

    int i;
    int hadPageFault = 0; //true or false.
    int oldest = -1;  //oldest index when finding which page to replace

    for(i = 0; i < MAX_FRAMES; i++ ) {
        // iterate over all the frames

        if(frames[i].isEmpty == 0){ 
            frames[i].isEmpty = 1;
            frames[i].referenceNbr = reference;
            frames[i].dirtyBit = 0; // set the dirty bit to 0
            frames[i].age = 0;
            hadPageFault = 1;   // Since we had to bring a frame into memory we generated a pagefault.
            break;
        }

        if (frames[i].referenceNbr == reference) {
            frames[i].dirtyBit = 1; // set the dirty bit of the frame.
            frames[i].age++; 
            return hadPageFault;
        }

        frames[i].age++; // increase the age of every process, is so we can know the oldest frame so we can replace it.
    }

    // replace the page.

    for(i=0; i < MAX_FRAMES; i++) {
        if(oldest < frames[i].age && !frames[i].dirtyBit) oldest = frames[i].age;
    }
    // get the oldest non-dirtybit page.

    for(i=0; i < MAX_FRAMES; i++) {
        if(oldest == frames[i].age) {
            clearedFrame = frames[i].referenceNbr;
            frames[i].isEmpty = 1;
            frames[i].referenceNbr = reference;
            frames[i].age = 0;
            hadPageFault = 1;   // Since we had to bring a frame into memory we generated a pagefault.
            break;
        }
    }

    for(i = 0; i < MAX_FRAMES; i++) frames[i].dirtyBit = 0;
    // set all dirty bits to 0.


    return hadPageFault;
}



void setupShmMsg() {

    key_t messageQueueKey, sharedClockKey, sharedIdKey, activeKey, pcbsKey;
    // SECTION FOR SETTING UP SHARED MEMORY, MESSAGE QUEUE AND SIGNAL HANDLING. (1)

    sharedClockKey = ftok("userProcess.c", 'T');
    messageQueueKey = ftok("userProcess.c", 'B');
    sharedIdKey = ftok("userProcess.c", 'Z');
    activeKey = ftok("userProcess.c", 'J');
    pcbsKey = ftok("userProcess.c", 'L');
    // generate keys.

    sharedClockId = shmget(sharedClockKey, 2 * sizeof(unsigned int), IPC_CREAT | 0666);
    sharedIdsId = shmget(sharedIdKey, 5* sizeof(int), IPC_CREAT | 0666);
    msqid = msgget(messageQueueKey, 0666 | IPC_CREAT);
    pcbsId = shmget(pcbsKey, MAX_PROCESSES * sizeof(pcb_t*), IPC_CREAT | 0666);
    activeId = shmget(activeKey, 1 *sizeof(int), IPC_CREAT | 0666);
    // get ids.

    sharedClock = (unsigned int*) shmat(sharedClockId, NULL, 0);
    activeProcesses = (int * ) shmat(activeId, NULL, 0);
    sharedIds = (int * ) shmat(sharedIdsId, NULL, 0);
    pcbs = (pcb_t *) shmat(pcbsId, NULL, 0);

    if(sharedClockKey == -1 || messageQueueKey == -1 || sharedIdKey == -1 || activeKey == -1 || pcbsKey == -1) {
        // check for all valid keys.
        perror("OSS: ftok failed ");
        exit(1);
    }

    if(sharedClockId == -1 || msqid == -1 || sharedIdsId == -1 || activeId == -1 || pcbsId == -1){
        // check that all ids were valid.
        perror("OSS: ERROR getting ids ");
        exit(1);
    } 

    if  (sharedClock == NULL || sharedIds == NULL || activeProcesses == NULL || pcbs == NULL){
        // check for errors attaching to shared memory.
        perror("OSS: ERROR: attaching shared memory ");
        exit(1);
    }


    sharedIds[CLOCK_IND] = sharedClockId;
    sharedIds[MSGQ_IND] = msqid;
    sharedIds[ACTIVE_IND] = activeId;
    sharedIds[PCB_IND] = pcbsId;
    // store all the ids in the shared ids memory to simplify code for userProcess.c
    // doing this allows me to access all the ids created here rather than re-creating them in userProcess.c


}

void printHelp() {
    // Display a helpful message for the user.
    printf("oss \n");
    printf("-p %10c Specify the number of processes allowed in the system at a given time\n", ' ');
    printf("-h %10c Show a helpful message of how to use ./oss \n", ' ');
}

void sigHandler(int signum) {  
    printf("signal %d caught exiting...\n", signum);
    shmctl(sharedIdsId, IPC_RMID,NULL);
    shmctl(sharedClockId, IPC_RMID,NULL);
    shmctl(pcbsId,IPC_RMID,NULL);
    shmctl(activeId, IPC_RMID, NULL);
    msgctl(msqid, IPC_RMID, NULL);
    exit(1);
}


int getRandBetween(int min, int max) {
    srand(min*max);
    return (rand() % (max - min + 1)) + min;
}

void launchProcess(int pid, FILE * fp) {
    // Code to launch a user process.

    int  i = 0;
    char simulatedPid[3];
    char pcbLoc[3];

    for( ; i < MAX_PROCESSES; i++) {
        if(pcbs[i].state == ready) break;
        else if(i == MAX_PROCESSES - 1) return;
    }

    sprintf(simulatedPid, "%d", pid);
    sprintf(pcbLoc, "%d", i);

    char * processArgs[] = {"./user_proc", simulatedPid, pcbLoc, NULL};

    sharedClock[CLOCK_SEC] += getRandBetween(0, 1);
    sharedClock[CLOCK_NS]  += getRandBetween(0, 500);
    // increment the logical clock.

    pcbs[i].state = running;
    pcbs[i].simpid = pid;

    fflush(fp);
    fprintf(fp,"OSS launched process P%s atclock time: %d:%d\n", simulatedPid,  sharedClock[CLOCK_SEC], sharedClock[CLOCK_NS]);
    fclose(fp);
    execvp(processArgs[0], processArgs);

}

int getIntFromString(char * msg, int mesageIndex) {
    // split the message by 'words' (spaces) and depending on the messageIndex a word will be converted to integer via strtol.
    char * endPtr, *token;
    long requestedAddress = -1;
    int i = 0;


    printf("msg: %s\n", msg);
    token = strtok(msg, " ");
    // get the first tokend

    while( token != NULL && i < mesageIndex -1) {
        token = strtok(NULL, " ");
        i++;
    }

    requestedAddress = strtol(token,&endPtr, 10);

    return (int)requestedAddress;
}


int getUserProcessCount(int argc, char ** argv){
    int numProcesses = 18, option;

    while(( option = getopt(argc, argv, "hp:")) != -1) {

        switch(option) {

            case 'h':
                printHelp();
                break;

            case 'p':
                // update the number of producers.
                numProcesses = atoi(optarg);
                break;
            case '?':
                // Display a usage message if unrecognized argument found.
                printHelp();         
                break;

        }    
    }

    return numProcesses;
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
        exit(0);
    }
}

