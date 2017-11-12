#include "datatypes.hpp"

#include <algorithm> // std::transform()

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