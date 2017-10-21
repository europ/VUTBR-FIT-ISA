#include <iostream>
#include <string>
#include <vector>

#include <dirent.h>
#include <sys/stat.h> // file_exists

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

std::vector<std::string> list_dir(std::string dirpath) {

    std::vector<std::string> files;
    std::string data;

    struct dirent *entry;
    DIR *dir = opendir(dirpath.c_str());
    if (dir == NULL) {
        return files;
    }
    while ((entry = readdir(dir)) != NULL) {
        data = entry->d_name;
        if (file_exists(data)) {
            data = realpath(data.c_str(), NULL); // TODO FIXIT realpath may return NULL
            files.push_back(data);
        }
    }
    closedir(dir);

    return files;
}

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

int main(int argc, char* argv[]) {
    
    if (argc != 2) {
        fprintf(stderr, "USAGE: ./test <dirpath>");
        exit(1);
    }

    std::vector<std::string> files;
    files = list_dir(argv[1]);
    if (!files.empty()) {
        for (auto i = files.begin(); i != files.end(); ++i) {
            std::cout << *i << std::endl;
        }
    }

    std::cout << std::endl;

    bool retval = move_file("/home/adrian/Downloads/file_test.asd", "/home/adrian/Downloads/test_directory");
    if (retval) {
        std::cout << "+" << std::endl;
    }
    else {
        std::cout << "-" << std::endl;
    }

    return 0;
}
