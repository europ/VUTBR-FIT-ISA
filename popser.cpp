/**
 * @author     Adrián Tóth
 * @date       4.10.2017
 * @brief      POP3 server
 */

#include <iostream>
#include <string>
#include <thread>

#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// class for options
class Args {

    public:

        bool a = false;
        bool c = false;
        bool p = false;
        bool d = false;
        bool r = false;
        string path_a = "";
        string port   = "";
        string path_d = "";

        // DEBUG INFO
        void print() {
            cout << "a = " << (a ? "T" : "False") << endl;
            cout << "c = " << (c ? "T" : "False") << endl;
            cout << "p = " << (p ? "T" : "False") << endl;
            cout << "d = " << (d ? "T" : "False") << endl;
            cout << "r = " << (r ? "T" : "False") << endl;
            cout << "path_a = " << path_a << endl;
            cout << "port   = " << port   << endl;
            cout << "path_d = " << path_d << endl;
        }

};

// print the help message and terminate the program
void usage() {
    string message =
        "This\n"
        "is\n"
        "help\n"
        "message\n"
    ;
    cout << message;
    exit(0);
}

// print the pertaining error message depending on exitcode and terminate the program with the exitcode
void error(int exitcode) {
    switch (exitcode) {
        case 1:
            cerr << "Invalid arguments!\n";
            break;
        case 2:
            cerr << "Argument value is missing!\n";
            break;
        case 3:
            cerr << "Arguments are missing!\n";
            break;
        case 4:
            cerr << "Wrong combination of \"-h\"!\n";
            break;
        case 5:
            cerr << "Wrong port!\n";
            break;
        case 6:
            cerr << "Wrong authentication file!\n";
            break;
        case 7:
            cerr << "Wrong maildir direcotry!\n";
            break;
        case 8:
            cerr << "socket() failed!\n";
            break;
        case 9:
            cerr << "bind() failed!\n";
            break;
        case 10:
            cerr << "listen() failed!\n";
            break;
        case 11:
            cerr << "accept() failed!\n";
            break;
        default:
            break;
    }
    exit(exitcode);
}

// check if STRING is an option
bool isarg(char* str) {
    if (strcmp(str, "-h") == 0) return true;
    else if (strcmp(str, "-a") == 0) return true;
    else if (strcmp(str, "-c") == 0) return true;
    else if (strcmp(str, "-p") == 0) return true;
    else if (strcmp(str, "-d") == 0) return true;
    else if (strcmp(str, "-r") == 0) return true;
    else return false;
}

// check PORT
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

// check if FILE exists
bool file_exists(const std::string path) {
    struct stat s;
    if((stat(path.c_str(),&s)==0)&&(s.st_mode & S_IFREG )) {
        return true;
    }
    else {
        return false;
    }
}

// check if DIRECTORY exists
bool dir_exists(const std::string& path) {
    struct stat s;
    if((stat(path.c_str(),&s)==0 )&&(s.st_mode & S_IFDIR)) {
        return true;
    }
    else {
        return false;
    }
}

// reset
void reset() { // TODO
    cout << "TODO - reset" << endl;
    cout << "-r ONLY" << endl;
}

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
        exit(1);
    }

    // check for other options
    else {
        int c;
        while ((c = getopt(*argc, argv, "a:cp:d:r")) != -1) {
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
            }
        }

        if( !args->p || !args->a || !args->d) {
            fprintf(stderr, "Wrong options!\n");
            exit(1);
        }

        // check port
        if (args->p) {
            if (!is_number(args->port.c_str())) {
                error(5);
            }
            // else {
            //     // TODO FIXIT
            //     // check port size (range)
            // }
        }

        // check authentication file existence
        if (args->a) {
            if (!file_exists(args->path_a)) {
                fprintf(stderr, "Wrong authentication file!\n");
                exit(2);
            }
        }

        // check maildir directory existence
        if (args->d) {
            if (!dir_exists(args->path_d)) {
                fprintf(stderr, "Wrong maildir direcotry!\n");
                exit(2);
            }
        }
    }

}

void thread_main(int socket) {
    std::thread::id tid = std::this_thread::get_id();
    cout << "CONNECTED (THREAD = "<< tid << ")" << endl;
    char buff[1024];
    int res = 0;
    while(1) {
        memset(&buff,0,sizeof(buff));
        res = recv(socket, buff, 1024,0);
        if (res <= 0)
            break;
        cout << buff;

        send(socket, buff, strlen(buff), 0);
    }
    close(socket);
}

void server_kernel(Args* args) {

    int rc;
    int port_number = atoi((args->port).c_str());
    int welcome_socket;
    struct sockaddr_in6 sa;
    struct sockaddr_in6 sa_client;
    socklen_t sa_client_len;

    // create an endpoint for communication
    sa_client_len=sizeof(sa_client);
    if ((welcome_socket = socket(PF_INET6, SOCK_STREAM, 0)) < 0) error(8);

    memset(&sa,0,sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_addr = in6addr_any;
    sa.sin6_port = htons(port_number);

    // bind a name to a socket
    if ((rc = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa))) < 0) error(9);

    // listen for connections on a socket
    if ((listen(welcome_socket, 2)) < 0) error(10); // TODO FIXIT listen(socket, PORT) ... check PORT number

    int communication_socket;
    while(1) {
        communication_socket = accept(welcome_socket, (struct sockaddr*)&sa_client, &sa_client_len);
        if (communication_socket == -1) { // TODO FIXIT - what to do if accept fails ???
            error(11);
        }
        else {
            // conection established, create a new independent thread
            std::thread thrd(thread_main, communication_socket);
            thrd.detach();
        }
    }
}

int main(int argc, char* argv[]) {

    Args args;
    argpar(&argc, argv, &args);
    args.print();
    cout << endl;

    server_kernel(&args);

    return 0;
}
