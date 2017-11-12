#include "md5.hpp"

#include "constants.hpp" // HOSTNAME_LENGTH
#include <unistd.h>      // getpid()
#include <string.h>      // memset()
#include <openssl/md5.h> // MD5()

// Function creates string for MD5 hash
std::string get_greeting_banner() {

    std::string str;

    pid_t  h_pid  = getpid();   // PID
    time_t h_time = time(NULL); // TIME

    char h_hostname[HOSTNAME_LENGTH];
    int retval = gethostname(h_hostname, HOSTNAME_LENGTH); // HOSTNAME
    if(retval != 0) {
        fprintf(stderr, "\"gethostname()\" failed in function \"get_greeting_banner()\", hostname will be empty string!\n");
        memset(&h_hostname,0,sizeof(h_hostname));
    }

    // [<][PID][.][TIME][@][HOSTNAME][>]
    str = "<" + std::to_string(h_pid) + "." + std::to_string(h_time) + "@" + h_hostname + ">";

    return str;
}

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