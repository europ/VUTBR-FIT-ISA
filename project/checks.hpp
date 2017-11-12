#ifndef __checks_hpp
#define __checks_hpp

#include <string> // std::string

#include "datatypes.hpp" // Args

// Check string if it consists from numbers only
bool is_number(const char* str);

// Check if FILE exists
bool file_exists(const std::string path);

// Check if DIRECTORY exists
bool dir_exists(const std::string& path);

// Function checks the structure of maildir
void check_maildir(Args* args);

#endif