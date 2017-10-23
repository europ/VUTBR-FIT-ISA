#include <iostream>
#include <string>
#include <vector>

#include <fstream>
#include <dirent.h>
#include <sys/stat.h>

#define LOG_FILE_NAME "log"

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

// Function get the realpath of every file (only file) in directory
std::vector<std::string> list_dir(std::string dirpath) {

    std::vector<std::string> files;
    std::string data;

    struct dirent *entry;
    DIR *dir = opendir(dirpath.c_str());
    if (dir == NULL) {
        return files;
    }
    while ((entry = readdir(dir)) != NULL) {
        data = (dirpath.back() == '/') ? dirpath : dirpath + "/";
        data.append(entry->d_name);
        if (file_exists(data)) {
            data = realpath(data.c_str(), NULL); // TODO FIXIT realpath may return NULL
            files.push_back(data);
        }
    }
    closedir(dir);

    return files;
}

// Function move file to specified directory
bool move_file(std::string filepath, std::string dirpath) {

    if (filepath.empty() || dirpath.empty()) {
        return false;
    }

    if (!file_exists(filepath) || !dir_exists(dirpath)) {
        return false;
    }

    std::string filename, newfilepath;
    filename = filepath.substr(filepath.find_last_of("/")+1, std::string::npos);
    newfilepath = (dirpath.back() == '/') ? dirpath : dirpath + "/";
    newfilepath.append(filename);

    int retval = rename(filepath.c_str(), newfilepath.c_str());
    if (retval != 0) {
        return false;
    }

    return true;
}

void move_new_to_curr(std::string from_dir, std::string to_dir) {

    std::ofstream logfile;
    logfile.open(LOG_FILE_NAME, std::fstream::out | std::fstream::app);

    std::vector<std::string> files;
    files = list_dir(from_dir);
    if (!files.empty()) {
        for (auto i = files.begin(); i != files.end(); ++i) {
            if (move_file(*i, to_dir)) {
                logfile << *i << std::endl;
            }
            else {
                std::cout << "FAIL: move_file() - " << *i << std::endl; // TODO REMOVE THIS !
            }
        }
    }
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		fprintf(stderr, "USAGE: ./test \"from_dir\" \"to_dir\"");
		exit(1);
	}
	move_new_to_curr(argv[1], argv[2]);
    return 0;
}
