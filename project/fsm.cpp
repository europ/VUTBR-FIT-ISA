#include "fsm.hpp"

//#include <iostream>
//#include <cstdio>

#include <time.h> // time()

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <csignal>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>

#include "md5.hpp"
#include "argpar.hpp"
#include "checks.hpp"
#include "logger.hpp"
#include "constants.hpp"
#include "datatypes.hpp"

// if send failed close socket, open mutex if locked, kill thread => do a "return" in thread_main (macro used only in thread_main)
#define TSEND(s,m)                       \
    if (!thread_send(s,m)) {             \
        close(socket);                   \
        if (flag_mutex) {                \
            mutex_maildir.unlock();      \
            flag_mutex = false;          \
        }                                \
        return;                          \
    }

// GLOBAL VARIABLES defined in popser.cpp
extern bool flag_exit;
extern bool flag_mutex;
extern std::mutex mutex_maildir;

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
bool thread_send(int socket, std::string& str) {

    long long int char_count = str.length();
    long long int char_sent  = 0;

    long long int retval;

    while(true) {

        retval = send(socket, str.c_str(), str.length(), 0); // send message

        if (retval > 0) {
            char_sent += retval;
            str = str.substr(retval, str.length()-retval);
        }

        // TODO remove this
        //std::cout << "CHAR_COUNT   = " << char_count   << std::endl;
        //std::cout << "CHAR_SENT    = " << char_sent    << std::endl;
        //std::cout << "std.length() = " << str.length() << std::endl;

        if (char_sent == char_count) {
            break;
        }
        else if (errno == EAGAIN) {
            continue;
        }
        else if (retval == -1) { // check whether send has succeeded
            fprintf(stderr, "\"send()\" failed in function \"thread_send()\", socket is corrupted!\n");
            return false; // send failed
        }

    }

    return true; // send succeeded
}


// Function provide username and md5 hash validation
int apop_parser(int socket, Args* args, std::string& str, std::string& greeting_banner) { // TODO comment

    std::string username;
    std::string digest;
    std::string msg;
    std::size_t pos;

    pos = str.find(" "); // fing first whitespace - delimiter between username and digest

    if (pos == std::string::npos) { // delimiter was not found
        msg = "-ERR Command APOP in AUTHORIZATION state failed! Wrong arguments of APOP.\r\n";
        return (!thread_send(socket, msg)) ? 2 : 1;
    }
    else {
        username = str.substr(0, pos);
        digest = str.substr(pos+1, str.length());
    }

    if (username.empty()) { // missing username
        msg = "-ERR Command APOP in AUTHORIZATION state failed! Wrong arguments of APOP. Username is missing.\r\n";
        return (!thread_send(socket, msg)) ? 2 : 1;
    }

    if (digest.empty()) { // missing diges
        msg = "-ERR Command APOP in AUTHORIZATION state failed! Wrong arguments of APOP. Digest is missing.\r\n";
        return (!thread_send(socket, msg)) ? 2 : 1;
    }

    if (args->username.compare(username) != 0) {
        msg = "-ERR Authentication failed!\r\n"; // wrong username
        return (!thread_send(socket, msg)) ? 2 : 1;
    }

    std::string hash = get_md5_hash(greeting_banner, args->password);
    if (hash.compare(digest) != 0) {
        msg = "-ERR Authentication failed!\r\n"; // wrong digest
        return (!thread_send(socket, msg)) ? 2 : 1;
    }

    return 0; // success
}

