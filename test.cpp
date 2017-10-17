#include <iostream>
#include <string>

#include <ctime>
#include <sys/types.h>
#include <unistd.h>

#define HOSTNAME_LENGTH 64

int main(/*int argc, char* argv[]*/) {


    time_t t = time(NULL);
    pid_t  p = getpid();
    int retval;

    char hostname[HOSTNAME_LENGTH];
    retval = gethostname(hostname, HOSTNAME_LENGTH);
    if(retval != 0) {
        fprintf(stderr, "gethostname() failed!\n");
        exit(1);
    }

    std::cout << t << std::endl;
    std::cout << p << std::endl;
    std::cout << hostname << std::endl;

    return 0;
}
