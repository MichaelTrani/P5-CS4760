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
#define PROC_LIMIT 18
#define RESOURCE_LIMIT 20

#define MIN_S_RESOURCE (int)(RESOURCE_LIMIT*0.15) // Minimum sharable resource
#define MAX_S_RESOURCE (int)(RESOURCE_LIMIT*0.25) // Maximum sharable resource


struct Shmem {
    int shared_PID = 0;
    unsigned int sec = 0;
    unsigned int nsec = 0;
};

struct Mssgbuff {
    long type; 
    int flag; 
    int priority;
};


//Grabs time
char* timeFunction() { // Grabs current time and outputs hour/min/sec
    time_t current_sec = time(0);
    int length = 9;
    std::string formatted_time = "%H:%M:%S";

    struct tm* local = localtime(&current_sec);

    char* output = new char[length];
    strftime(output, length, formatted_time.c_str(), local);
    return output;
}

// Removes whitespace - no longer needed
std::string whitespaceRemover(std::string modifyME) { // This removes an annoying whitespace fed into the program.
    remove(modifyME.begin(), modifyME.end(), ' ');

    return modifyME;
}


#endif


