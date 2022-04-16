/*  Author: Michael Trani
    April 2022       */

#include"p5.h"
#include"config.h"

std::string error_message;
std::string warning_message;

key_t share_key_1, share_key_2; // shared memory key

struct Shmem* shared_memory_clock;

int shared_memory_ID, msg_queueID; // shared memory id, message queue id




struct Mssgbuff Message_Buffer_1, Message_Buffer_2;

void child(int);
void parent(int);
void sigalrm_handler(int signum, siginfo_t* info, void* ptr);
void sigint_handler(int signum, siginfo_t* info, void* ptr);
void sigalrm_catcher();
void sigint_catcher();

int main(int argc, char* argv[]) {

    // Output for error messages
    error_message = argv[0];
    error_message.erase(0, 2) += "::ERROR: ";

    // Program argument handling
    int option;
    int termination_time = DEFAULT_TIME;
    std::string log_file_name = "logfile";

    while ((option = getopt(argc, argv, "hs:l:")) != -1) {
        switch (option) {
        case 'h':
            std::cout << "oss[-h][-s t][-l f]\n" <<
                "- h Describe how the project should be run and then, terminate.\n" <<
                "-s t Indicate how many maximum seconds before the system terminates\n" <<
                "- l f Specify a particular name for the log file\n";
            return 0;

        case 's':  // Indicate how many maximum seconds before the system terminates
            termination_time = atoi(optarg);
            break;

        case 'l': // Specify a particular name for the log file
            log_file_name = optarg;
            break;

        default:
            std::cout << "Unknown input\n";
            return 1;
        }
    }

    // Input validation
    if (termination_time <= 1) {
        std::cout << error_message << "::Invalid input:\n\tTime must be greater than 1.\n";
        return 1;
    }

    // Log file
    FILE* log_file_pointer;
    log_file_pointer = fopen(log_file_name.c_str(), "w+");


        // Clock
    if ((shared_memory_ID = shmget(share_key_1, SHMEM_SIZE, 0666 | IPC_CREAT)) == -1) {
        error_message += ": Shared Memory: key1 - shmget";
        perror(error_message.c_str());
        return 1;
    }

    if ((share_key_1 = ftok("makefile", 246811)) == (key_t)-1) {
        error_message += ": Shared Memory: key1";
        perror(error_message.c_str());
        return 1;
    }

    // This is the clock
    shared_memory_clock = (struct Shmem*)shmat(shared_memory_ID, NULL, 0);
    if (shared_memory_clock < 0) {


    }

    // Message queue
    if ((msg_queueID = msgget(share_key_2, 0666 | IPC_CREAT)) == -1) {
        error_message += ": Shared Memory: key2-msg_queueID";
        perror(error_message.c_str());
        return 1;
    }


    if ((share_key_2 = ftok("makefile", 246812)) == (key_t)-1) {
        error_message += ": Shared Memory: key2";
        perror(error_message.c_str());
        return 1;
    }

    // Signal Alarm Catching Functions
    sigalrm_catcher();
    sigint_catcher();


    // Initialize stared memory struct
    shared_memory_clock->shared_PID = 0;
    shared_memory_clock->seconds = 0;
    shared_memory_clock->nano_seconds = 0;

    // Generate time delta
    srand((unsigned int)time(NULL) + getpid());


    // Handle time-out
    alarm(termination_time);




    do {
        break;  // stop

        switch (fork()) {

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






    // cleanup
    shmdt(shared_memory_clock);
    if ((shmctl(shared_memory_ID, IPC_RMID, NULL)) == -1) { // Removes the message queue
        error_message += ": Shared Memory Removal [Program Exit]: shared_memory_ID";
        perror(error_message.c_str());
    }

    if (msgctl(msg_queueID, IPC_RMID, NULL) == -1) { // Removes the message queue
        error_message += ": Shared Memory Removal [Program Exit]: msg_queueID";
        perror(error_message.c_str());
    }

    system("sleep 2 && ipcs | grep trani");
    if (log_file_pointer) {
        fclose(log_file_pointer);
    }






    return 0;
}



// Parent Child functions
void parent(int temp) {
    // Get the pointer to shared block
    char* paddr = (char*)(shmat(shared_memory_ID, 0, 0));
    int* pint = (int*)(paddr);

    /* Write into the shared area. */
    *pint = temp;
}

void child(int temp) {

    //pid_t temp = getpid();

    std::string slave_pid_arg = std::to_string(temp);  // argument for slave - PID
    //std::string slave_time = timeFunction();       // argument for slave - time
    //std::string slave_max = std::to_string(total_processes);    // argument for total num of processes

    //execl("./user", "user", "-i", slave_pid_arg.c_str(), "-t", slave_time.c_str(), "-n", slave_max.c_str(), (char*)NULL);
    execl("user", "user", slave_pid_arg.c_str(), NULL);

    // If we get to this point the call failed.
    error_message += "::excel failed to execute.\n";
    perror(error_message.c_str());
    std::cout << "excel failed to execute.\n" << std::endl;
}

// Signal handlers
void sigalrm_handler(int signum, siginfo_t* info, void* ptr) {

    // ignore other interrupt signals
    signal(SIGINT, SIG_IGN);

    std::cout << warning_message << " Child processes exceeded limit\n";

    // clean shared mem
    shmdt(shared_memory_clock);
    shmctl(shared_memory_ID, IPC_RMID, NULL);
    if (msgctl(msg_queueID, IPC_RMID, NULL) == -1) { // Removes the message queue
        error_message += ": Shared Memory Removal [sigalrm_handler()]: key2-msg_queueID";
        perror(error_message.c_str());
    }

}

void sigint_handler(int signum, siginfo_t* info, void* ptr) {
    // prevents multiple interrupts
    signal(SIGINT, SIG_IGN);
    signal(SIGALRM, SIG_IGN);

    std::cout << warning_message << " Interrupt. Exiting.\n";

    // clean shared mem
    shmdt(shared_memory_clock);
    shmctl(shared_memory_ID, IPC_RMID, NULL);
    if (msgctl(msg_queueID, IPC_RMID, NULL) == -1) { // Removes the message queue
        error_message += ": Shared Memory Removal [sigint_handler()]: key2-msg_queueID";
        perror(error_message.c_str());
    }

}

void sigalrm_catcher() {
    static struct sigaction _sigact;

    memset(&_sigact, 0, sizeof(_sigact));

    _sigact.sa_sigaction = sigalrm_handler;
    _sigact.sa_flags = SA_SIGINFO;

    sigaction(SIGALRM, &_sigact, NULL);
}

void sigint_catcher() {
    static struct sigaction _sigact;

    memset(&_sigact, 0, sizeof(_sigact));

    _sigact.sa_sigaction = sigint_handler;
    _sigact.sa_flags = SA_SIGINFO;

    sigaction(SIGINT, &_sigact, NULL);
}