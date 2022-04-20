/*  Author: Michael Trani
    April 2022       */
#ifndef P5_H
#define P5_H
#pragma once

// P1 & P2 Libraries
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>

 // P3 Libraries
#include <semaphore.h>
#include <fcntl.h>

// P4 Libraries
#include <vector>
#include <sys/msg.h>
#include <bitset>
#include <queue>

// P5 Libraries
#include <sys/time.h>


#define PROC_LIMIT 18
#define RESOURCE_LIMIT 20

#define MIN_S_RESOURCE (int)(RESOURCE_LIMIT*0.15)  /* Minimum sharable resource */
#define MAX_S_RESOURCE (int)(RESOURCE_LIMIT*0.25)  /* Maximum sharable resource */


struct Shmem {
    int shared_PID = 0;
    unsigned int sec = 0;
    unsigned int nsec = 0;
    int initial[RESOURCE_LIMIT];  /* Number of initialized resources (1-10) */
    int available[RESOURCE_LIMIT];  /* Number of available resources */
    int sharable[RESOURCE_LIMIT];  /* 1's in the sharable resources indexes */
    int claim[PROC_LIMIT][RESOURCE_LIMIT];  /* Max claims matrix (C) */
    int alloc[PROC_LIMIT][RESOURCE_LIMIT];  /* Allocation matrix (A) */
    int needed[PROC_LIMIT][RESOURCE_LIMIT];  /* "max needed in future" matrix (C-A) */

    int pgid;

};

struct Mssgbuff {
    long mtype;  /* type of message being passed (explained in code) */
    int mflag;  /* to send a msg to a specific process */
    bool mrequest;  /* true for request, false for releas e */
    bool mterminated;  /* true if it terminated, false if it didnt */
    bool msafe;  /* true if safe state, false if unsafe state */
    int mresource_count;  /* Number of resources in the specific resource class to be requested/released */
    int mresource_index;  /* Index of the resource class being requested/released */
    int mpcb_index;
};


 /*Grabs time */
char* timeFunction() {  /* Grabs current time and outputs hour/min/sec */
    time_t current_sec = time(0);
    int length = 9;
    std::string formatted_time = "%H:%M:%S";

    struct tm* local = localtime(&current_sec);

    char* output = new char[length];
    strftime(output, length, formatted_time.c_str(), local);
    return output;
}

 /* Removes whitespace - no longer needed */
std::string whitespaceRemover(std::string modifyME) {  /* This removes an annoying whitespace fed into the program. */
    remove(modifyME.begin(), modifyME.end(), ' ');

    return modifyME;
}


#endif


