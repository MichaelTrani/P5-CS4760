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
#define QUANTUM 40000
#define QUANTUM2 40000*2
#define QUANTUM3 40000*3


// process control block
struct Pcb {
    int simPID; // local simulated pid
    int priority; // current priority
    unsigned int total_seconds; // time in system
    unsigned int total_nanoseconds;
    unsigned int CPU_seconds; // total CPU time used
    unsigned int CPU_nanoseconds;
    unsigned int burst_seconds; // total time used during last burst1
    unsigned int burst_nanoseconds;
    unsigned int start_s; // When this process was generated
    unsigned int start_ns;
    unsigned int resume_seconds;
    unsigned int resume_nnanoseconds;
};

struct Shmem {
    int shared_PID = 0;
    unsigned int seconds = 0;
    unsigned int nano_seconds = 0;
    Pcb process_array[PROC_LIMIT]; // Array storing Process Control Blocks for each child process in the system
    int process_group_ID;


};

struct Mssgbuff {
    long type; 
    int flag; 
    int priority;
};




//Legacy shared memory from previous projects up to 3.
//key_t shmkey;
int shmid_shared_num;
//int* shared_num_ptr;
#define STR_SZ  sizeof ( std::string )
#define INT_SZ  sizeof ( int )
#define PERM (S_IRUSR | S_IWUSR)
//#define SEMAPHORE_NAME "/semname"
#define SHMEM_SIZE sizeof(Shmem)
#define SHMKEY  859047


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


