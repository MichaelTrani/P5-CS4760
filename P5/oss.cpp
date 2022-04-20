/*  Author: Michael Trani
    April 2022       
    Process Master      */

#include"p5.h"
#include"config.h"


void matrix_maker();
bool logcheck_trani();

void memory_wipe();
void memory_maker();
void sig_handle(int signal);

void increment_clock();
void clock(int seconds, std::string exe_name);
void countdown_to_interrupt(int seconds, std::string exe_name);

void need_calculator();
bool block_check(int process_index, int resource_index, int resources);

bool safety_algorithm(int process_index, int resource_index, int resources);



std::string error_message;
static FILE* log_file_pointer;
static std::string log_file_name = "logfile";

int sid, mqid;   /* shared memory id and  message queue id*/
int line_count = 0;
int simulated_PID = -1;

static bool verbose = false;
static bool five_second_alarm;

key_t mkey, skey; /* message queue key and shared memory key*/

static std::bitset<PROC_LIMIT> bitv; /* bitv tracks PCB use*/

struct Shmem* shared_mem;
struct Mssgbuff buffer1, buffer2;

static int blocked[PROC_LIMIT][2]; /* Stores process index - 0 resource index - 1 resource count*/

int main(int argc, char* argv[]) {
    signal(SIGINT, sig_handle);
    //srand((unsigned int)time(NULL) + getpid());

    log_file_pointer = fopen(log_file_name.c_str(), "w+");

    int total_procs = 0;
    int current_procs = 0;

    // SEED THE RANDOM GENERATOR USING NANO-SEC's
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand((time_t)ts.tv_nsec);


    std::string exe_name; 

    // Gets the executable name and removes "./"
    exe_name = argv[0];
    std::string ext = "/";
    if (exe_name.find(ext) != -1) {
        size_t findExt = exe_name.find_last_of("/");
        exe_name = exe_name.substr(findExt + 1, exe_name.length());
    }

    // Check if the verbose option was chosen
    int option;
    while ((option = getopt(argc, argv, "hvl:")) != -1) {
        switch (option) {
        case 'h':
            std::cout << "oss[-h][-s t][-l f]\n" <<
                "-v verbose logfile\n" <<
                "-l f Specify a particular name for the log file\n";
            return 0;
        case 'v': // Specify a particular name for the log file
            verbose = true;
            break;
        case 'l': // Specify a particular name for the log file
            log_file_name = optarg;
            break;
        default:
            std::cout << "Unknown input\n";
            return 1;
        }
    }

    memory_maker();


    // START COUNTDOWN TIMER
    int terminate_after = 5; // real life seconds to run the simulation
    countdown_to_interrupt(terminate_after, exe_name.c_str());

    // INITIALIZE THE SHARED MEMORY CLOCK
    shared_mem->sec = 0;
    shared_mem->nsec = 0;

    // Initialize 20 Resources
    for (int j = 0; j < RESOURCE_LIMIT; j++) {
        shared_mem->initial[j] = rand() % 10 + 1;
        shared_mem->available[j] = shared_mem->initial[j];
    }

    // Determines the number of sharable resources! 
    int num_shared = rand() % (MAX_S_RESOURCE - (MAX_S_RESOURCE - MIN_S_RESOURCE)) + MIN_S_RESOURCE;

    // Acts as a static bitvector for which resources are "sharable"
    while (num_shared != 0) {
        int random = rand() % (19 + 1) + 0; 
        if (shared_mem->sharable[random] == 0) { // Does not repeat a choice
            shared_mem->sharable[random] = 1;
            num_shared -= 1;
        }
    }

    // Initialize timer to generate new processes
    unsigned int new_sec = 0;
    unsigned int new_nsec = rand() % 500000000 + 1000000; // Add the random time from 1-500 ms to "new_nsec"

    // Move the shared clock ahead to represent work being done to schedule the very first process
    shared_mem->nsec = new_nsec;

    // DO A NONBLOCKING WAIT ON MESSAGES (if there is no message, increment the clock)
    int pcb_index = -1;
    int simulated_PID = -1;
    bool bitv_is_open = false;
    int spawn_nsec;
    int granted_requests = 0;
    pid_t childpid;


    while (1) {
        // Handles breaking from this loop whenever termination criteria was reached
        if ((five_second_alarm || total_procs == 40) && current_procs == 0) {
            break;
        }

        // Print matrices every 5 granted requests
        if (granted_requests >= 5) {
            granted_requests = 0;
            matrix_maker();
        }

        // When the shared clock has passed the time to launch a new process, it passes this check
        if ((shared_mem->sec == new_sec && shared_mem->nsec) >= new_nsec || shared_mem->sec > new_sec) {
            new_sec = 0;
            new_nsec = 0; // Reset the clock to fork new procs

            // Checks if the bitvector has any open spots
            for (int i = 0; i < bitv.size(); i++) {
                if (!bitv.test(i)) {
                    pcb_index = i;
                    simulated_PID = i + 2;
                    bitv.set(i); // We are now using this PID
                    bitv_is_open = true;
                    break;
                }
            }

            // If there were open spots, generate a new child proc
            if (bitv_is_open && total_procs < 40 && !five_second_alarm) { // pr_count < 3 can be removed at final version

                // For sending the process index through execl
                char indexPCB[3];
                snprintf(indexPCB, 3, "%d", pcb_index);

                // Fork and then execl the child                
                switch (childpid = fork()) {
                case -1:
                    error_message += "Failed to fork";
                    perror(error_message.c_str());
                    exit(EXIT_FAILURE);

                case 0:
                    shared_mem->pgid = getpid();
                    execl("./user", "user", indexPCB, '\0');
                    error_message = exe_name + ": Error: failed to execl";
                    perror(error_message.c_str());
                    exit(EXIT_FAILURE);

                }
            
            
                total_procs++;
                current_procs++; // Gets decremented when a process terminates     

                // Closes this so that a new process must be vetted by the bitvector check above
                bitv_is_open = false;

            }

            // Adds the shared clock + the random time from 1-500 ms (to temp variables new_sec and new_nsec)
            new_sec = shared_mem->sec;
            new_nsec = (rand() % 500000000 + 1000000) + shared_mem->nsec;

            // Correct the time to launch the NEXT PROCESS
            while (new_nsec >= 1000000000) {
                new_nsec -= 1000000000;
                new_sec += 1;
            }
        }

        // Add time to the shared clock, simulating the system performing calculations (taking CPU time)
        increment_clock();

        int count = 0;
        // Receive a message from the user_proc
        int msg = msgrcv(mqid, &buffer2, sizeof(buffer2), 1, IPC_NOWAIT);
        if (msg > 0) {
            // Grab the index of the user_proc, to know who sent the message to this parent process
            pcb_index = buffer2.mpcb_index;

            // TERMINATING 
            if (buffer2.mflag == 1) {
                current_procs--; // Decrement because a process has terminated

                if (logcheck_trani() && verbose) {
                    fprintf(log_file_pointer, "OSS has acknowledged that P%d is terminating\n", pcb_index);
                }
                if (logcheck_trani() && verbose) {
                    fprintf(log_file_pointer, "OSS releasing all resources for P%d\n", pcb_index);
                }

                // Release all resources for this process, and reset the claims/allocated matrices
                for (int i = 0; i < RESOURCE_LIMIT; i++) {
                    shared_mem->available[i] += shared_mem->alloc[pcb_index][i];
                    shared_mem->alloc[pcb_index][i] = 0;
                    shared_mem->claim[pcb_index][i] = 0;
                }
                // Open up the spot in the bitvector for this process index
                bitv.reset(pcb_index);

                // See if it is safe to unblock any processes
                for (int i = 0; i < PROC_LIMIT; i++) {
                    if (blocked[i][1] > 0) { // If this blocked process requested more than zero resources (meaning it was blocked)
                        bool safety = safety_algorithm(i, blocked[i][0], blocked[i][1]);
                        if (safety) { // If the algorithm said that this BLOCKED process was okay to be UNBLOCKED,

                            if (logcheck_trani()) {
                                fprintf(log_file_pointer, "OSS unblocking P%d at time %d:%d\n", i, shared_mem->sec, shared_mem->nsec);
                            }
                            if (logcheck_trani() && verbose) {
                                fprintf(log_file_pointer, "OSS granting P%d request for %d resources of type R%d at time %d:%09d\n", i, blocked[i][1], blocked[i][0], shared_mem->sec, shared_mem->nsec);
                            }

                            // Track the number of total granted requests
                            granted_requests++;

                            // Set the Resource index and count, for adding to allocated and removing from available
                            int resource_index = blocked[i][0];
                            int resource_count = blocked[i][1];

                            // Add the requested resource to the allocated vector for this process,
                            shared_mem->alloc[i][resource_index] += resource_count;

                            // Remove the # of requested resources from the available resources vector
                            shared_mem->available[resource_index] -= resource_count;

                            matrix_maker();

                            // When these values are zero, the process is seen as "UNBLOCKED"
                            blocked[i][0] = 0;
                            blocked[i][1] = 0;

                            // Send a message to this user process saying that it was granted the request and unblocked
                            buffer1.mtype = i + 2;
                            buffer1.msafe = safety;
                            msgsnd(mqid, &buffer1, sizeof(buffer1), 0);
                        }
                    }
                }
            }
            // REQUESTING
            else if (buffer2.mflag == 2) {

                if (logcheck_trani() && verbose) {
                    fprintf(log_file_pointer, "OSS has detected process P%d requesting %d resources of type R%d at time %d:%09d\n", pcb_index, buffer2.mresource_count, buffer2.mresource_index, shared_mem->sec, shared_mem->nsec);
                }

                if (logcheck_trani() && verbose) {
                    fprintf(log_file_pointer, "OSS running deadlock avoidance at time %d:%09d\n", shared_mem->sec, shared_mem->nsec);
                }

                // Check if the request gets blocked or not
                bool safe = block_check(pcb_index, buffer2.mresource_index, buffer2.mresource_count);

                // For the message to be sent to the user_proc
                buffer1.mtype = pcb_index + 2;
                buffer1.msafe = safe;

                // If the request for this process index was safe, do the following 
                if (buffer1.msafe == true) {

                    if (logcheck_trani() && verbose) {
                        fprintf(log_file_pointer, "OSS granting P%d request for %d resources of type R%d at time %d:%09d\n", pcb_index, buffer2.mresource_count, buffer2.mresource_index, shared_mem->sec, shared_mem->nsec);
                    }

                    // Track the number of granted requests
                    granted_requests++;

                    // Add the requested resource to the allocated vector for this process,
                    shared_mem->alloc[pcb_index][buffer2.mresource_index] += buffer2.mresource_count;

                    // Remove the # of requested resources from the available resources vector
                    shared_mem->available[buffer2.mresource_index] -= buffer2.mresource_count;

                    // Send a message to this user process saying that it was granted the request
                    msgsnd(mqid, &buffer1, sizeof(buffer1), 0);

                }// If the request was not safe, do the following         
                else {
                    if (logcheck_trani()) {
                        fprintf(log_file_pointer, "OSS blocking P%d Request Denied at time %d:%09d\n", pcb_index, shared_mem->sec, shared_mem->nsec);
                    }
                }
            }
            // RELEASING
            else if (buffer2.mflag == 3) { // Release of resources was requested

                if (logcheck_trani() && verbose) {
                    fprintf(log_file_pointer, "OSS releasing %d resources of type R%d for P%d at time %d:%09d\n", buffer2.mresource_count, buffer2.mresource_index, pcb_index, shared_mem->sec, shared_mem->nsec);
                }

                // Subtract the requested resource from the allocated vector for this process,
                shared_mem->alloc[pcb_index][buffer2.mresource_index] -= buffer2.mresource_count;

                // Add the # of requested resources to the available resource vector
                shared_mem->available[buffer2.mresource_index] += buffer2.mresource_count;

                // See if it is safe to unblock any processes
                for (int i = 0; i < PROC_LIMIT; i++) {
                    if (blocked[i][1] > 0) { // If this blocked process requested more than zero resources (meaning it was blocked)
                        bool safety = safety_algorithm(i, blocked[i][0], blocked[i][1]);
                        if (safety) { // If the algorithm said that this BLOCKED process was okay to be UNBLOCKED,

                            if (logcheck_trani()) {
                                fprintf(log_file_pointer, "OSS unblocking P%d at time %d:%d\n", i, shared_mem->sec, shared_mem->nsec);
                            }
                            if (logcheck_trani() && verbose) {
                                fprintf(log_file_pointer, "OSS granting P%d request for %d resources of type R%d at time %d:%09d\n", i, blocked[i][1], blocked[i][0], shared_mem->sec, shared_mem->nsec);
                            }

                            // Track the number of granted requests
                            granted_requests++;

                            // Set the Resource index and count, for adding to allocated and removing from available
                            int resource_index = blocked[i][0];
                            int resource_count = blocked[i][1];

                            // Add the requested resource to the allocated vector for this process,
                            shared_mem->alloc[i][resource_index] += resource_count;

                            // Remove the # of requested resources from the available resources vector
                            shared_mem->available[resource_index] -= resource_count;

                            matrix_maker();

                            // When these values are zero, the process is seen as "UNBLOCKED"
                            blocked[i][0] = 0;
                            blocked[i][1] = 0;

                            // Send a message to this user process saying that it was granted the request and unblocked
                            buffer1.mtype = i + 2;
                            buffer1.msafe = safety;
                            msgsnd(mqid, &buffer1, sizeof(buffer1), 0);
                        }
                    }
                }
            }
            count++;
        }
    }


    // Close the log file if it wasn't closed already
    if (log_file_pointer) {
        fclose(log_file_pointer);
    }
    log_file_pointer = NULL;

    // Clear shared memory
    memory_wipe();
    return 0;

}