// Passed function to thread which define behavior of thread
void thread_main(int socket, Args* args) {

    // DECLARATION
    int res;
    int retval;
    char buff[THREAD_RECV_BUFF_SIZE];
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
    TSEND(socket, msg);

    while(1) {

        // UPDATE
        if (STATE == UPDATE) {
            if (FILENAMES_TO_REMOVE.size() != 0) {
                for (auto i = FILENAMES_TO_REMOVE.begin(); i != FILENAMES_TO_REMOVE.end(); ++i) {
                    remove_file(*i, args);
                }
            }
            close(socket);
            flag_mutex = false;
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
        res = recv(socket, buff, THREAD_RECV_BUFF_SIZE, 0);
        if (flag_exit) { // SIGINT
            close(socket);
            return;
        }
        else if ( TIMEOUT_SECONDS <= (time_curr - time_from)) { // TIMEOUT
            msg = "Timeout expired! You were AFK for "+std::to_string(time_curr - time_from)+" seconds.\r\n";
            TSEND(socket, msg);
            close(socket);
            return;
        }
        else if (res > 0) { // DATA INCOME
            time_from = time(NULL);
            data = buff;
        }
        else if (res == 0) { // client disconnected
            close(socket);
            return; // kill thread
        }
        else if (errno == EAGAIN) { // recv would block EWOULDBLOCK
            continue;
        }
        else {
            fprintf(stderr, "\"recv()\" failed in function \"thread_main\", close socket and kill thread!\n");
            close(socket);
            return;
        }

        if (data.size() < 2) {
            msg = "-ERR Received wrong command!\r\n";
            TSEND(socket, msg);
            continue;
        }
        else {
            data = data.substr(0, data.size()-2); // remmove CRLF (last 2 characters)
            if (data.empty()) { // we dont have string including "COMMAND [ARGS]"
                msg = "-ERR Received wrong command (empty commnad)!\r\n";
                TSEND(socket, msg);
                continue;
            }
        }

        load_cmd_and_args(&COMMAND, CMD_ARGS, data); // split the string to CMD and ARG-STRING (delimiter = whitespace)

        switch(STATE){
            // ==========================================================
            case AUTHORIZATION:
                switch(COMMAND){
                    // ==================================================
                    case QUIT:
                        if (!CMD_ARGS.empty()) { // QUIT str
                            msg = "-ERR Command QUIT in AUTHORIZATION state does not support any arguments!\r\n";
                            TSEND(socket, msg);
                        }
                        else { // QUIT
                            msg = "+OK Closing connection.\r\n";
                            TSEND(socket, msg);
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
                                TSEND(socket, msg);
                            }
                            else { // USER
                                msg = "-ERR Command USER in AUTHORIZATION state has no argument!\r\n";
                                TSEND(socket, msg);
                            }
                        }
                        else { // USER command is NOT supported
                            msg = "-ERR Command USER in AUTHORIZATION state is not supported!\r\n";
                            TSEND(socket, msg);
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
                                            TSEND(socket, msg);
                                            if (!mutex_maildir.try_lock()) {
                                                // RFC: If the maildrop cannot be opened for some reason, the POP3 server responds with a negative status indicator.
                                                msg = "-ERR Maildrop is locked by someone else!\r\n";
                                                TSEND(socket, msg);
                                                // RFC: After returning a negative status indicator, the server may close the connection.
                                                close(socket);
                                                return; // kill thread
                                            }
                                            // TODO check maildir
                                            flag_mutex = true;
                                            STATE = TRANSACTION;
                                            move_new_to_curr(args);
                                            WORKING_VECTOR = load_file_lines_to_vector(DATA_FILE_NAME);
                                        }
                                        else { // Wrong password
                                            msg = "-ERR Authentication failed!\r\n";
                                            TSEND(socket, msg);
                                        }
                                    }
                                    else { // Wrong username
                                        msg = "-ERR Authentication failed!\r\n";
                                        TSEND(socket, msg);
                                    }
                                }
                                else { // PASS
                                    msg = "-ERR Command PASS in AUTHORIZATION state has no argument!\r\n";
                                    TSEND(socket, msg);
                                }
                            }
                            else { // USERNAME missing
                                msg = "-ERR No username given!\r\n";
                                TSEND(socket, msg);
                            }
                        }
                        else { // PASS command is NOT supported
                            msg = "-ERR Command PASS in AUTHORIZATION state is not supported!\r\n";
                            TSEND(socket, msg);
                        }
                        break;
                    // ==================================================
                    case APOP:
                        if (!args->c) { // APOP command is supported
                            if (CMD_ARGS.empty()) { // APOP
                                msg = "-ERR Command APOP in AUTHORIZATION state has no arguments!\r\n";
                                TSEND(socket, msg);
                            }
                            else { // APOP str

                                retval = apop_parser(socket, args, CMD_ARGS, GREETING_BANNER);
                                if (retval == 1) {
                                    break;
                                }
                                else if (retval == 2) {
                                    return;
                                }
                                msg = "+OK Logged in.\r\n";
                                TSEND(socket, msg);

                                if (!mutex_maildir.try_lock()) {
                                    // RFC: If the maildrop cannot be opened for some reason, the POP3 server responds with a negative status indicator.
                                    msg = "-ERR Maildrop is locked by someone else!\r\n";
                                    TSEND(socket, msg);
                                    // RFC: After returning a negative status indicator, the server may close the connection.
                                    close(socket);
                                    return; // kill thread
                                }

                                // TODO check maildir
                                flag_mutex = true;
                                STATE = TRANSACTION;
                                move_new_to_curr(args);
                                WORKING_VECTOR = load_file_lines_to_vector(DATA_FILE_NAME);

                            }
                        }
                        else { // APOP command is NOT supported
                            msg = "-ERR Command APOP in AUTHORIZATION state is not supported!\r\n";
                            TSEND(socket, msg);
                        }
                        break;
                    // ==================================================
                    default:
                        msg = "-ERR Wrong command in AUTHORIZATION state!\r\n";
                        TSEND(socket, msg);
                        break;
                }
                break;
            // ==========================================================
            case TRANSACTION:
                switch(COMMAND){
                    // ==================================================
                    case NOOP:
                        if (!CMD_ARGS.empty()) { // NOOP str
                            msg = "-ERR Command NOOP in TRANSACTION state does not support any arguments!\r\n";
                            TSEND(socket, msg);
                        }
                        else { // NOOP
                            msg = "+OK\r\n";
                            TSEND(socket, msg);
                        }
                        break;
                    // ==================================================
                    case QUIT:
                        if (!CMD_ARGS.empty()) { // QUIT str
                            msg = "-ERR Command QUIT in TRANSACTION state does not support any arguments!\r\n";
                            TSEND(socket, msg);
                        }
                        else { // QUIT
                            msg = "+OK POP3 server signing off.\r\n";
                            TSEND(socket, msg);
                            STATE = UPDATE;
                        }
                        break;
                    // ==================================================
                    case UIDL:
                        if (CMD_ARGS.empty()) { // UIDL

                            msg = "+OK\r\n";
                            TSEND(socket, msg);

                            unsigned int index = 0;
                            std::string filename;

                            if (vector_size(WORKING_VECTOR) > 0) { // WORKING_VECTOR is empty
                                for (auto i = WORKING_VECTOR.begin(); i != WORKING_VECTOR.end(); ++i) {
                                    index++;
                                    if ((*i).compare("") != 0) {
                                        filename = (*i).substr(0, (*i).find("/"));
                                        msg = std::to_string(index) + " " + get_file_id(filename, WORKING_VECTOR) + "\r\n";
                                        TSEND(socket, msg);
                                    }
                                }
                            }

                            msg = ".\r\n";
                            TSEND(socket, msg);
                        }
                        else { // UIDL str
                            if (!is_number(CMD_ARGS.c_str())) {
                                msg = "-ERR Command UIDL in state TRANSACTION needs a mumerical argument (index)!\r\n";
                                TSEND(socket, msg);
                                break;
                            }
                            unsigned int index = std::stoi(CMD_ARGS);
                            unsigned int WV_size = WORKING_VECTOR.size();
                            if (0 < index && index <= WV_size) {
                                std::string filename = WORKING_VECTOR[index-1];
                                if (filename.empty()) {
                                    msg = "-ERR Message is already deleted!\r\n";
                                    TSEND(socket, msg);
                                }
                                else {
                                    filename = filename.substr(0, filename.find("/"));
                                    msg = "+OK " + std::to_string(index) + " " + get_file_id(filename, WORKING_VECTOR) + "\r\n";
                                    TSEND(socket, msg);
                                }
                            }
                            else {
                                msg = "-ERR Out of range indexing in messages via \"UIDL <index>\"!\r\n";
                                TSEND(socket, msg);
                            }

                        }
                        break;
                    // ==================================================
                    case STAT:
                        if (CMD_ARGS.empty()) {
                            msg = "+OK " + std::to_string(vector_size(WORKING_VECTOR)) + " " + std::to_string(get_size_summary(WORKING_VECTOR)) + "\r\n";
                            TSEND(socket, msg);
                        }
                        else {
                            msg = "-ERR Command STAT in TRANSACTION state does not support any arguments!\r\n";
                            TSEND(socket, msg);
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
                            TSEND(socket, msg);

                            unsigned int index = 0;
                            std::string filename;

                            if (vector_size(WORKING_VECTOR) > 0) { // WORKING_VECTOR is empty
                                for (auto i = WORKING_VECTOR.begin(); i != WORKING_VECTOR.end(); ++i) {
                                    index++;
                                    if ((*i).compare("") != 0) {
                                        filename = (*i).substr(0, (*i).find("/"));
                                        msg = std::to_string(index) + " " + std::to_string(get_file_size(filename, WORKING_VECTOR)) + "\r\n";
                                        TSEND(socket, msg);
                                    }
                                }
                            }

                            msg = ".\r\n";
                            TSEND(socket, msg);
                        }
                        else { // LIST str
                            if (!is_number(CMD_ARGS.c_str())) {
                                msg = "-ERR Command LIST in state TRANSACTION needs a numerical argument (index)!\r\n";
                                TSEND(socket, msg);
                                break;
                            }
                            unsigned int index = std::stoi(CMD_ARGS);
                            unsigned int WV_size = WORKING_VECTOR.size();
                            if (0 < index && index <= WV_size) {
                                std::string filename = WORKING_VECTOR[index-1];
                                if (filename.empty()) {
                                    msg = "-ERR Message is already deleted!\r\n";
                                    TSEND(socket, msg);
                                }
                                else {
                                    filename = filename.substr(0, filename.find("/"));
                                    msg = "+OK " + std::to_string(index) + " " + std::to_string(get_file_size(filename, WORKING_VECTOR)) + "\r\n";
                                    TSEND(socket, msg);
                                }
                            }
                            else {
                                msg = "-ERR Out of range indexing in messages via \"LIST <index>\"!\r\n";
                                TSEND(socket, msg);
                            }

                        }
                        break;
                    // ==================================================
                    case RETR:
                        if (!CMD_ARGS.empty()) { // RETR str

                            if (!is_number(CMD_ARGS.c_str())) {
                                msg = "-ERR Command RETR in state TRANSACTION needs a numerical argument (index)!\r\n";
                                TSEND(socket, msg);
                                break;
                            }

                            unsigned int index = std::stoi(CMD_ARGS);
                            unsigned int WV_size = WORKING_VECTOR.size();

                            if (0 < index && index <= WV_size) {
                                std::string filename = WORKING_VECTOR[index-1];
                                if (filename.empty()) {
                                    msg = "-ERR Message is already deleted!\r\n";
                                    TSEND(socket, msg);
                                }
                                else {
                                    filename = filename.substr(0, filename.find("/"));

                                    msg = "+OK " + std::to_string(get_file_size(filename, WORKING_VECTOR)) + " octets\r\n";
                                    TSEND(socket, msg);

                                    std::string filepath = (args->path_maildir_cur.back() == '/') ? args->path_maildir_cur + filename :args->path_maildir_cur + "/" + filename;
                                    msg = get_file_content(filepath);
                                    msg += ".\r\n";
                                    TSEND(socket, msg);
                                }
                            }
                            else {
                                msg = "-ERR Out of range indexing in messages via \"RETR <index>\"!\r\n";
                                TSEND(socket, msg);
                            }
                        }
                        else { // RETR
                            msg = "-ERR Command RETR in TRANSACTION state needs a argument!\r\n";
                            TSEND(socket, msg);
                        }
                        break;
                    // ==================================================
                    case DELE:
                        if (!CMD_ARGS.empty()) { // DELE str
                            if (!is_number(CMD_ARGS.c_str())) {
                                msg = "-ERR Command DELE in state TRANSACTION needs a mumerical argument (index)!\r\n";
                                TSEND(socket, msg);
                                break;
                            }
                            unsigned int index = std::stoi(CMD_ARGS);
                            unsigned int WV_size = WORKING_VECTOR.size();
                            if (index <= WV_size) {
                                std::string filename = WORKING_VECTOR[index-1];
                                if (filename.empty()) {
                                    msg = "-ERR Message is already deleted!\r\n";
                                    TSEND(socket, msg);
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

                                    msg = "+OK Message " + CMD_ARGS + " deleted.\r\n";
                                    TSEND(socket, msg);
                                }
                            }
                            else {
                                msg = "-ERR Out of range indexing in messages via \"DELE <index>\"!\r\n";
                                TSEND(socket, msg);
                            }
                        }
                        else {
                            msg = "-ERR Command DELE in state TRANSACTION needs a argument!\r\n";
                            TSEND(socket, msg);
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
                                        break;
                                    }
                                }
                            }
                            FILENAMES_TO_REMOVE.clear();
                            msg = "+OK\r\n";
                            TSEND(socket, msg);
                        }
                        else { // RSET str
                            msg = "-ERR Command RSET in state TRANSACTION does not support any arguments!\r\n";
                            TSEND(socket, msg);
                        }
                        break;
                    // ==================================================
                    default:
                        msg = "-ERR Wrong command in TRANSACTION state!\r\n";
                        TSEND(socket, msg);
                        break;
                }
                break;
            // ==========================================================
            default: // never happen, DONT TOUCH
                break;
        }
    }
}
