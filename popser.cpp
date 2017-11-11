/**
 * @author     Adrián Tóth
 * @date       28.10.2017
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

// DEBUG
#define PRINT_VEC(vector) for (auto i = vector.begin(); i != vector.end(); ++i) std::cout << *i << std::endl; std::cout << std::endl;
#define PRINT(data) std::cout << data << std::endl << std::endl;
#define ECHO() std::cout << std::endl;

// CONSTANTS
#define PORT_MAX 65535
#define HOSTNAME_LENGTH 64
#define TIMEOUT_SECONDS 600 // 10 minutes
#define LOG_FILE_NAME "log"
#define DATA_FILE_NAME "data"
#define DATA_FILE_DELIMITER "/"
#define ID_LENGTH 20
#define ID_CHARS "!\"#$%&'()*+,-." /* excluding SLASH */   "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" // SLASH is used many times as a delimiter

// GLOBAL VARIABLES
bool flag_exit = false;
std::mutex mutex_maildir;

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

// Print the help message to stdout and terminate the program
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
        "\n";

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

// Function move file to specified directory
bool move_file(std::string filepath, std::string dirpath) {

    if (filepath.empty() || dirpath.empty()) {
        return false;
    }

    std::string filename, newfilepath;

    filename = filepath.substr(filepath.find_last_of("/")+1, std::string::npos);
    newfilepath = (dirpath.back() == '/') ? dirpath + filename : dirpath + "/" + filename;

    int retval = rename(filepath.c_str(), newfilepath.c_str());
    if (retval != 0) {
        return false;
    }

    return true;
}

// Function generate a random server-determined string for message ID
std::string id_generator() {

    std::string str = "";
    char alphanum[] = ID_CHARS;
    srand(std::clock()+time(NULL));

    for (unsigned int i = 0; i < ID_LENGTH; ++i) {
        str += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return str;
}

// Function get the file size in octets
std::string file_size(std::string filename) {

    long long unsigned int size = 0;

    // file ends with \n
    bool ENDS_WITH_NEWLINE = false;
    std::ifstream in(filename);
    std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (str.back() == '\n') ENDS_WITH_NEWLINE = true;
    
    // file size
    std::ifstream infile(filename, std::ifstream::ate | std::ifstream::binary);
    size = infile.tellg();

    std::string line;
    std::ifstream file(filename);
    std::string buff;
    while(std::getline(file, line)) {
        if (line.back() != '\r') {
            size++;
        }
    }

    // file does not end with \n
    if (!ENDS_WITH_NEWLINE) {
        size++;
    }

    return std::to_string(size);
}

// Function return DATA-LINE specified by filename from DATA_FILE_NAME
std::string get_filename_line_from_data(std::string filename) {
    std::ifstream data(DATA_FILE_NAME);
    std::string line, line_filename;
    while(std::getline(data, line)) {
        line_filename = line.substr(0, line.find("/"));
        if (filename.compare(line_filename) == 0) {
            return line;
        }
    }
    data.close();
    return ""; // filename not found in file DATA_FILE_NAME
}

// Function loads the whole file content into std::string
std::string get_file_content(std::string filepath) {

    std::ifstream file(filepath);

    std::string str;
    std::string line;

    while (std::getline(file, line)) {

        // add DOT before DOT => line starts with dot so add a dot before that dot
        if (line.front() == '.')
            line.insert(0, ".");

        // every line must be terminated with CRLF
        if (line.back() != '\r')
            line.append("\r");

        line.append("\n"); // getline() remove \n when it is reading from file

        // line is handled and has \r\n at the end
        str.append(line);
    }

    return str;
}

// Function load file content line by line as std::string to a std::vector
std::vector<std::string> load_file_lines_to_vector(std::string file) {

    std::vector<std::string> files;

    if (!file_exists(file)) {
        return files; // empty vector
    }

    std::ifstream logfile(file);
    std::string line;

    while (std::getline(logfile, line)) {
        files.push_back(line);
    }

    logfile.close();

    return files;
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

// Function provide addition of filesizes which are saved in vector (vector = loaded DATA file)
unsigned int get_size_summary(std::vector<std::string>& data) {
    unsigned int octets = 0;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            octets += std::stoi((*i).substr((*i).find_last_of("/")+1, std::string::npos));
        }
    }
    return octets;
}

