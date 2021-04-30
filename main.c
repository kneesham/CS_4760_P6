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

#include "messageType.h"     // Structure for the message queues
#include "sharedIds.h"      
#include "constants.h"
#include "pcbType.h"
#include "bits.h"
#include "pageTable.h"

void printHelp();
void sendMsg(int, const char *);
void timerHandler(int);
void sigHandler(int);
void setupShmMsg();
void launchProcess(int, FILE *);
int getRandBetween(int, int);
int getUserProcessCount(int, char**);

// Functions

int sharedClockId, msqid, sharedIdsId, activeId, pcbsTableId;
unsigned int * sharedClock;
int * activeProcesses;
int * sharedIds;

pcb_t * pcbs;
// Process control blocks.

int parentPid;
//globals for signal functions to use.

int main(int argc, char * argv[]) {

    FILE * fp;
    struct msg_t messageData;
    char buff[MAX_MSG];
    int numActive, status, maxSpecifiedProcesses = getUserProcessCount(argc, argv);
    char * logfileName = "logfile";

    int * frameBitVector;

    setupShmMsg();
    // setting up shared memory and message queues.

    signal(SIGINT, sigHandler);
    // Setup signal handler to catch any inturrupts.

    if(remove(logfileName) == 1) printf("OSS: Deleting old logfile\n");
    // remove the old log if it exists. If it dosn't this won't do anything.
    fp = fopen("logfile", "a");
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
                exit(0);
            }
            currentProcessCount++;
            numActive++;
        }



        msgrcv(msqid, &messageData, sizeof(messageData), 0, IPC_NOWAIT);
        // wait for a message to come back from a child proces.


        switch((int)messageData.msgType) {
            // write a different message to the logfile depending on what kind of message is sent back.

            case REQ_WRITE: 
                {
                    fprintf(fp,"%s\n", messageData.message);
                    // printf("the value of messageData.address = %d\n", messageData.address);
                    sendMsg(GRANTED, "");
                    strcpy(messageData.message, "");
                    messageData.msgType = 0;
                    break;
                }

            case REQ_READ: 
                {
                    fprintf(fp,"%s\n", messageData.message);
                    // printf("the value of messageData.address = %d\n", messageData.address);
                    sendMsg(GRANTED, "");
                    strcpy(messageData.message, "");
                    messageData.msgType = 0;
                    break;
                }
        }


        if((currentProcessCount >= MAX_NUM_CREATED_PROC )) {
            // Terminate the OSS since there are no active processesand the termination criterion has been met.
            printf("num Processes  created: %d\n", currentProcessCount);
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
    msgctl(msqid, IPC_RMID, NULL);

    // destroy any shared memeory, or message queues.
    printf("see 'logfile' for details\n");

    return 0;
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


    printf("msqid oss: %d\n", msqid);


    activeId = shmget(activeKey, 1 *sizeof(int), IPC_CREAT | 0666);
    pcbsTableId = shmget(pcbsKey, sizeof(pcb_t *) * MAX_PROCESSES, IPC_CREAT | 0666);
    // get ids.

    sharedClock = (unsigned int*) shmat(sharedClockId, NULL, 0);
    activeProcesses = (int * ) shmat(activeId, NULL, 0);
    sharedIds = (int * ) shmat(sharedIdsId, NULL, 0);
    pcbs = (pcb_t *) shmat(pcbsTableId, NULL, 0);

    if(sharedClockKey == -1 || messageQueueKey == -1 || sharedIdKey == -1 || activeKey == -1 || pcbsKey == -1) {
        // check for all valid keys.
        perror("OSS: ftok failed ");
        exit(1);
    }

    if(sharedClockId == -1 || msqid == -1 || sharedIdsId == -1 || activeId == -1 || pcbsTableId == -1){
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
    sharedIds[PCB_IND] = pcbsTableId;
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
    shmctl(activeId, IPC_RMID, NULL);
    msgctl(msqid, IPC_RMID, NULL);
    system("killall user_process");
    exit(1);
}


int getRandBetween(int min, int max) {
    srand(min*max);
    return (rand() % (max - min + 1)) + min;
}

void launchProcess(int pid, FILE * fp) {
    // Code to launch a user process.

    char simulatedPid[3];
    sprintf(simulatedPid, "%d", pid);
    char * processArgs[] = {"./user_proc", simulatedPid, NULL};

    sharedClock[CLOCK_SEC] += getRandBetween(0, 1);
    sharedClock[CLOCK_NS]  += getRandBetween(0, 500);
    // increment the logical clock.

    fflush(fp);
    fprintf(fp,"OSS launched process P%s atclock time: %d:%d\n", simulatedPid,  sharedClock[CLOCK_SEC], sharedClock[CLOCK_NS]);
    fclose(fp);
    execvp(processArgs[0], processArgs);

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

