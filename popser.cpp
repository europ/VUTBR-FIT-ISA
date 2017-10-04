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

        bool h = false;
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
            cout << "h = " << (h ? "T" : "False") << endl;
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

void argpar(int* argc, char* argv[], Args* args) {

    // NO args
    if (*argc <= 1) {
        error(3);
    }

    for(int i=1; i<*argc; i++) {
        // HELP arg
        if (strcmp(argv[i], "-h") == 0) {
            args->h = true;
        }
        // AUTH FILE arg + load its value
        else if (strcmp(argv[i], "-a") == 0) {
            args->a = true;
            if (i+1 < *argc && !isarg(argv[i+1])) {
                args->path_a = argv[i+1];
                i++;
            }
            else {
                error(2);
            }
        }
        // CLEAR PASS arg
        else if (strcmp(argv[i], "-c") == 0) {
            args->c = true;
        }
        // PORT arg + load its value
        else if (strcmp(argv[i], "-p") == 0) {
            if (i+1 < *argc && !isarg(argv[i+1])) {
                args->p = true;
                args->port = argv[i+1];
                i++;
            }
            else {
                error(2);
            }
        }
        // MAILDIR arg + load its value
        else if (strcmp(argv[i], "-d") == 0) {
            if (i+1 < *argc && !isarg(argv[i+1])) {
                args->d = true;
                args->path_d = argv[i+1];
                i++;
            }
            else {
                error(2);
            }
        }
        // RESET arg
        else if (strcmp(argv[i], "-r") == 0) {
            args->r = true;
        }
        else {
            error(1);
        }
    }

    // TODO FIXIT arguments from forum

    // check help
    if (args->h) {
        if (args->a == true || args->c == true || args->p == true || args->d == true || args->r == true) { // -h is with others (not first)
            error(4);
        }
        else if (*argc > 2) { // -h is first with others
            error(4);
        }
        else { // only -h
            usage();
        }
    }

    // check port
    if (args->p) {
        if (!is_number(args->port.c_str())) {
            error(5);
        }
        /*
        else {
            // TODO FIXIT
            // check port size (range)
        }
        */
    }

    // check authentication file existence
    if (args->a) {
        if (!file_exists(args->path_a)) {
            error(6);
        }
    }

    // check maildir directory existence
    if (args->d) {
        if (!dir_exists(args->path_d)) {
            error(7);
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
