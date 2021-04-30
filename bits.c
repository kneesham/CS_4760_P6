#include <stdio.h>
#include <stdlib.h>

#define WORDSIZE 32     // word size
#define BITS_WS 5       // number of bits needed to get the word size.
#define MASK 0x1f       // least sig bits.

int initbv(int **bv, int val) {
    *bv = calloc(val/WORDSIZE + 1, sizeof(int));
    return *bv != NULL;
}

void setbv(int bv[], int i){
    bv[i>>BITS_WS] |= (1 << (i & MASK));
}

int frameIsAllocated(int bv[], int i ) {
    return bv[i>>BITS_WS] & (1 << (i & MASK));
}


/****************************************
int main() {
    // example code using these functions.
    int *bitVector, i = 0;
    int allocatedFrames[] = { 1, 5, 7, 9, 14 };
    int frameIndexToCheck = 255;    // Frame index that you want to allocate/read/write...
    int frameIndexToCheck2 = 5;      // Frame index that you want to allocate/read/write...
    
    initbv(&bitVector, 256);        // 256 because our total memory is 256k.
    //initialize

    for( ; i < 5 ; i++) 
        setbv(bitVector, allocatedFrames[i]);
    // Set the allocated frames into the bit vector

    if(frameIsAllocated(bitVector, frameIndexToCheck2)) printf("That frame has already been allocated!\n");
    if(!frameIsAllocated(bitVector, frameIndexToCheck)) printf("That frame is free go ahead, take the frame.\n");

    printf("bits: ");

    for(i = 0; i < 8; i++) {
        //We use 8 here because we initialized 256/32 with initbv
        printf("%d, ", bitVector[i]);
    }
    printf("\n");



    unsetbv(bitVector, 5);
    // unset the 5th frame in the bit vector

    if(frameIsAllocated(bitVector, frameIndexToCheck2)) printf("That frame has already been allocated!\n");
    if(!frameIsAllocated(bitVector, frameIndexToCheck2)) printf("That frame has been unset.\n");

    printf("bits: ");

    for(i = 0; i < 8; i++) {
        //We use 8 here because we initialized 256/32 with initbv
        printf("%d, ", bitVector[i]);
    }
    printf("\n");




    free(bitVector);

     return 0;
}

*******************************************/
