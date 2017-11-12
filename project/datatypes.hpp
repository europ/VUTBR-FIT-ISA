#ifndef __datatypes_hpp
#define __datatypes_hpp

#include <string> // std::string

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

// Converter STRING to ENUM
Command get_command(std::string& str);

#endif