#include "checks.hpp"

#include <sys/stat.h> // stat, stat()

// Check string if it consists from numbers only
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