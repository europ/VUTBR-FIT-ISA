/**
 * @author     Adrián Tóth
 * @date       18.10.2017
 * @brief      POP3 server
 */

//#define md5

#include <ctime>
#include <mutex>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <csignal>
#include <iostream>
#include <algorithm>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>

#ifdef md5
#include <openssl/md5.h>
#endif

using namespace std;

#define HOSTNAME_LENGTH 64
#define PORT_MAX 65535
#define LOG_FILE_NAME "log"

bool flag_exit = false;

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
        std::string path_maildir_new = "";
        std::string path_maildir_cur = "";
        std::string path_maildir_tmp = "";
};

// print the help message to stdout and terminate the program
void usage() {
    std::string message =
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
        "\n"
    ;
    fprintf(stdout, "%s", message.c_str());
    exit(0);
}

// Function used to handling SIGINT
void signal_handler(int x) {
    (void)x; // -Wunused-parameter
    flag_exit = true;
    exit(0);
}

// Check string PORT if it consists from numbers only
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

// Function get the file size in octets
std::string file_size(std::string filename) {
    FILE* file;
    file = fopen(filename.c_str(),"rb");
    fseek(file, 0L, SEEK_END);
    long long unsigned int size = ftell(file);
    return std::to_string(size);
}

// Check if FILE exists
bool file_exists(const std::string path) {
    struct stat s;
    if((stat(path.c_str(),&s)==0)&&(s.st_mode & S_IFREG )) {
        return true;
    }
    else {
        return false;
    }
}

// Check if DIRECTORY exists
bool dir_exists(const std::string& path) {
    struct stat s;
    if((stat(path.c_str(),&s)==0 )&&(s.st_mode & S_IFDIR)) {
        return true;
    }
    else {
        return false;
    }
}

std::vector<std::string> load_logfile() {

    std::ifstream logfile(LOG_FILE_NAME);
    std::vector<std::string> files;
    std::string line;
    while (std::getline(logfile, line)) {
        files.push_back(line);
    }

    logfile.close();
    return files;
}

bool filename_is_in_dir(std::string filename, std::string directory) {
    std::string data;
    data = (directory.back() == '/') ? directory : directory + "/";
    data.append(filename);
    return (file_exists(data)) ? true : false;
}

// Function move file to specified directory
bool move_file(std::string filepath, std::string dirpath) {

    if (filepath.empty() || dirpath.empty()) {
        return false;
    }

    std::string filename, newfilepath;
    filename = filepath.substr(filepath.find_last_of("/")+1, std::string::npos);
    newfilepath = (dirpath.back() == '/') ? dirpath : dirpath + "/";
    newfilepath.append(filename);

    int retval = rename(filepath.c_str(), newfilepath.c_str());
    if (retval != 0) {
        return false;
    }
    return true;
}

// Reset
void reset() {

    if (!file_exists(LOG_FILE_NAME)) {
        return;
    }

    std::vector<std::string> files;
    files = load_logfile();

    std::string buff;
    std::string path_cur;
    std::string path_new;

    buff = files.front();
    buff = buff.substr(0, buff.find_last_of("/"));
    buff = buff.substr(0, buff.find_last_of("/")+1);

    path_cur = path_new = buff;

    path_cur.append("cur");
    path_new.append("new");

    std::string filename;
    std::string absfilepath;

    for (auto i = files.begin(); i != files.end(); ++i) {
        absfilepath = *i;
        filename = absfilepath.substr(absfilepath.find_last_of("/")+1, std::string::npos);
        buff = path_cur + "/" + filename;
        if (filename_is_in_dir(filename,path_cur)) {
            move_file(buff,path_new);
        }
    }

    remove(LOG_FILE_NAME);

    return;
}

// Function loads username and password from authentication file
void load_auth_file(Args* args) { // TODO FIXIT

    FILE* fd;
    fd = fopen(args->path_a.c_str(),"r");
    if (fd == NULL ) {
        fprintf(stderr, "Failed to open authentication file!\n");
        exit(1);
    }

    int retval;
    char buff[4096];
    memset(&buff,0,sizeof(buff));

    retval = fscanf(fd,"username = %s\n", buff);
    if (retval == 0) {
        fprintf(stderr, "Failed to load username from authentication file!\n");
        exit(1);
    }
    args->username = buff;

    memset(&buff,0,sizeof(buff));

    retval = fscanf(fd,"password = %s", buff);
    if (retval == 0) {
        fprintf(stderr, "Failed to load password from authentication file!\n");
        exit(1);
    }
    args->password = buff;

    fclose(fd);
}

