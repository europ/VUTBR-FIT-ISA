#include <iostream>
#include <string>
#include <fstream>

#define DEL_FILE_NAME "del"

// Function return true/false on status of file deleted/non-deleted
bool is_file_deleted(std::string filename) {
    std::ifstream file(DEL_FILE_NAME);
    std::string line;
    while (std::getline(file, line)) {
        if (filename.compare(line) == 0) {
            return true;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "USAGE: ./test \"filename\"");
        exit(1);
    }

    std::cout << ((is_file_deleted(argv[1])) ? "YES" : "NO") << std::endl;

    return 0;
}
