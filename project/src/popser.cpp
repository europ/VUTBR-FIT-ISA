#include <mutex>  // std::mutex
#include <thread> // std::thread
#include <csignal>

#include <string.h> // memset()
#include <fcntl.h>  // fnctl()
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>

#include "fsm.hpp"
#include "md5.hpp"
#include "argpar.hpp"
#include "checks.hpp"
#include "logger.hpp"
#include "constants.hpp"
#include "datatypes.hpp"

// GLOBAL VARIABLES
bool flag_exit = false;
bool flag_mutex = false;
std::mutex mutex_maildir;

// Function used to handling SIGINT
void signal_handler(int x) {
    (void)x; // -Wunused-parameter
    flag_exit = true;
}

// Function is the kernel of this program, set up connection and threads
void server_kernel(Args* args) {

    // necessary variables for communication management
    int retval;
    int flags;
    int port_number = atoi((args->port).c_str()); // port number was checked before
    int welcome_socket;
    int communication_socket;
    struct sockaddr_in6 sa;
    struct sockaddr_in6 sa_client;
    socklen_t sa_client_len;

    // create an endpoint for communication
    sa_client_len=sizeof(sa_client);
    welcome_socket = socket(PF_INET6, SOCK_STREAM, 0);
    if (welcome_socket < 0) {
        fprintf(stderr, "socket() failed!\n");
        exit(1);
    }

    // reuse port
    int enable = 1;
    if (setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <  0) { 
        fprintf(stderr, "setsockopt(SO_REUSEADDR) failed!\n");
        close(welcome_socket);
        exit(1);
    }

    memset(&sa,0,sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_addr = in6addr_any;
    sa.sin6_port = htons(port_number);

    if (port_number == 0) { // PORT NUMBER 0 is reserved ... yell error
        fprintf(stderr, "bind() failed! Use another port!\n");
        close(welcome_socket);
        exit(1);
    }

    // bind a name to a socket
    retval = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa));
    if (retval < 0) {
        fprintf(stderr, "bind() failed! Use another port!\n");
        close(welcome_socket);
        exit(1);
    }

    // listen for connections on a socket
    retval = listen(welcome_socket, 2);
    if (retval < 0) {
        fprintf(stderr, "listen() failed!\n");
        close(welcome_socket);
        exit(1);
    }

    // connection management
    while(!flag_exit) {

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(welcome_socket, &fds);

        // setting up select which will wait until any I/O operation can be performed
        if (select(welcome_socket + 1, &fds, NULL, NULL, NULL) == -1) {
            if (flag_exit) {
                break;
            }
            fprintf(stderr, "select() failed!\n");
            continue;
        }

        // accept a connection on a socket
        communication_socket = accept(welcome_socket, (struct sockaddr*)&sa_client, &sa_client_len);
        if (communication_socket == -1) {
            fprintf(stderr, "accept() failed!\n");
            continue;
        }

        // manipulate file descriptor
        flags = fcntl(communication_socket, F_GETFL, 0);
        retval = fcntl(communication_socket, F_SETFL, flags | O_NONBLOCK);
        if (retval < 0) {
            fprintf(stderr, "fcntl() failed!\n");
            continue;
        }

        std::thread thrd(thread_main, communication_socket, args); // thread_main is a FinitStateMachine declared in fsm.hpp
        thrd.detach();
    }

    close(welcome_socket);
    return;
}

// The Main
int main(int argc, char* argv[]) {

    srand(time(NULL));
    std::signal(SIGINT, signal_handler); // signal handler

    Args args; // shared class
    argpar(&argc, argv, &args); // check arguments

    server_kernel(&args); // start server

    return 0;
}