/* This function calculates the "needed" matrix, which is what is left over to be requested*/
void need_calculator() {
    for (int i = 0; i < PROC_LIMIT; i++) {
        for (int j = 0; j < RESOURCE_LIMIT; j++) {
            shared_mem->needed[i][j] = 0;
            shared_mem->needed[i][j] = shared_mem->claim[i][j] - shared_mem->alloc[i][j];
        }
    }
}

/* Whenever a request is made, this function gets ran*/
bool block_check(int process_index, int resource_index, int resources) {

    /* Temporarily adds this request to the blocked 2D array*/
    blocked[process_index][0] = resource_index; /* The type of resource requested*/
    blocked[process_index][1] = resources; /* The number of resources requested*/

    /* Checks if the request was safe or not*/
    bool safe = safety_algorithm(process_index, resource_index, resources);

    /* When it is safe, unblock the process*/
    if (safe) {
        blocked[process_index][0] = 0; /* Unblocks the process*/
        blocked[process_index][1] = 0;
        return true;
    } /* Otherwise, the process stays blocked*/
    else {
        return false;
    }
}

bool safety_algorithm(int process_index, int resource_index, int resources) {

    /* Grabs the needed 2D array*/
    need_calculator();

    /* Every process starts "unfinished" (changes as this runs) */
    bool finished[PROC_LIMIT] = { 0 };

    /* This will be the sequence of safe or unsafe processes*/
    int safe_sequence[PROC_LIMIT];

    /* Copy for the available vector*/
    int copy_avail[RESOURCE_LIMIT];

    /* Copies the available vector*/
    for (int i = 0; i < RESOURCE_LIMIT; i++) {
        copy_avail[i] = shared_mem->available[i];
    }

    /* Perform the "requested" resources subtraction from the copy of available resources*/
    copy_avail[resource_index] -= resources;

    /* When there aren't enough available resources, return unsafe state*/
    if (copy_avail[resource_index] < 0) {
        return false;
    }

    /* Subtract the "requested" resources from the "needed" matrix*/
    shared_mem->needed[process_index][resource_index] -= resources;

    /* count tracks every process*/
    int count = 0;
    /* This loops until every process has been considered*/
    while (count < PROC_LIMIT) {
        int current_count = count;
        /* Loops through each process and determines if it is safe or not*/
        for (int x = 0; x < PROC_LIMIT; x++) {
            if (finished[x] == 0) { /* each proc starts at finished[x] == 0*/
                if (blocked[x][1] == 0 || x == process_index) { /* Ignores blocked processes and the process_index*/
                    int y; /* Once this hits the RESOURCE_LIMIT, there were requests*/
                    for (y = 0; y < RESOURCE_LIMIT; y++) { /* Check that each resource of this Process*/
                        if (shared_mem->needed[x][y] > copy_avail[y]) { /* is less than the available resources*/
                            break;
                        }
                    }
                    if (!bitv.test(x)) { /* This makes sure only running processes get tested*/
                        safe_sequence[count++] = -1; /* This process was not active or had 0 claims for every resource*/
                        finished[x] = 1; /* It was checked, finished*/
                    }
                    else if (y == RESOURCE_LIMIT) { /* There were requests,*/
                        for (int z = 0; z < RESOURCE_LIMIT; z++) { /* Add allocated resources to the copy*/
                            copy_avail[z] += shared_mem->alloc[x][z]; /* Add the allocated to the temp available array */
                        }
                        safe_sequence[count++] = x; /* Adds this process to the safe sequence*/
                        finished[x] = 1; /* It was checked, finished*/
                    }
                }
                else {
                    //fprintf(stderr, "P%d IS BLOCKED and is not the current process index to be checked\n", x); // remove if problem
                    safe_sequence[count++] = -1; /* Negatives are added for blocked processes (to be ignored)*/
                    finished[x] = 1; /* Marked as "finished"*/
                }
            }
        }
        /* The state of the system is unsafe if current_count == count*/
        if (current_count == count) {
            return false;
        }
    }

    if (logcheck_trani() && verbose) {

        fprintf(log_file_pointer, "OSS system: Safe state. Safe Sequence: |");
        for (int i = 0; i < PROC_LIMIT; i++) {
            if (i < PROC_LIMIT - 1 && safe_sequence[i] > 0) {
                /*fprintf(stderr, "P%d|", safe_sequence[i]);*/
                fprintf(log_file_pointer, "P%d|", safe_sequence[i]);
            }
            else if (i == PROC_LIMIT && safe_sequence[i] > 0) {
                /*fprintf(stderr, "P%d|\n\n", safe_sequence[i]);*/
                fprintf(log_file_pointer, "P%d|\n\n", safe_sequence[i]);
            }
        }
        /*fprintf(stderr, "\n");*/
        fprintf(log_file_pointer, "\n");
    }
    return true;
}

