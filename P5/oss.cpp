#include "p5.h"
#include"config.h"

std::string error_message;
std::string warning_message;

int main(int argc, char *argv[])
{

    error_message = argv[0];
    error_message.erase(0, 2);        // removes leading './' at start of program name
    error_message += "::ERROR: ";

    // user args section
    int option;
    int user_time = DEFAULT_TIME;
    std::string logname = "logfile";

    while ((option = getopt(argc, argv, "hs:l:")) != -1) {
        switch (option) {
        case 'h':
            std::cout << "oss[-h][-s t][-l f]\n" <<
                "- h Describe how the project should be run and then, terminate.\n" <<
                "-s t Indicate how many maximum seconds before the system terminates\n" <<
                "- l f Specify a particular name for the log file\n";
            return 0;

        case 's':  // Indicate how many maximum seconds before the system terminates
            user_time = atoi(optarg);
            break;

        case 'l': // Specify a particular name for the log file
            logname = optarg;
            break;

        default:
            std::cout << "Unknown input\n";
            return 1;
        }
    }    
    
    if (user_time <= 1) {
        std::cout << error_message << "::Invalid input:\n\tTime must be greater than 1.\n";
        return 1;
    }

    FILE* fptr;
    fptr = fopen(logname.c_str(), "w+");
    int line_number_tracker = 0;

    fclose(fptr);
    
    system("sleep 2 && ipcs | grep trani");
    
    return 0;
}