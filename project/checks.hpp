#ifndef __checks_hpp
#define __checks_hpp

#include <string>

// Check string if it consists from numbers only
bool is_number(const char* str);

// Check if FILE exists
bool file_exists(const std::string path);

// Check if DIRECTORY exists
bool dir_exists(const std::string& path);

#endif