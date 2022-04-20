/*   Author: Michael Trani
     April 2022     */

#include"p5.h"
void sig_handler(int signal);
void shared_memory();


std::string error_message;
std::string warning_message;


struct Shmem* shared_mem;
struct Mssgbuff buffer1, buffer2;

key_t skey, mkey;
int sid;     
int mqid; 

int main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL) + getpid());

    int indexPCB = atoi(argv[1]); // Grabs the simulated PID from the execl command

   // std::cout << "user " << indexPCB << " says hello\n";

     /* Output for error messages */
    error_message = argv[0];
    signal(SIGTERM, sig_handler);  /* Detects signals to terminate this process */
    shared_memory();

    // Declare maximum claims
    int claims_left[RESOURCE_LIMIT]; // Copies claims array (to be modified locally)

    // Calculate the max claims matrix for this user process 
    for (int j = 0; j < RESOURCE_LIMIT; j++) {
        shared_mem->claim[indexPCB][j] = rand() % (shared_mem->initial[j] + 1);
        claims_left[j] = shared_mem->claim[indexPCB][j];
    }

    // To see if a process has ran for its random value of time
    int temp_clock_s;
    int temp_clock_ns;

    // Grab the current clock time (at time of creation)
    int user_clock_s = shared_mem->sec + 1; // Add 1 second
    int user_clock_ns = shared_mem->nsec;

    // The latest time to request/release (MUST PASS THIS TIME TO REQUEST/RELEASE)
    int acquire_resources_s = shared_mem->sec;
    int acquire_resources_ns = shared_mem->nsec + rand() % 1000000 + 0; // Grabs the current nano time and adds nanoseconds

    // Percentage chance to request a resource vs. release one
    int chance_to_request = 90;
    bool finished = false;
    bool terminate = false;
    bool request = false;
    bool release = false;
    bool has_resources = false;
    bool requests_left_over = true;
    int terminate_s;
    int terminate_ns;

    do {

        // finished becomes true whenever this process has ran for 1 second + the random time generated above
        if (!finished) {
            // Grab THIS CHILD's new clock time
            temp_clock_s = shared_mem->sec;
            temp_clock_ns = shared_mem->nsec;

            // MAKE SURE THE PROCESS HAS RAN FOR AT LEAST 1 SECOND BEFORE DEALLOCATING ALL RESOURCES
            if ((temp_clock_ns - user_clock_ns) >= 1000000000) {
                finished = true;
            }
            else if ((temp_clock_s - user_clock_s) >= 1) {
                finished = true;
            }

            // Set the time to terminate
            if (finished) {
                terminate_s = shared_mem->sec;
                terminate_ns = shared_mem->nsec + rand() % 2500000000 + 0;
                if (terminate_ns >= 1000000000) {
                    terminate_ns -= 1000000000;
                    terminate_s += 1;
                }
            }

            // Make sure that this child has ran for at least the random value from "acquire_resources_ns", (random from 0 to 1ms)
            if (shared_mem->sec == acquire_resources_s && shared_mem->nsec >= acquire_resources_ns || shared_mem->sec > acquire_resources_s) {
                if (chance_to_request <= rand() % 100 + 1 && requests_left_over) { // If the random value from 1-100 was greater than
                                 // or equal to the percentage to request, AND there is available room to request, then we go in here
                    has_resources = true;

                    // Grabs a random resource from a random resource class and makes sure there are claims left over
                    do {
                        buffer2.mresource_index = rand() % RESOURCE_LIMIT + 0;
                        buffer2.mresource_count = rand() % (claims_left[buffer2.mresource_index] + 1);
                    } while (buffer2.mresource_count == 0);
                    claims_left[buffer2.mresource_index] -= buffer2.mresource_count; // Keeps accurate count of Max Claims available for future requests

                    // Check to see if there is room for more requests or not
                    requests_left_over = false;
                    for (int i = 0; i < RESOURCE_LIMIT; i++) {
                        if (claims_left[i] != 0) {
                            requests_left_over = true;
                            break;
                        }
                    }
                    // Get ready to send the request message
                    buffer2.mflag = 2;
                    request = true;
                }
                // If the value from that random 1-100 above was LESS THAN the percentage to request, then we release a resource
                else if (has_resources) {
                    requests_left_over = true;

                    // Grabs a random resource from a random resource class (from the allocated resources row of this user process)
                    do {
                        buffer2.mresource_index = rand() % RESOURCE_LIMIT + 0;
                        buffer2.mresource_count = rand() % (shared_mem->alloc[indexPCB][buffer2.mresource_index] + 1);
                    } while (buffer2.mresource_count == 0);
                    claims_left[buffer2.mresource_index] += buffer2.mresource_count; // Keeps accurate count of Max claims available for future requests

                    // Check to see if this process has resources or not
                    has_resources = false;
                    for (int i = 0; i < RESOURCE_LIMIT; i++) {
                        if (shared_mem->alloc[indexPCB][i] != 0) {
                            has_resources = true;
                        }
                    }
                    // Get ready to send the release message
                    buffer2.mflag = 3;
                    release = true;
                }
            }
        }// end of finished

        
        else { // Process has ran for at least one second, check if it should terminate
            if (shared_mem->sec == terminate_s && shared_mem->nsec >= terminate_ns || shared_mem->sec > terminate_s) {
                buffer2.mflag = 1;
                terminate = true;
            }
        }
        // Send the message to OSS concerning if a resource was requested/released or if this process terminated 
        buffer2.mpcb_index = indexPCB;
        buffer2.mtype = 1;
        msgsnd(mqid, &buffer2, sizeof(buffer2), 0);


        if (request == true) {
            // Message from OSS to see if its safe to request resources
            msgrcv(mqid, &buffer1, sizeof(buffer1), indexPCB + 2, 0);
        }
        if (terminate) {
            break;
    
        }

    }while(true);


    shmdt(shared_mem); 
    return 0;
}

void sig_handler(int signal) { // Interrupt handler
    if (signal == 15) {
        //printf("[USER]: %d -- Process forced to terminate.\n", getpid());
        exit(EXIT_FAILURE);
    }
}


void shared_memory() {
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
     /* Connect to the queue */
    if ((mqid = msgget(mkey, 0644)) == -1) {
        error_message += "mqid/msgget: allocation failure";
        perror(error_message.c_str());
        exit(EXIT_FAILURE);
    }   
}