#include <iostream>
#include <thread>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define BUFFSIZE 16384

bool die = false;

void error(const char* message, int exitcode) {
    fprintf(stderr,"%s",message);
    exit(exitcode);
}

void handler(int s) {
    (void)s;
    die = true;
}


void thrd1_body_recv(int client_socket) {

    char buffer[BUFFSIZE];
    long long int retval;

    while (1) {

        if (die) return;

        bzero(buffer, BUFFSIZE);
        retval=recv(client_socket, buffer, BUFFSIZE-1, 0);

        if (retval <= 0) {
            close(client_socket);
            exit(1);
        }

        fprintf(stdout,"SERVER: %s",buffer);
    }
}

void thrd2_body_send(int client_socket) {

    std::string line="";
    while(1) {

        if (die) return;

        line.clear();
        std::getline(std::cin, line);
        if (line.empty()) {
            std::cout << "CLIENT: EMPTY" << std::endl;
            send(client_socket, "\0", 1, 0);
        }
        else {
            std::cout << "CLIENT: " << line << std::endl;
            send(client_socket, line.c_str(), line.size(), 0);
        }

    }
}

int main(int argc, char* argv[]) {

    signal(SIGINT, handler);

    std::string ipaddress  = argv[1];
    int portnumber = std::stoi(argv[2]);

    struct hostent *server;
    if ((server = gethostbyname(ipaddress.c_str())) == NULL) error("Wrong IP address!\nNo such host.\n",1); // find ip in network


    // initialize server_address structure
    struct sockaddr_in server_address;
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(portnumber);

    // create socket
    int client_socket;
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) error("Could not create socket!\n",1);

    // connect
    if (connect(client_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) != 0) error("Could not connect!\n",1);


    // run thread for printing incoming messages
    std::thread thrd1(thrd1_body_recv, client_socket);
    thrd1.detach();

    std::thread thrd2(thrd2_body_send, client_socket);
    thrd2.detach();

    while(1) {
        if (die) break;
    }

    std::cout << std::endl << "=EXIT=" << std::endl;
    return 0;
}