// Function get the realpath of every file (only file) in directory
std::vector<std::string> list_dir(std::string dirpath) {

    std::vector<std::string> files;
    std::string data;

    struct dirent *entry;
    DIR *dir = opendir(dirpath.c_str());
    if (dir == NULL) {
        return files;
    }
    while ((entry = readdir(dir)) != NULL) {
        data = (dirpath.back() == '/') ? dirpath : dirpath + "/";
        data.append(entry->d_name);
        if (file_exists(data)) {
            data = realpath(data.c_str(), NULL); // TODO FIXIT realpath may return NULL
            files.push_back(data);
        }
    }
    closedir(dir);

    return files;
}

void move_new_to_curr(Args* args) {

    std::ofstream logfile;
    logfile.open(LOG_FILE_NAME, std::fstream::out | std::fstream::app);

    std::vector<std::string> files;
    files = list_dir(args->path_maildir_new);
    if (!files.empty()) {
        for (auto i = files.begin(); i != files.end(); ++i) {
            if (move_file(*i, args->path_maildir_cur)) {
                logfile << *i << std::endl;
            }
        }
    }
    logfile.close();
}

void list_dirfiles_to_file(std::ofstream& file, std::string dir) {
    std::vector<std::string> dir_content;
    dir_content = list_dir(dir);
    if (!dir_content.empty()) {
        for (auto i = dir_content.begin(); i != dir_content.end(); ++i) {
            file << *i << std::endl;
        }
    }
}

// Function checks the structure of maildir
void analyze_maildir(Args* args) {

    args->path_maildir_new = args->path_maildir_cur = args->path_maildir_tmp = args->path_d;

    if (args->path_d.back() == '/') {
        args->path_maildir_new.append("new");
        args->path_maildir_cur.append("cur");
        args->path_maildir_tmp.append("tmp");
    }
    else {
        args->path_maildir_new.append("/new");
        args->path_maildir_cur.append("/cur");
        args->path_maildir_tmp.append("/tmp");
    }

    if (!dir_exists(args->path_maildir_new) || !dir_exists(args->path_maildir_cur) || !dir_exists(args->path_maildir_tmp)) {
        fprintf(stderr, "Wrong folder structure of maildir!\n");
        exit(1);
    }

    return;
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
            else {
                const char* str = args->port.c_str();
                long int number = strtol(str, NULL, 10);
                if ((number < 0) || (PORT_MAX < number) || (errno == ERANGE)) {
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
                load_auth_file(args);
            }
        }

        // check maildir directory existence
        if (args->d) {
            if (!dir_exists(args->path_d)) {
                fprintf(stderr, "Wrong maildir direcotry!\n");
                exit(1);
            }
            else {
                analyze_maildir(args);
            }
        }

        // provide reset
        if (args->r) {
            reset();
        }
    }
}

// TODO FIXIT REMOVE THIS
void print_status(State S, Command C) {

    std::string state, command;

    if (S == AUTHORIZATION) state = "AUTHORIZATION";
    else if (S == TRANSACTION) state = "TRANSACTION";
    else if (S == UPDATE) state = "UPDATE";
    else state = "\033[1m\033[31mSTATE_ERROR\033[0m";

    if (C == QUIT) command = "QUIT";
    else if (C == STAT) command = "STAT";
    else if (C == LIST) command = "LIST";
    else if (C == RETR) command = "RETR";
    else if (C == DELE) command = "DELE";
    else if (C == NOOP) command = "NOOP";
    else if (C == RSET) command = "RSET";
    else if (C == UIDL) command = "UIDL";
    else if (C == USER) command = "USER";
    else if (C == PASS) command = "PASS";
    else if (C == APOP) command = "APOP";
    else command = "\033[1m\033[31mCOMMAND_ERROR\033[0m";

    std::cout << "STATE = " << state << std::endl << "COMMAND = " << command << std::endl;
    return;
}

