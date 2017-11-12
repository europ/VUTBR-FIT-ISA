#ifndef __md5_hpp
#define __md5_hpp

#include <string> // std::string

// Function creates string for MD5 hash
std::string get_greeting_banner();

// Function create a MD5 digest hash
std::string get_md5_hash(std::string& greeting_banner, std::string& password);

#endif