// Function return file size from vector (vector = loaded DATA file)
unsigned int get_file_size(std::string& filename, std::vector<std::string>& data) {
    unsigned int size = 0;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            if (filename.compare((*i).substr(0, (*i).find("/"))) == 0) {
                size = std::stoi((*i).substr((*i).find_last_of("/")+1, std::string::npos));
            }
        }
    }
    return size;
}

// Function return file id from vector (vector = loaded DATA file)
std::string get_file_id(std::string& filename, std::vector<std::string>& data) {
    std::string id;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            if (filename.compare((*i).substr(0, (*i).find("/"))) == 0) {
                id = (*i).substr((*i).find("/")+1, (*i).find_last_of("/"));
            }
        }
    }
    return id;
}

// Function return file names from vector (vector = loaded DATA file)
std::vector<std::string> get_files(std::vector<std::string>& data) {
    std::vector<std::string> files;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            files.push_back((*i).substr(0, (*i).find("/")));
        }
    }
    return files;
}

// Function return vector size (count of messages which are not marked as deleted)
unsigned int vector_size(std::vector<std::string>& data) {
    unsigned int counter = 0;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            counter++;
        }
    }
    return counter;
}

void remove_file(std::string& filename, Args* args) {

    std::string content_line_filename;
    std::vector<std::string> vector;
    std::vector<std::string> content;
    std::ofstream output_file;

    //remove from file DATA_FILE_NAME
    content = load_file_lines_to_vector(DATA_FILE_NAME);
    for (auto i = content.begin(); i != content.end(); ++i) {
        content_line_filename = (*i).substr(0, (*i).find("/"));
        if (filename.compare(content_line_filename) != 0) {
            vector.push_back(*i);
        }
    }
    output_file.open(DATA_FILE_NAME);
    for (auto i = vector.begin(); i != vector.end(); ++i) {
        output_file << *i << std::endl;
    }

    //remove from file LOG_FILE_NAME
    content = load_file_lines_to_vector(LOG_FILE_NAME);
    for (auto i = content.begin(); i != content.end(); ++i) {
        content_line_filename = (*i).substr((*i).find_last_of("/")+1, std::string::npos);
        if (filename.compare(content_line_filename) != 0) {
            vector.push_back(*i);
        }
    }
    output_file.open(LOG_FILE_NAME);
    for (auto i = vector.begin(); i != vector.end(); ++i) {
        output_file << *i << std::endl;
    }

    //remove physically maildir/cur/filename
    std::string filepath;
    filepath = (args->path_maildir_cur.back() == '/') ? args->path_maildir_cur + filename : args->path_maildir_cur + "/" + filename;
    remove(filepath.c_str());
}

// Function get the realpath of every file (only file) in directory
std::vector<std::string> get_file_paths_in_directory(std::string dirpath) {

    std::vector<std::string> files;

    struct dirent *entry;
    DIR *dir = opendir(dirpath.c_str());
    if (dir == NULL) { // check directory
        return files;
    }

    std::string data;

    while ((entry = readdir(dir)) != NULL) { // read files from directory
        data = (dirpath.back() == '/') ? dirpath + entry->d_name : dirpath + "/" + entry->d_name; // create relative path
        if (file_exists(data)) { // check file
            data = realpath(data.c_str(), NULL); // create absolute path
            if (!data.empty()) { // file is alright
                files.push_back(data);
            }
        }
    }
    closedir(dir);

    return files;
}