// Converter STRING to ENUM
Command get_command(std::string& str) {

    if (str.empty()) return COMMAND_ERROR;

    std::string cmd = str;
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

// Parser for string which is in format "COMMAND + ' ' + ARGUMENTS"
void load_cmd_and_args(Command* CMD, std::string& ARGS, std::string& str) {
    std::string cmd;
    std::size_t pos = str.find(" ");
    if (pos == std::string::npos) {
        cmd = str;
    }
    else {
        cmd = str.substr(0, pos);
        ARGS = str.substr(pos+1, str.length());
    }
    *CMD = get_command(cmd);
    return;
}

// Function sends message to client via socket
void thread_send(int socket, std::string& str) {
    // TODO FIXIT socket - what if socket is corrupted?
    send(socket, str.c_str(), str.length(), 0);
    return;
}

// Function creates string for MD5 hash
std::string get_greeting_banner() {

    std::string str;

    pid_t  h_pid  = getpid();
    time_t h_time = time(NULL);

    char h_hostname[HOSTNAME_LENGTH];
    int retval = gethostname(h_hostname, HOSTNAME_LENGTH);
    if(retval != 0) {
        fprintf(stderr, "gethostname() failed!\n");
        return NULL;
    }

    str.append("<");
    str.append(std::to_string(h_pid));
    str.append(".");
    str.append(std::to_string(h_time));
    str.append("@");
    str.append(h_hostname);
    str.append(">");

    return str;
}

#ifdef md5 // =====================================================================================

// Function create a MD5 digest hash
std::string get_md5_hash(std::string& greeting_banner, std::string& password) {

    std::string hash; // digest
    std::string input;

    input.append(greeting_banner);
    input.append(password);

    unsigned char result[32];
    MD5((const unsigned char*)input.c_str(), input.length(), result);

    char buffer[32];
    for (int i=0;i<16;i++){
        sprintf(buffer, "%02x", result[i]);
        hash.append(buffer);
    }

    return hash;
}

// Function provide username and md5 hash validation
int apop_parser(int socket, Args* args, std::string& str, std::string& greeting_banner) {

    std::string username;
    std::string digest;
    std::string msg;
    std::size_t pos;

    pos = str.find(" ");

    if (pos == std::string::npos) {
        msg = "-ERR Command APOP in AUTHORIZATION state failed! Wrong arguments of APOP.\r\n";
        thread_send(socket, msg);
        return 1;
    }
    else {
        username = str.substr(0, pos);
        digest = str.substr(pos+1, str.length());
    }

    if (username.empty()) {
        msg = "-ERR Command APOP in AUTHORIZATION state failed! Wrong arguments of APOP. Username is missing.\r\n";
        thread_send(socket, msg);
        return 1;
    }

    if (digest.empty()) {
        msg = "-ERR Command APOP in AUTHORIZATION state failed! Wrong arguments of APOP. Digest is missing.\r\n";
        thread_send(socket, msg);
        return 1;
    }

    if (args->username.compare(username) != 0) {
        msg = "-ERR Command APOP in AUTHORIZATION state failed! Invalid username.\r\n";
        thread_send(socket, msg);
        return 1;
    }

    std::string hash = get_md5_hash(greeting_banner, args->password);
    if (hash.compare(digest) != 0) {
        msg = "-ERR Command APOP in AUTHORIZATION state failed! Invalid digest.\r\n";
        thread_send(socket, msg);
        return 1;
    }

    return 0;
}

#endif //==========================================================================================

// Passed function to thread which define behaviour of thread
void thread_main(int socket, Args* args) {

    // DECLARATION
    int res;
    int retval;
    char buff[1024];
    bool flag_USER;
    State STATE;
    Command COMMAND;
    std::string msg;
    std::string data;
    std::mutex mutex;
    std::string CMD_ARGS;
    std::string GREETING_BANNER;

    // INITIALIZATION
    STATE = AUTHORIZATION;
    GREETING_BANNER = get_greeting_banner();
    flag_USER = false;

    // sending GREETING BANNER
    msg = "+OK POP3 server ready " + GREETING_BANNER + "\r\n";
    thread_send(socket, msg);

    while(1) {

        if (flag_exit) {
            // TODO send message to client about killing server
            close(socket);
            return;
        }

        // RESET
        memset(&buff,0,sizeof(buff));
        CMD_ARGS.clear();
        data.clear();

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

        /*
        if (strlen(buff) < 2) continue; // 0-1 chars received
        else { // 2-N chars received
            if (strcmp("\r\n",buff) == 0) continue; // only CRLF received
            else {
                data = buff;
                if (data.substr(data.length()-2) != "\r\n") { // data without CRLF received
                    ; // TODO
                }
            }
        }
        */

        if (strcmp("\r\n",buff) == 0) continue; // only CRLF received

        if (data.substr(data.length()-2) != "\r\n")

        // ADD BUFFER TO C++ STIRING
        data = buff;
        data = data.substr(0, data.size()-2); // remmove CRLF (last 2 characters)

        if (data.empty()) continue; // we dont have string including "COMMAND [ARGS]"

        load_cmd_and_args(&COMMAND, CMD_ARGS, data);

        print_status(STATE,COMMAND); // TODO: remove this + function definition

        switch(STATE){
            // ==========================================================
            case AUTHORIZATION:
                switch(COMMAND){
                    // ==================================================
                    case NOOP:
                        if (!CMD_ARGS.empty()) { // NOOP str
                            msg = "-ERR Command NOOP in AUTHORIZATION state does not support any arguments!\r\n";
                            thread_send(socket, msg);
                        }
                        else { // NOOP
                            msg = "+OK\r\n";
                            thread_send(socket, msg);
                        }
                        break;
                    // ==================================================
                    case QUIT:
                        if (!CMD_ARGS.empty()) { // QUIT str
                            msg = "-ERR Command QUIT in AUTHORIZATION state does not support any arguments!\r\n";
                            thread_send(socket, msg);
                        }
                        else { // QUIT
                            msg = "+OK Closing connection.\r\n";
                            thread_send(socket, msg);
                            close(socket);
                            return;
                        }
                        break;
                    // ==================================================
                    case USER:
                        if (args->c) { // USER command is supported
                            if (!CMD_ARGS.empty()) { // USER str
                                if (args->username.compare(CMD_ARGS) == 0 ) { // USER USERNAME
                                    flag_USER = true;
                                    msg = "+OK Userame is valid.\r\n";
                                    thread_send(socket, msg);
                                }
                                else { // USER wrongstr
                                    msg = "-ERR Invalid username!\r\n";
                                    thread_send(socket, msg);
                                }
                            }
                            else { // USER
                                msg = "-ERR Command USER in AUTHORIZATION state has no argument!\r\n";
                                thread_send(socket, msg);
                            }
                        }
                        else { // USER command is NOT supported
                            msg = "-ERR Command USER in AUTHORIZATION state is not supported!\r\n";
                            thread_send(socket, msg);
                        }
                        break;
                    // ==================================================
                    case PASS:
                        if (args->c) { // PASS command is supported
                            if (flag_USER) { // USERNAME was
                                if (!CMD_ARGS.empty()) { // PASS str
                                    if (args->password.compare(CMD_ARGS) == 0 ) { // PASS PASSWORD

                                        msg = "+OK Password is valid. Authorized.\r\n";
                                        thread_send(socket, msg);

                                        if (!mutex.try_lock()) {
                                            msg = "-ERR Maildrop cannot be opened because of locked mutex!\r\n";
                                            thread_send(socket, msg);
                                        }

                                        STATE = TRANSACTION;

                                    }
                                    else { // PASS wrongstr
                                        msg = "-ERR Invalid password!\r\n";
                                        thread_send(socket, msg);
                                    }
                                }
                                else { // PASS
                                    msg = "-ERR Command PASS in AUTHORIZATION state has no argument!\r\n";
                                    thread_send(socket, msg);
                                }
                            }
                            else { // USERNAME was not
                                msg = "-ERR Missing command USER in AUTHORIZATION before PASS!\r\n";
                                thread_send(socket, msg);
                            }
                        }
                        else { // PASS command is NOT supported
                            msg = "-ERR Command PASS in AUTHORIZATION state is not supported!\r\n";
                            thread_send(socket, msg);
                        }
                        break;
                    // ==================================================
                    case APOP:
                        if (CMD_ARGS.empty()) { // APOP
                            msg = "-ERR Command APOP in AUTHORIZATION state has no arguments!\r\n";
                            thread_send(socket, msg);
                        }
                        else { // APOP str

                            #ifdef md5

                                retval = apop_parser(socket, args, CMD_ARGS, GREETING_BANNER);
                                if (retval == 1) {
                                    break;
                                }
                                msg = "+OK Authorized.\r\n";
                                thread_send(socket, msg);

                                if (!mutex.try_lock()) {
                                    msg = "-ERR Maildrop cannot be opened because of locked mutex!\r\n";
                                    thread_send(socket, msg);
                                }

                                STATE = TRANSACTION;

                            #else // REMOVE THIS ########################################

                                (void)retval; // -Wunused-variable
                                msg = "+OK Authorized. MD5 is switched OFF\r\n";
                                thread_send(socket, msg);

                                if (!mutex.try_lock()) {
                                    msg = "-ERR Maildrop cannot be opened because of locked mutex!\r\n";
                                    thread_send(socket, msg);
                                }

                                STATE = TRANSACTION;

                            #endif // ###################################################
                        }
                        break;
                    // ==================================================
                    default:
                        msg = "-ERR Wrong command in AUTHORIZATION state!\r\n";
                        thread_send(socket, msg);
                        break;
                }
                break;
            // ==========================================================
            case TRANSACTION:

                move_new_to_curr(args);

                switch(COMMAND){
                    // ==================================================
                    case NOOP:
                        if (!CMD_ARGS.empty()) { // NOOP str
                            msg = "-ERR Command NOOP in AUTHORIZATION state does not support any arguments!\r\n";
                            thread_send(socket, msg);
                        }
                        else { // NOOP
                            msg = "+OK\r\n";
                            thread_send(socket, msg);
                        }
                        break;
                    // ==================================================
                    case QUIT:
                        if (!CMD_ARGS.empty()) { // QUIT str
                            msg = "-ERR Command QUIT in TRANSACTION state does not support any arguments!\r\n";
                            thread_send(socket, msg);
                        }
                        else { // QUIT
                            msg = "+OK\r\n";
                            thread_send(socket, msg);
                            STATE = UPDATE;
                        }
                        break;
                    // ==================================================
                    case UIDL:
                        break;
                    // ==================================================
                    case STAT:
                        break;
                    // ==================================================
                    case LIST: // LIST
                        // TODO
                        if (!CMD_ARGS.empty()) {
                            msg = "+OK LIST\r\n";
                            thread_send(socket, msg);
                        }
                        else { // LIST str
                            msg = "+OK LIST [str]\r\n";
                            thread_send(socket, msg);
                        }
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
                        msg = "-ERR Wrong command in TRANSACTION state!\r\n";
                        thread_send(socket, msg);
                        break;
                }
                break;
            // ==========================================================
            case UPDATE:
                // TODO
                close(socket);
                mutex.unlock();
                return;
                break;
            // ==========================================================
            default: // never happen, DONT TOUCH
                break;
        }
    }
}

// Function is the kernel of this program, set up connection and threads
void server_kernel(Args* args) {

    reset();

    int retval;
    int flags;
    int port_number = atoi((args->port).c_str());
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

    memset(&sa,0,sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_addr = in6addr_any;
    sa.sin6_port = htons(port_number);

    // bind a name to a socket
    retval = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa));
    if (retval < 0) {
        fprintf(stderr, "bind() failed!\n");
        exit(1);
    }

    // listen for connections on a socket
    retval = listen(welcome_socket, 2);
    if (retval < 0) {
        fprintf(stderr, "listen() failed!\n");
        exit(1);
    }

    while(1) {

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(welcome_socket, &fds);

        // setting up select which will wait until any I/O operation can be performed
        if (select(welcome_socket + 1, &fds, NULL, NULL, NULL) == -1) {
            fprintf(stderr, "select() failed!\n");
            continue;
        }

        // accept a connection on a socket
        communication_socket = accept(welcome_socket, (struct sockaddr*)&sa_client, &sa_client_len);
        if (communication_socket == -1) { // TODO FIXIT - what to do if accept fails ??? WHAT? WHAT IS THIS ?
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

        std::thread thrd(thread_main, communication_socket, args);
        thrd.detach();
    }

    return;
}

// The Main
int main(int argc, char* argv[]) {

    signal(SIGINT, signal_handler);

    Args args;
    argpar(&argc, argv, &args);

    server_kernel(&args);

    return 0;
}
