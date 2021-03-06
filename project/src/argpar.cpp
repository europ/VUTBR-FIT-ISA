#include "argpar.hpp"

#include <unistd.h> // getopt()
#include <string.h> // memset(), strcmp()

#include "checks.hpp"    // is_number(), file_exists(), dir_exists()
#include "logger.hpp"    // reset()
#include "constants.hpp" // PORT_MAX

// Print the help message to stdout and terminate the program
void usage() {
    const char* message =
        "\n"
        "USAGE:\n"
        "\tHELP:\n"
        "\tNecessary parameter: [-h]\n"
        "\n"
        "\tRESET:\n"
        "\tNecessary parameter: [-r]\n"
        "\n"
        "\tLAUNCH:\n"
        "\tNecessary parameters: [-a authentication_file] [-p port_numer] [-d mail_directory]\n"
        "\tOptional  parameters: [-c] [-r]\n"
        "\n";

    fprintf(stdout, "%s", message);
    exit(0);
}

// Function loads username and password from authentication file
void load_auth_file(Args* args) {

    FILE* fd;
    fd = fopen(args->path_a.c_str(),"r");
    if (fd == NULL ) { // check file open
        fprintf(stderr, "Failed to open authentication file!\n");
        exit(1);
    }

    int retval;
    char buff[4096];
    memset(&buff,0,sizeof(buff));

    retval = fscanf(fd,"username = %s\n", buff); // read username
    if (retval == 0) {
        fprintf(stderr, "Failed to load username from authentication file!\n");
        fclose(fd);
        exit(1);
    }
    args->username = buff; // add username to shared class

    memset(&buff,0,sizeof(buff));

    retval = fscanf(fd,"password = %s", buff); // read password
    if (retval == 0) {
        fprintf(stderr, "Failed to load password from authentication file!\n");
        fclose(fd);
        exit(1);
    }
    args->password = buff; // add password to shared class

    fclose(fd); // close file
}


// Parsing arguments and initializating class args
void argpar(int* argc, char* argv[], Args* args) {
    // [-h] [-a PATH] [-c] [-p PORT] [-d PATH] [-r]

    // check for "-h" (help option)
    for(int i=1; i<*argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            usage();
        }
     }

    // check for "-r" (reset only)
    if (*argc == 2 && (strcmp(argv[1], "-r") == 0)) {
        reset();
        exit(0);
    }

    // check for other options
    else {
        int c;
        while ((c = getopt(*argc, argv, "a:cp:d:r")) != -1) { // load options
            switch (c) {
                case 'a':
                    args->a = true;
                    args->path_a = optarg;
                    break;
                case 'c':
                    args->c = true;
                    break;
                case 'p':
                    args->p = true;
                    args->port = optarg;
                    break;
                case 'd':
                    args->d = true;
                    args->path_d = optarg;
                    break;
                case 'r':
                    args->r = true;
                    break;
                case '?':
                    fprintf(stderr, "Wrong options!\n");
                    exit(1);
                default:
                    abort();
            }
        }

        // check if port or authfile or maildir is missing
        if( !args->p || !args->a || !args->d) {
            fprintf(stderr, "Wrong options!\n");
            exit(1);
        }

        // check port
        if (args->p) {
            if (!is_number(args->port.c_str())) { // check if port is string consisting only from numbers
                fprintf(stderr, "Wrong port!\n");
                exit(1);
            }
            else {
                const char* str = args->port.c_str();
                long int number = strtol(str, NULL, 10);
                if ((number < 0) || (PORT_MAX < number) || (errno == ERANGE)) { // check if port is within 0-PORTMAX range
                    fprintf(stderr, "Wrong port! Port is not from range 0-%d.\n", PORT_MAX);
                    exit(1);
                }
            }
        }

        // check authentication file existence
        if (args->a) {
            if (!file_exists(args->path_a)) {
                fprintf(stderr, "Authentication file does not exist!\n");
                exit(1);
            }
            else {
                load_auth_file(args); // load username and password to class args
            }
        }

        // check maildir directory existence
        if (args->d) {

            /* 
             * If we have "-x a -y b" and we do not give a value "a",
             * getopt will load the following arguments next to "-x",
             * which is "-y" what is a parameter not a value
             */
            
            // this is a hack due to lazy getopt ...
            if ((args->path_d).compare("-c") == 0) {
                args->c = true;
            }
            if ((args->path_d).compare("-r") == 0) {
                args->r = true;
            }

            // the -a and -p will fail on checks before
        }

        // provide reset
        if (args->r) {
            reset();
        }
    }
}
