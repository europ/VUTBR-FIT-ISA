/**
 * @author     Adrián Tóth
 * @date       9.10.2017
 * @brief      POP3 server
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h> // fcntlk() - nonblocking socket
#include <sys/select.h> // select()

using namespace std;

// enum for states
enum State {
    AUTHORIZATION,
    TRANSACTION,
    UPDATE,
    STATE_ERROR
};

// enum for commands
enum Command {
    QUIT,
    STAT,
    LIST,
    RETR,
    DELE,
    NOOP,
    RSET,
    UIDL,
    USER,
    PASS,
    APOP,
    COMMAND_ERROR
};

// class for options
class Args {
    public:
        bool a = false;
        bool c = false;
        bool p = false;
        bool d = false;
        bool r = false;
        std::string path_a = "";
        std::string port   = "";
        std::string path_d = "";
        std::string username = "";
        std::string password = "";
};

// print the help message to stdout and terminate the program
void usage() {
    string message =
        "\n"
        "USAGE:\n"
        "\tHELP:"
        "\t[-h]\n"
        "\n"
        "\tRESET:\n"
        "\t[-r]\n"
        "\n"
        "\tLAUNCH\n"
        "\t[-a authentication_file] [-p port_numer] [-d mail_directory] (OPTIONAL: [-c] [-r])\n"
        "\n"
    ;
    std::cout << message;
    exit(0);
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
void reset() {
    // TODO
    // FIXIT
    cout << "TODO: reset()" << endl;
    exit(0);
}

// username and password loading from authentication file
void load_auth_file(Args* args) { // TODO FIXIT

    FILE* fd;
    fd = fopen(args->path_a.c_str(),"r");

    if (fd == NULL ) {
        fprintf(stderr, "Failed to open authentication file!\n");
        exit(1);
    }

    char buff[4096];
    int retval;

    memset(&buff,0,sizeof(buff));
    retval = fscanf(fd,"username = %s\n", buff  );

    if (retval == 0) {
        fprintf(stderr, "Failed to load username from authentication file!\n");
        exit(1);
    }

    args->username = buff;

    memset(&buff,0,sizeof(buff));
    retval = fscanf(fd,"password = %s",   buff  );

    if (retval == 0) {
        fprintf(stderr, "Failed to load password from authentication file!\n");
        exit(1);
    }

    args->password = buff;
    fclose(fd);
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
                default:
                    abort();
            }
        }

        if( !args->p || !args->a || !args->d) {
            fprintf(stderr, "Wrong options!\n");
            exit(1);
        }

        // check port
        if (args->p) {
            if (!is_number(args->port.c_str())) {
                fprintf(stderr, "Wrong port!\n");
                exit(1);
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
                exit(1);
            }
            else {
                load_auth_file(args);
            }
        }

        // check maildir directory existence
        if (args->d) {
            if (!dir_exists(args->path_d)) {
                fprintf(stderr, "Wrong maildir direcotry!\n");
                exit(1);
            }
        }
    }
}

Command get_command(std::string& str) {

    if (str.empty()) return COMMAND_ERROR;

    std::string cmd = "";
    for(unsigned i=0; i<str.length(); i++) {
        if (str[i] == ' ' || str[i] == '\0') break;
        cmd += str[i];
    }

    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd.compare("quit") == 0) return QUIT;
    else if (cmd.compare("stat") == 0) return STAT;
    else if (cmd.compare("list") == 0) return LIST;
    else if (cmd.compare("retr") == 0) return RETR;
    else if (cmd.compare("dele") == 0) return DELE;
    else if (cmd.compare("noop") == 0) return NOOP;
    else if (cmd.compare("rset") == 0) return RSET;
    else if (cmd.compare("uidl") == 0) return UIDL;
    else if (cmd.compare("user") == 0) return USER;
    else if (cmd.compare("pass") == 0) return PASS;
    else if (cmd.compare("apop") == 0) return APOP;
    else return COMMAND_ERROR;
}

std::vector<std::string> get_cmd_args(std::string& str) {

    std::vector<std::string> vector;

    if (str.empty()) return vector;

    std::string strbuff;
    std::istringstream iss(str.c_str());
    bool flag = false;
    while ( getline( iss, strbuff, ' ' ) ) {
        if (flag == false) { // first string is CMD, we dont need it
            flag = true;
        }
        else { // these are ARGS, we need to add them into vector
            vector.push_back(strbuff);
        }
    }

    return vector;
}

void thread_main(int socket) {

    State STATE = AUTHORIZATION;
    Command COMMAND;
    std::vector<std::string> CMD_ARGS;

    char buff[1024];
    int res = 0;
    string data;

    while(1) {

        // RESET BUFFER
        memset(&buff,0,sizeof(buff));

        // RECEIVE
        res = recv(socket, buff, 1024,0); // FIXIT TODO buffer size
        if (res == 0) { // client disconnected
            break;
        }
        else if (errno == EAGAIN && res <= 0) { // recv would block EWOULDBLOCK
            continue;
        }
        else if (res < 0) {
            fprintf(stderr, "recv() failed!\n");
            return;
        }

        // ADD BUFFER TO C++ STIRING
        data = buff;
        data = data.substr(0, data.size()-2); // remmove CRLF (last 2 characters)

        COMMAND = get_command(data);
        CMD_ARGS = get_cmd_args(data);

        std::cout << "CMD=" << COMMAND << endl;
        for (auto i = CMD_ARGS.begin(); i != CMD_ARGS.end(); ++i)
            std::cout << "CMD_ARG=" << *i << ' ';
        cout << "==========================" << endl;
        continue;

        switch(STATE){
            // ==========================================================
            case AUTHORIZATION:
                switch(COMMAND){
                    // ==================================================
                    case NOOP:
                        break;
                    // ==================================================
                    case QUIT:
                        break;
                    // ==================================================
                    case USER:
                        break;
                    // ==================================================
                    case PASS:
                        break;
                    // ==================================================
                    case APOP:
                        break;
                    // ==================================================
                    default:
                        fprintf(stderr, "Wrong command in authorization state!\n");
                        return;
                        break;
                }
                break;
            // ==========================================================
            case TRANSACTION:
                switch(COMMAND){
                    // ==================================================
                    case NOOP:
                        break;
                    // ==================================================
                    case QUIT:
                        break;
                    // ==================================================
                    case UIDL:
                        break;
                    // ==================================================
                    case STAT:
                        break;
                    // ==================================================
                    case LIST:
                        break;
                    // ==================================================
                    case RETR:
                        break;
                    // ==================================================
                    case DELE:
                        break;
                    // ==================================================
                    case RSET:
                        break;
                    // ==================================================
                    default:
                        fprintf(stderr, "Wrong command in transaction state!\n");
                        return;
                        break;
                }
                break;
            // ==========================================================
            case UPDATE:
                switch(COMMAND){
                    default:
                        break;
                }
                break;
            // ==========================================================
            default:
                break;
        }
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
    if ((welcome_socket = socket(PF_INET6, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket() failed!\n");
        exit(1);
    }

    memset(&sa,0,sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_addr = in6addr_any;
    sa.sin6_port = htons(port_number);

    // bind a name to a socket
    if ((rc = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa))) < 0) {
        fprintf(stderr, "bind() failed!\n");
        exit(1);
    }

    // listen for connections on a socket
    if ((listen(welcome_socket, 2)) < 0) { // TODO FIXIT listen(socket, PORT) ... check PORT number
        fprintf(stderr, "listen() failed!\n");
        exit(1);
    }

    int communication_socket;
    while(1) {

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(welcome_socket, &fds);
        if (select(welcome_socket + 1, &fds, NULL, NULL, NULL) == -1) {
            fprintf(stderr, "select() failed!\n");
            continue;
        }

        communication_socket = accept(welcome_socket, (struct sockaddr*)&sa_client, &sa_client_len);

        int flags = fcntl(communication_socket, F_GETFL, 0);
        int rc = fcntl(communication_socket, F_SETFL, flags | O_NONBLOCK);
        if (rc < 0) {
            fprintf(stderr, "fcntl() failed!\n");
            continue;
        }

        if (communication_socket == -1) { // TODO FIXIT - what to do if accept fails ???
            fprintf(stderr, "accept() failed!\n");
            continue;
        }
        else {
            // conection established
            std::thread thrd(thread_main, communication_socket);
            thrd.detach();
        }

    }
}

int main(int argc, char* argv[]) {

    Args args;
    argpar(&argc, argv, &args);

    server_kernel(&args);

    return 0;
}