bool logcheck_trani() {
    if (++line_count < MAX_LOG_LINES)
        return true;

    return false;
}


void matrix_maker() {

    if (logcheck_trani() && verbose) {
        fprintf(log_file_pointer, "Allocation Matrix:\n");
    }

    for (int i = 0; i < PROC_LIMIT; i++) {

        if (logcheck_trani() && verbose) {

            fprintf(log_file_pointer, "|");

            for (int j = 0; j < RESOURCE_LIMIT; j++) {

                fprintf(log_file_pointer, "%2d", shared_mem->alloc[i][j]);
                fprintf(log_file_pointer, "|");
            }
            fprintf(log_file_pointer, "\n");
        }
    }
    if (logcheck_trani() && verbose) {

        fprintf(log_file_pointer, "\n");
    }

    if (logcheck_trani() && verbose) {

        fprintf(log_file_pointer, "Blocked:\n");
    }
    for (int i = 0; i < PROC_LIMIT; i++) {

        if (logcheck_trani() && verbose) {

            fprintf(log_file_pointer, "|");
            fprintf(log_file_pointer, "%2d|", blocked[i][0]);
            fprintf(log_file_pointer, "%2d|", blocked[i][1]);
            fprintf(log_file_pointer, "\n");
        }
    }
    if (logcheck_trani() && verbose) {
        fprintf(log_file_pointer, "\n");
    }
}

