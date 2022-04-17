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





void child(int);
void parent(int);
void memory_wipe();


int main(int argc, char* argv[]) {
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
        error_message +=" skey/ftok creation failure::";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }

    if ((sid = shmget(skey, sizeof(struct Shmem), IPC_CREAT | 0666)) == -1) {
        error_message += ": sid/shmget allocation failure::";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }

    else {
        shared_mem = (struct Shmem*)shmat(sid, NULL, 0);
    }


    // SET UP A MESSAGE QUEUE
    if ((mkey = ftok("makefile", 'b')) == (key_t)-1) {
        error_message += ": mkey/ftok: creation failure::";
        perror(error_message.c_str());
        memory_wipe();
        exit(EXIT_FAILURE);
    }

    // Create a queue
    if ((mqid = msgget(mkey, 0644 | IPC_CREAT)) == -1) {
        error_message +=": mqid/msgget: allocation failure::";
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
    
    std::cout << num_shared;
    
    // Which resources are "sharable"
    while (num_shared != 0) {
        int random = rand() % (19 + 1) + 0; // Chooses randomly
        if (shared_mem->sharable[random] == 0) { // Does not repeat a choice
            shared_mem->sharable[random] = 1;
            num_shared -= 1;
        }
    }

    // Initialize timer
    unsigned int new_sec = 0;
    unsigned int new_nsec = rand() % 500000000 + 1000000;


    // Advance the clock
    shared_mem->nsec = new_nsec; 

    
    do {
        break;  // temp stop

        int garbage = 0;
        //switch (fork()) {
        switch(garbage){

        case -1:
            error_message += "::Failed to fork.\n";
            perror(error_message.c_str());
            return (1);

        case 0:
            //child(NULL);
            break;

        default:
            //parent(third_process_counter);
            break;
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

void child(int temp) {
    std::string slave_pid_arg = std::to_string(temp);  // argument for slave - PID

    execl("user", "user", slave_pid_arg.c_str(), NULL);

    // If we get to this point the call failed.
    error_message += "::excel failed to execute.\n";
    perror(error_message.c_str());
    std::cout << "excel failed to execute.\n" << std::endl;
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
