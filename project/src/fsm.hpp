#ifndef __fsm_hpp
#define __fsm_hpp

#include <string> // std::string

#include "datatypes.hpp" // Command, Args

// Parser for string which is in format "COMMAND + ' ' + ARGUMENTS"
void load_cmd_and_args(Command* CMD, std::string& ARGS, std::string& str);

// Function sends message to client via socket
bool thread_send(int socket, std::string& str);

// Function provide username and md5 hash validation
int apop_parser(int socket, Args* args, std::string& str, std::string& greeting_banner);

// Passed function to thread which define behavior of thread
void thread_main(int socket, Args* args);

#endif
