/*  Author: Michael Trani
    April 2022       
    Process Master      */

#include"p5.h"
#include"config.h"

std::string error_message;
std::string warning_message;
static bool verbose = false;
static FILE* log_file_pointer;

key_t skey; // shared memory key
int sid;    // shared memory id
struct Shmem* shared_mem;
key_t mkey; // message queue key
int mqid; // message queue id

static bool five_second_alarm;

static std::bitset<PROC_LIMIT> bitv; // bitv tracks PCB use



void child(char);
void parent(int);
void memory_wipe();
void sig_handle(int signal);

int main(int argc, char* argv[]) {
    signal(SIGINT, sig_handle);
    srand((unsigned int)time(NULL) + getpid());

    // Output for error messages
    error_message = argv[0];
    error_message.erase(0, 2) += "::ERROR: ";

    // Program argument handling
    int option;
    int termination_time = DEFAULT_TIME;
    std::string log_file_name = "logfile";

    while ((option = getopt(argc, argv, "hvl:s:")) != -1) {
        switch (option) {
        case 'h':
            std::cout << "oss[-h][-s t][-l f]\n" <<
                "-v verbose logfile\n" <<
                "-s t Indicate how many maximum seconds before the system terminates\n" <<
                "-l f Specify a particular name for the log file\n";
            return 0;
        case 'v': // Specify a particular name for the log file
            verbose = true;
            break;
        case 'l': // Specify a particular name for the log file
            log_file_name = optarg;
            break;
        case 's':  // Indicate how many maximum seconds before the system terminates
            termination_time = atoi(optarg);
            break;
        default:
            std::cout << "Unknown input\n";
            return 1;
        }
    }

    // Log file
    log_file_pointer = fopen(log_file_name.c_str(), "w+");



    //Shared Memory
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
    else {
        shared_mem = (struct Shmem*)shmat(sid, NULL, 0);
    }


    shared_mem = (struct Shmem*)shmat(sid, NULL, 0);


    // Message queue setup
    if ((mkey = ftok("makefile", 'b')) == (key_t)-1) {
        error_message += "mkey/ftok: creation failure::";
        perror(error_message.c_str());
        memory_wipe();
        exit(EXIT_FAILURE);
    }
    // Create the queue
    if ((mqid = msgget(mkey, 0644 | IPC_CREAT)) == -1) {
        error_message += "mqid/msgget: allocation failure::";
        perror(error_message.c_str());
        memory_wipe();
        exit(EXIT_FAILURE);
    }

    // Handle time-out
    alarm(termination_time);

    // Initialize clock
    shared_mem->sec = 0;
    shared_mem->nsec = 0;

    // Create descriptors for 20 resources, out of which about 20% should be sharable resources1.
    for (int j = 0; j < RESOURCE_LIMIT; j++) {
        shared_mem->initial[j] = rand() % 10 + 1;
        shared_mem->available[j] = shared_mem->initial[j];
    }

    // Determines sharable resources
    int num_shared = rand() % (MAX_S_RESOURCE - (MAX_S_RESOURCE - MIN_S_RESOURCE)) + MIN_S_RESOURCE;
    
    
    // Which resources are "sharable"
    while (num_shared != 0) {
        int random = rand() % (19 + 1) + 0; // Chooses randomly
        if (shared_mem->sharable[random] == 0) { // Does not repeat a choice
            shared_mem->sharable[random] = 1;
            num_shared -= 1;
        }
    }

    // Initialize timer and loop variables
    unsigned int new_sec = 0;
    unsigned int new_nsec = rand() % 500000000 + 1000000;

    int total_procs = 0;
    int current_procs = 0;
    five_second_alarm = false;
    pid_t childpid;
    int granted_requests = 0;
    int pcb_index = -1;
    int simulated_PID = -1;
    bool bitv_is_open = false;

    // Advance the clock
    shared_mem->nsec = new_nsec; 
    bool temp = true;
    do {
        
        // Handles breaking from this loop 40 children or more than 5 real time sec
        if ((five_second_alarm || total_procs == 40) && current_procs == 0) {
            break;
        }

        // When the shared clock has passed the time to launch a new process, it passes this check
        if (shared_mem->sec == new_sec && shared_mem->nsec >= new_nsec || shared_mem->sec > new_sec) {
            new_sec = 0;
            new_nsec = 0; // Reset the clock to fork new procs

            // looks at bitvector for openings
            for (int i = 0; i < bitv.size(); i++) {
                if (!bitv.test(i)) {
                    pcb_index = i;
                    simulated_PID = i + 2;
                    bitv.set(i); // current pid
                    bitv_is_open = true;
                    break;
                }
            }

            // If there were open spots, generate a new child proc
            if (bitv_is_open && total_procs < 40 && !five_second_alarm) { 

                // For sending the process index through execl
                char indexPCB[3];
                snprintf(indexPCB, 3, "%d", pcb_index);


                switch (childpid = fork()) {

                case -1:
                    error_message += "Failed to fork";
                    perror(error_message.c_str());
                    exit(EXIT_FAILURE);

                case 0:
                    shared_mem->pgid = getpid();
                    child(0);
                    break;

                default:
                    //parent(third_process_counter);
                    break;
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

    } while (true);


    memory_wipe();
    if (log_file_pointer) {
        fclose(log_file_pointer);
    }
    system("ipcs | grep trani");
    return 0;
}



// Parent Child functions
void parent(int temp) {

}


void child(char temp) {

    execl("./user", "user", temp, '\0');

    // If we get to this point the call failed.
    error_message += "excel failed to execute";
    perror(error_message.c_str());
    exit(EXIT_FAILURE);
}


// Takes the shared memory struct and frees the shared memory
void memory_wipe() {

    // Performs control operations on the message queue
    if (msgctl(mqid, IPC_RMID, NULL) == -1) { // Removes the message queue
        perror("oss: Error: Could not remove the  message queue");
        exit(EXIT_FAILURE);
    }

    shmdt(shared_mem); // Detaches the shared memory of "shared_mem" from the address space of the calling process
    shmctl(sid, IPC_RMID, NULL); // Performs the IPC_RMID command on the shared memory segment with ID "sid"
}


// Kills all child processes and terminates, and prints a log to log file and frees shared memory
void sig_handle(int signal) {
    //statistics(); // Prints statistics
    if (signal == 2) {
        printf("\n[OSS]: CTRL+C was received, interrupting process!\n");
    }
    if (signal == 14) { // "wake up call" for the timer being done
        printf("\n[OSS]: The countdown timer [-t time] has ended, interrupting processes!\n");
        five_second_alarm = true;
    }

    if (log_file_pointer) {
        fclose(log_file_pointer);
    }
    log_file_pointer = NULL;
    if (killpg(shared_mem->pgid, SIGTERM) == -1) { // Tries to terminate the process group (killing all children)
        memory_wipe(); // Clears all shared memory
        killpg(getpgrp(), SIGTERM);
        while (wait(NULL) > 0); // Waits for them all to terminate
        exit(EXIT_SUCCESS);
    }
    while (wait(NULL) > 0);
    memory_wipe(); // clears all shared memory

    exit(EXIT_SUCCESS);
}