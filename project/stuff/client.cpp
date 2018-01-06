#include <iostream>
#include <thread>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define PORT_MAX 65535
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

bool is_number(const char* str) {
    if (!*str)
        return false;
    while (*str) {
        if (!isdigit(*str))
            return false;
        else
            ++str;
    }
    return true;
}

unsigned int check_port(char* port) {
    if (!is_number(port)) {
        fprintf(stderr, "Wrong port!\n");
        exit(1);
    }
    else {
        long int number = strtol(port, NULL, 10);
        if ((number < 0) || (PORT_MAX < number) || (errno == ERANGE)) {
            fprintf(stderr, "Wrong port! Port is not from range 0-%d.\n", PORT_MAX);
            exit(1);
        }
    }
    return std::stoi(port);
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

    if (argc != 3) {
        fprintf(stderr, "Usage: %s \"IP\" \"PORT\"\n", argv[0]);
        exit(1);
    }
    int portnumber = check_port(argv[2]);
    struct hostent *server;

    if ((server = gethostbyname(argv[1])) == NULL) error("Wrong IP address!\nNo such host.\n",1); // find ip in network


    struct sockaddr_in server_address;
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(portnumber);

    int client_socket;
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) error("Could not create socket!\n",1);

    if (connect(client_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) != 0) error("Could not connect!\n",1);


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