// Function move content of maildir/new to maildir/curr
void move_new_to_curr(Args* args) {

    //##########################################
    // ABSOLUTE PATHS OF ALL FILES in maildir/NEW
    //##########################################
    std::vector<std::string> files;
    files = get_file_paths_in_directory(args->path_maildir_new); // returns ABSOLUTE paths of every files in directory

    if (files.empty()) return; // 0 files in maildir/NEW



    //##########################################
    // CREATE DATA FILE
    //##########################################
    std::ofstream datafile;
    datafile.open(DATA_FILE_NAME, std::fstream::out | std::fstream::app);

    std::string f_name;
    std::string f_id;
    std::string f_size;

    for (auto i = files.begin(); i != files.end(); ++i) {

        f_name = (*i).substr((*i).find_last_of("/")+1, std::string::npos); // filename
        f_id   = id_generator();                                           // file unique-id
        f_size = file_size(*i);                                            // file size

        // [filename][/][unique-id][/][size]["\n"]
        datafile << f_name << DATA_FILE_DELIMITER << f_id << DATA_FILE_DELIMITER << f_size << std::endl;
    }

    datafile.close();



    //##########################################
    // CREATE LOG FILE
    //##########################################
    std::ofstream logfile;
    logfile.open(LOG_FILE_NAME, std::fstream::out | std::fstream::app);

    for (auto i = files.begin(); i != files.end(); ++i) {
        if (move_file(*i, args->path_maildir_cur)) {
            // [file absolute path][\n]
            logfile << *i << std::endl;
        }
    }

    logfile.close();



    return;
}

// Reset
void reset() {

    if (file_exists(DATA_FILE_NAME)) { // remove DATA_FILE_NAME
        remove(DATA_FILE_NAME);
    }

    if (file_exists(LOG_FILE_NAME)) { // remove LOG_FILE_NAME

        std::vector<std::string> files;
        files = load_file_lines_to_vector(LOG_FILE_NAME); // load LOG_FILE_NAME

        std::string buff;
        std::string path_cur;
        std::string path_new;

        // calculation of maildir path
        buff = files.front();
        buff = buff.substr(0, buff.find_last_of("/"));
        buff = buff.substr(0, buff.find_last_of("/")+1);

        path_cur = buff + "cur"; // maildir/cur path
        path_new = buff + "new"; // maildir/new path

        std::string filename;
        std::string file_in_new;
        std::string file_in_cur;

        for (auto i = files.begin(); i != files.end(); ++i) {
            file_in_new = *i; // absolute path of file in NEW
            filename = file_in_new.substr(file_in_new.find_last_of("/")+1, std::string::npos); // filename
            file_in_cur = path_cur + "/" + filename; // absolute path of file in CUR
            if (file_exists(file_in_cur)) { // file exists in CUR
                move_file(file_in_cur, path_new); // move file from CUR to NEW
            }
        }

        remove(LOG_FILE_NAME);
    }

    return;
}

// Function checks the structure of maildir
void check_maildir(Args* args) {

    args->path_maildir_new = args->path_maildir_cur = args->path_maildir_tmp = args->path_d; // assign path of maildir

    if (args->path_d.back() == '/') { // maildir ends with SLASH
        args->path_maildir_new.append("new");
        args->path_maildir_cur.append("cur");
        args->path_maildir_tmp.append("tmp");
    }
    else { // maildir does not end with SLASH
        args->path_maildir_new.append("/new");
        args->path_maildir_cur.append("/cur");
        args->path_maildir_tmp.append("/tmp");
    }

    // NEW, CUR, TMP exists in maildir
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
            // TODO check if param -d has string
            if (!dir_exists(args->path_d)) {
                fprintf(stderr, "Wrong maildir direcotry!\n");
                exit(1);
            }
            else {
                check_maildir(args);
            }
        }

        // provide reset
        if (args->r) {
            reset();
        }
    }
}

// Converter STRING to ENUM
Command get_command(std::string& str) {

    if (str.empty()) return COMMAND_ERROR;

    std::string cmd = str;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower); // TO LOWERCASE

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
    std::size_t pos = str.find(" "); // get FIRST space position
    if (pos == std::string::npos) { // no space in string => 1 command
        cmd = str;
    }
    else { // 1 space in string => 1 command & 1 argument
        cmd = str.substr(0, pos); // commnad
        ARGS = str.substr(pos+1, str.length()); // argument
    }
    *CMD = get_command(cmd); // enum of the command
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

    pid_t  h_pid  = getpid();   // PID
    time_t h_time = time(NULL); // TIME

    char h_hostname[HOSTNAME_LENGTH];
    int retval = gethostname(h_hostname, HOSTNAME_LENGTH); // HOSTNAME
    if(retval != 0) {
        fprintf(stderr, "gethostname() failed!\n"); // TODO FIXIT nic nepisat na stderr
        return NULL;
    }

    // [<][PID][.][TIME][@][HOSTNAME][>]
    str = "<" + std::to_string(h_pid) + "." + std::to_string(h_time) + "@" + h_hostname + ">";

    return str;
}