void memory_maker() {
    /*Shared Memory*/
    if ((skey = ftok("makefile", 'a')) == (key_t)-1) {
        error_message += " skey/ftok creation failure::";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }
    if ((sid = shmget(skey, sizeof(struct Shmem), IPC_CREAT | 0666)) == -1) {
        error_message += "sid/shmget allocation failure::";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }

    shared_mem = (struct Shmem*)shmat(sid, NULL, 0);

    /* Message queue setup*/
    if ((mkey = ftok("makefile", 'b')) == (key_t)-1) {
        error_message += "mkey/ftok: creation failure::";
        perror(error_message.c_str());
        memory_wipe();
        exit(EXIT_FAILURE);
    }
    /* Create the queue*/
    if ((mqid = msgget(mkey, 0644 | IPC_CREAT)) == -1) {
        error_message += "mqid/msgget: allocation failure::";
        perror(error_message.c_str());
        memory_wipe();
        exit(EXIT_FAILURE);
    }
}

/* Function to increment the clock*/
void increment_clock() {
    int random_nsec = rand() % 1000000 + 1;
    shared_mem->nsec += random_nsec;
    while (shared_mem->nsec >= 1000000000) {
        shared_mem->nsec -= 1000000000;
        shared_mem->sec += 1;
    }
}

