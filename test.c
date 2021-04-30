#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>



int main() {

    int max = 40;
    int i  = 0;


    for( ; i < max; i++ ) {

        if(fork() == 0) {

            printf("child %d\n", i);
            exit(0);
        } else {
        
        wait(NULL);
        }

    }


    return 0;
}