#ifdef md5 // =====================================================================================

// Function create a MD5 digest hash
std::string get_md5_hash(std::string& greeting_banner, std::string& password) {

    std::string hash; // digest
    std::string input;

    // [<][PID][.][TIME][@][HOSTNAME][>][password]
    input = greeting_banner + password;

    unsigned char result[32];
    MD5((const unsigned char*)input.c_str(), input.length(), result); // create MD5

    // create a STD::STRING from MD5 hash
    char buffer[32];
    for (int i=0;i<16;i++){
        sprintf(buffer, "%02x", result[i]);
        hash.append(buffer);
    }

    return hash; // digest
}

// Function provide username and md5 hash validation
int apop_parser(int socket, Args* args, std::string& str, std::string& greeting_banner) { // TODO comment

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
        msg = "-ERR Authentication failed!\r\n"; // wrong username
        thread_send(socket, msg);
        return 1;
    }

    std::string hash = get_md5_hash(greeting_banner, args->password);
    if (hash.compare(digest) != 0) {
        msg = "-ERR Authentication failed!\r\n"; // wrong digest
        thread_send(socket, msg);
        return 1;
    }

    return 0;
}

#endif //==========================================================================================

// Passed function to thread which define behavior of thread
void thread_main(int socket, Args* args) {

    // DECLARATION
    int res;
    int retval;
    char buff[1024];
    bool flag_USER;
    bool flag_WRONGUSER;
    time_t time_from;
    time_t time_curr;
    State STATE;
    Command COMMAND;
    std::string msg;
    std::string data;
    std::string CMD_ARGS;
    std::string GREETING_BANNER;
    std::vector<std::string> WORKING_VECTOR;
    std::vector<std::string> FILENAMES_TO_REMOVE;

    // INITIALIZATION
    STATE = AUTHORIZATION;
    GREETING_BANNER = get_greeting_banner();
    flag_USER = false;
    flag_WRONGUSER = false;
    time_from = time(NULL);

    // sending GREETING BANNER
    msg = "+OK POP3 server ready " + GREETING_BANNER + "\r\n";
    thread_send(socket, msg);

    while(1) {

        if (STATE == UPDATE) {
            if (FILENAMES_TO_REMOVE.size() != 0) {
                for (auto i = FILENAMES_TO_REMOVE.begin(); i != FILENAMES_TO_REMOVE.end(); ++i) {
                    remove_file(*i, args);
                }
            }
            // TODO send message ?
            close(socket);
            mutex_maildir.unlock();
            return;
        }

        // RESET
        memset(&buff,0,sizeof(buff));
        CMD_ARGS.clear();
        data.clear();
        msg.clear();
        time_curr = time(NULL);


        // RECEIVE
        res = recv(socket, buff, 1024,0); // FIXIT TODO buffer size
        if (flag_exit) { // SIGINT
            close(socket);
            return;
        }
        else if ( TIMEOUT_SECONDS <= (time_curr - time_from)) { // TIMEOUT
            msg = "Timeout expired! You were AFK for "+std::to_string(time_curr - time_from)+" seconds.\r\n";
            thread_send(socket,msg);
            close(socket);
            return;
        }
        else if (res > 0) { // DATA INCOME
            time_from = time(NULL);
        }
        else if (res == 0) { // client disconnected
            close(socket);
            return; // kill thread
        }
        else if (errno == EAGAIN) { // recv would block EWOULDBLOCK
            continue;
        }
        else {
            fprintf(stderr, "recv() failed!\n");
            return;
        }

        /*
        // TODO - we have to check this ?
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

        data = buff;
        data = data.substr(0, data.size()-2); // remmove CRLF (last 2 characters)

        if (data.empty()) continue; // we dont have string including "COMMAND [ARGS]"

        load_cmd_and_args(&COMMAND, CMD_ARGS, data);

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
                            return; // kill thread
                        }
                        break;
                    // ==================================================
                    case USER:
                        if (args->c) { // USER command is supported
                            if (!CMD_ARGS.empty()) { // USER str
                                flag_USER = true;
                                flag_WRONGUSER = (args->username.compare(CMD_ARGS) == 0) ? false : true;
                                msg = "+OK Username received.\r\n";
                                thread_send(socket, msg);
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
                            if (flag_USER) { // USERNAME received
                                flag_USER = false;
                                if (!CMD_ARGS.empty()) { // PASS str
                                    if (!flag_WRONGUSER) {
                                        if (args->password.compare(CMD_ARGS) == 0 ) { // PASS PASSWORD
                                            msg = "+OK Logged in.\r\n";
                                            thread_send(socket, msg);
                                            if (!mutex_maildir.try_lock()) {
                                                // RFC: If the maildrop cannot be opened for some reason, the POP3 server responds with a negative status indicator.
                                                msg = "-ERR Maildrop is locked by someone else!\r\n";
                                                thread_send(socket, msg);
                                                // RFC: After returning a negative status indicator, the server may close the connection.
                                                close(socket);
                                                return; // kill thread
                                            }
                                            // TODO check maildir
                                            STATE = TRANSACTION;
                                            move_new_to_curr(args);
                                            WORKING_VECTOR = load_file_lines_to_vector(DATA_FILE_NAME);
                                        }
                                        else { // Wrong password
                                            msg = "-ERR Authentication failed!\r\n";
                                            thread_send(socket, msg);
                                        }
                                    }
                                    else { // Wrong username
                                        msg = "-ERR Authentication failed!\r\n";
                                        thread_send(socket, msg);
                                    }
                                }
                                else { // PASS
                                    msg = "-ERR Command PASS in AUTHORIZATION state has no argument!\r\n";
                                    thread_send(socket, msg);
                                }
                            }
                            else { // USERNAME missing
                                msg = "-ERR No username given!\r\n";
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
                        if (!args->c) { // APOP command is supported
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
                                    msg = "+OK Logged in.\r\n";
                                    thread_send(socket, msg);

                                    if (!mutex_maildir.try_lock()) {
                                        // RFC: If the maildrop cannot be opened for some reason, the POP3 server responds with a negative status indicator.
                                        msg = "-ERR Maildrop is locked by someone else!\r\n";
                                        thread_send(socket, msg);
                                        // RFC: After returning a negative status indicator, the server may close the connection.
                                        close(socket);
                                        return; // kill thread
                                    }

                                    // TODO check maildir
                                    STATE = TRANSACTION;
                                    move_new_to_curr(args);
                                    WORKING_VECTOR = load_file_lines_to_vector(DATA_FILE_NAME);

                                #else // REMOVE THIS ########################################

                                    (void)retval; // -Wunused-variable
                                    msg = "+OK Logged in.\r\n";
                                    thread_send(socket, msg);

                                    if (!mutex_maildir.try_lock()) {
                                        // RFC: If the maildrop cannot be opened for some reason, the POP3 server responds with a negative status indicator.
                                        msg = "-ERR Maildrop is locked by someone else!\r\n";
                                        thread_send(socket, msg);
                                        // RFC: After returning a negative status indicator, the server may close the connection.
                                        close(socket);
                                        return; // kill thread
                                    }

                                    // TODO check maildir
                                    STATE = TRANSACTION;
                                    move_new_to_curr(args);
                                    WORKING_VECTOR = load_file_lines_to_vector(DATA_FILE_NAME);

                                #endif // ###################################################
                            }
                        }
                        else { // APOP command is NOT supported
                            msg = "-ERR Command APOP in AUTHORIZATION state is not supported!\r\n";
                            thread_send(socket, msg);
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
                        if (CMD_ARGS.empty()) { // UIDL

                            msg = "+OK\r\n";
                            thread_send(socket, msg);

                            unsigned int index = 0;
                            std::string filename;

                            if (vector_size(WORKING_VECTOR) > 0) { // WORKING_VECTOR is empty
                                for (auto i = WORKING_VECTOR.begin(); i != WORKING_VECTOR.end(); ++i) {
                                    index++;
                                    if ((*i).compare("") != 0) {
                                        filename = (*i).substr(0, (*i).find("/"));
                                        msg = std::to_string(index) + " " + get_file_id(filename, WORKING_VECTOR) + "\r\n";
                                        thread_send(socket, msg);
                                    }
                                }
                            }

                            msg = ".\r\n";
                            thread_send(socket, msg);
                        }
                        else { // UIDL str
                            if (!is_number(CMD_ARGS.c_str())) {
                                msg = "-ERR Command UIDL in state TRANSACTION needs a mumerical argument (index)!\r\n"; // TODO
                                thread_send(socket, msg);
                                break;
                            }
                            unsigned int index = std::stoi(CMD_ARGS);
                            unsigned int WV_size = WORKING_VECTOR.size();
                            if (0 < index && index <= WV_size) {
                                std::string filename = WORKING_VECTOR[index-1];
                                if (filename.empty()) {
                                    msg = "-ERR Message is already deleted!\r\n"; // TODO
                                    thread_send(socket, msg);
                                }
                                else {
                                    filename = filename.substr(0, filename.find("/"));
                                    msg = "+OK " + std::to_string(index) + " " + get_file_id(filename, WORKING_VECTOR) + "\r\n";
                                    thread_send(socket, msg);
                                }
                            }
                            else {
                                msg = "-ERR Out of range indexing in messages via \"UIDL <index>\"!\r\n"; // TODO
                                thread_send(socket, msg);
                            }

                        }
                        break;
                    // ==================================================
                    case STAT:
                        if (CMD_ARGS.empty()) {
                            msg = "+OK " + std::to_string(vector_size(WORKING_VECTOR)) + " " + std::to_string(get_size_summary(WORKING_VECTOR)) + "\r\n";
                            thread_send(socket, msg);
                        }
                        else {
                            msg = "-ERR Command STAT in TRANSACTION state does not support any arguments!\r\n";
                            thread_send(socket, msg);
                        }
                        break;
                    // ==================================================
                    case LIST:
                        if (CMD_ARGS.empty()) { // LIST
                            unsigned int count;
                            unsigned int octets;

                            count = vector_size(WORKING_VECTOR);
                            octets = get_size_summary(WORKING_VECTOR);

                            msg = "+OK " + std::to_string(count) + " messages (" + std::to_string(octets) + " octets)\r\n";
                            thread_send(socket, msg);

                            unsigned int index = 0;
                            std::string filename;

                            if (vector_size(WORKING_VECTOR) > 0) { // WORKING_VECTOR is empty
                                for (auto i = WORKING_VECTOR.begin(); i != WORKING_VECTOR.end(); ++i) {
                                    index++;
                                    if ((*i).compare("") != 0) {
                                        filename = (*i).substr(0, (*i).find("/"));
                                        msg = std::to_string(index) + " " + std::to_string(get_file_size(filename, WORKING_VECTOR)) + "\r\n";
                                        thread_send(socket, msg);
                                    }
                                }
                            }

                            msg = ".\r\n";
                            thread_send(socket, msg);
                        }
                        else { // LIST str
                            if (!is_number(CMD_ARGS.c_str())) {
                                msg = "-ERR Command LIST in state TRANSACTION needs a numerical argument (index)!\r\n"; // TODO
                                thread_send(socket, msg);
                                break;
                            }
                            unsigned int index = std::stoi(CMD_ARGS);
                            unsigned int WV_size = WORKING_VECTOR.size();
                            if (0 < index && index <= WV_size) {
                                std::string filename = WORKING_VECTOR[index-1];
                                if (filename.empty()) {
                                    msg = "-ERR Message is already deleted!\r\n"; // TODO
                                    thread_send(socket, msg);
                                }
                                else {
                                    filename = filename.substr(0, filename.find("/"));
                                    msg = "+OK " + std::to_string(index) + " " + std::to_string(get_file_size(filename, WORKING_VECTOR)) + "\r\n";
                                    thread_send(socket, msg);
                                }
                            }
                            else {
                                msg = "-ERR Out of range indexing in messages via \"LIST <index>\"!\r\n"; // TODO
                                thread_send(socket, msg);
                            }

                        }
                        break;
                    // ==================================================
                    case RETR:
                        if (!CMD_ARGS.empty()) { // RETR str

                            if (!is_number(CMD_ARGS.c_str())) {
                                msg = "-ERR Command RETR in state TRANSACTION needs a numerical argument (index)!\r\n"; // TODO
                                thread_send(socket, msg);
                                break;
                            }

                            unsigned int index = std::stoi(CMD_ARGS);
                            unsigned int WV_size = WORKING_VECTOR.size();

                            if (0 < index && index <= WV_size) {
                                std::string filename = WORKING_VECTOR[index-1];
                                if (filename.empty()) {
                                    msg = "-ERR Message is already deleted!\r\n"; // TODO
                                    thread_send(socket, msg);
                                }
                                else {
                                    filename = filename.substr(0, filename.find("/"));

                                    msg = "+OK " + std::to_string(get_file_size(filename, WORKING_VECTOR)) + " octets\r\n";
                                    thread_send(socket, msg);

                                    std::string filepath = (args->path_maildir_cur.back() == '/') ? args->path_maildir_cur + filename :args->path_maildir_cur + "/" + filename;
                                    msg = get_file_content(filepath);
                                    msg += ".\r\n";
                                    thread_send(socket, msg);
                                }
                            }
                            else {
                                msg = "-ERR Out of range indexing in messages via \"RETR <index>\"!\r\n";
                                thread_send(socket, msg);
                            }
                        }
                        else { // RETR
                            msg = "-ERR Command RETR in TRANSACTION state needs a argument!\r\n";
                            thread_send(socket, msg);
                        }
                        break;
                    // ==================================================
                    case DELE:
                        if (!CMD_ARGS.empty()) { // DELE str
                            if (!is_number(CMD_ARGS.c_str())) {
                                msg = "-ERR Command DELE in state TRANSACTION needs a mumerical argument (index)!\r\n"; // TODO
                                thread_send(socket, msg);
                                break;
                            }
                            unsigned int index = std::stoi(CMD_ARGS);
                            unsigned int WV_size = WORKING_VECTOR.size();
                            if (index <= WV_size) {
                                std::string filename = WORKING_VECTOR[index-1];
                                if (filename.empty()) {
                                    msg = "-ERR Message is already deleted!\r\n"; // TODO
                                    thread_send(socket, msg);
                                }
                                else {
                                    filename = filename.substr(0, filename.find("/"));

                                    // add filename to FILENAMES_TO_REMOVE (used in STATE==UPDATE)
                                    FILENAMES_TO_REMOVE.push_back(filename);

                                    // remove filename from WORKING_VECTOR (replace it with empty string)
                                    std::string vector_filename;
                                    for (auto i = WORKING_VECTOR.begin(); i != WORKING_VECTOR.end(); ++i) {
                                        vector_filename = (*i).substr(0, (*i).find("/"));
                                        if (filename.compare(vector_filename) == 0) {
                                            *i = "";
                                        }
                                    }
                                }
                            }
                            else {
                                msg = "-ERR Out of range indexing in messages via \"DELE <index>\"!\r\n"; // TODO
                                thread_send(socket, msg);
                            }
                        }
                        else {
                            msg = "-ERR Command DELE in state TRANSACTION needs a argument!\r\n";
                            thread_send(socket, msg);
                        }
                        break;
                    // ==================================================
                    case RSET:
                        if (CMD_ARGS.empty()) { // RSET
                            std::string rset_tmp;
                            for (auto FTR_i = FILENAMES_TO_REMOVE.begin(); FTR_i != FILENAMES_TO_REMOVE.end(); ++FTR_i) {
                                for (auto WV_i = WORKING_VECTOR.begin(); WV_i != WORKING_VECTOR.end(); ++WV_i) {
                                    if ((*WV_i).empty()) {
                                        rset_tmp = get_filename_line_from_data(*FTR_i);
                                        *WV_i = rset_tmp;
                                    }
                                }
                            }
                            FILENAMES_TO_REMOVE.clear();
                            msg = "+OK\r\n";
                            thread_send(socket, msg);
                        }
                        else { // RSET str
                            msg = "-ERR Command RSET in state TRANSACTION does not support any arguments!\r\n";
                            thread_send(socket, msg);
                        }
                        break;
                    // ==================================================
                    default:
                        msg = "-ERR Wrong command in TRANSACTION state!\r\n";
                        thread_send(socket, msg);
                        break;
                }
                break;
            // ==========================================================
            default: // never happen, DONT TOUCH
                break;
        }
    }
}

// Function is the kernel of this program, set up connection and threads
void server_kernel(Args* args) {

    if (args->r) {
        reset();
    }

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