/* Takes the shared memory struct and frees the shared memory*/
void memory_wipe() {

    /* Performs control operations on the message queue*/
    if (msgctl(mqid, IPC_RMID, NULL) == -1) { /* Removes the message queue */
        perror("oss: Error: Could not remove the  message queue");
        exit(EXIT_FAILURE);
    }

    shmdt(shared_mem); /* Detaches the shared memory of "shared_mem" from the address space of the calling process*/
    shmctl(sid, IPC_RMID, NULL);
    /* Performs the IPC_RMID command on the shared memory segment with ID "sid" */
}

/* Kills all child processes and terminates, and prints a log to log file and frees shared memory*/
void sig_handle(int signal) {
    if (signal == 2) {
        printf("\n[OSS]: CTRL+C was received, interrupting process\n");
    }
    if (signal == 14) { /* "wake up call" for the timer being done*/
        printf("\n[OSS]: The countdown timer has ended. Terminating.\n");
        five_second_alarm = true;
    }

    if (log_file_pointer) {
        fclose(log_file_pointer);
    }
    log_file_pointer = NULL;
    if (killpg(shared_mem->pgid, SIGTERM) == -1) { /* Tries to terminate the process group (killing all children)*/
        memory_wipe(); /* Clears all shared memory*/
        killpg(getpgrp(), SIGTERM);
        while (wait(NULL) > 0); /* Waits for them all to terminate*/
        exit(EXIT_SUCCESS);
    }
    while (wait(NULL) > 0);
    memory_wipe(); /* clears all shared memory */

    exit(EXIT_SUCCESS);
}

