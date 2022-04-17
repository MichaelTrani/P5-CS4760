/*   Author: Michael Trani
     April 2022     */

#include"p5.h"
void sig_handler(int signal);



std::string error_message;
std::string warning_message;


struct Shmem* shared_mem;

key_t skey; // shared memory key
int sid;    // shared memory id
key_t mkey; // message queue key
int mqid; // message queue id

int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL) + getpid());
    std::cout << "user says hello\n";

    // Output for error messages
    error_message = argv[0];

    signal(SIGTERM, sig_handler); // Detects signals to terminate this process

    if ((skey = ftok("makefile", 'a')) == (key_t)-1) {
        error_message += "skey/ftok creation failure";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }
    if ((sid = shmget(skey, sizeof(struct Shmem), 0)) == -1) {
        error_message += "sid/shmget allocation failure";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }
        
    shared_mem = (struct Shmem*)shmat(sid, NULL, 0);

    /* SET UP MESSAGE QUEUE */
    if ((mkey = ftok("makefile", 'b')) == (key_t)-1) {
        error_message += "mkey/ftok: creation failure";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }
    // Connect to the queue
    if ((mqid = msgget(mkey, 0644)) == -1) {
        error_message += "mqid/msgget: allocation failure";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }    return 0;

    std::cout << "user says hello\n";
    return 0;
}


// Handles interrupt
void sig_handler(int signal) { 
    if (signal == 15) {
        exit(EXIT_FAILURE);
    }
}