/* Countdown timer based on [-t time] in seconds */
void countdown_to_interrupt(int seconds, std::string exe_name) {
    std::string error_message;
    clock(seconds, exe_name.c_str());
    struct sigaction action; /* action created using struct shown above */
    sigemptyset(&action.sa_mask); /* Initializes the signal set to empty */
    action.sa_handler = &sig_handle; /* points to a signal handling function, */
              /* which specifies the action to be associated with SIGALRM in sigaction() */
    action.sa_flags = SA_RESTART; /* Restartable system call */
    /* SIGALRM below is an asynchronous call, the signal raised when the time interval specified expires  */
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        /* &action specifies the action to be taken when the timer is done */
        error_message = exe_name + ": Error: Sigaction structure was not initialized properly";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }
}

void clock(int seconds, std::string exe_name) {
    std::string error_message;
    struct itimerval time;
    /* .it_value = time until timer expires and signal gets sent */
    time.it_value.tv_sec = seconds; /* .tv_sec = configure timer to "seconds" whole seconds */
    time.it_value.tv_usec = 0; /* .tv_usec = configure timer to 0 microseconds */
    /* .it_interval = value the timer will be set to once it expires and the signal gets sent */
    time.it_interval.tv_sec = 0; /* configure the interval to 0 whole seconds */
    time.it_interval.tv_usec = 0; /* configure the interval to 0 microseconds */
    if (setitimer(ITIMER_REAL, &time, NULL) == -1) { /* makes sure that the timer gets set */
        error_message = exe_name + ": Error: Cannot arm the timer for the requested time";
        perror(error_message.c_str());
    } /* setitimer(ITIMER_REAL, ..., NULL) is a timer that counts down in real time, */
      /* at each expiration a SIGALRM signal is generated */
}

