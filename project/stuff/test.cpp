#include <iostream>
#include <string>
#include <fstream>

// Function get the file size in octets
std::string file_size(std::string filename) {

    long long unsigned int size = 0;

    // file ends with \n
    bool ENDS_WITH_NEWLINE = false;
    std::ifstream in(filename);
    std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (str.back() == '\n') ENDS_WITH_NEWLINE = true;
    
    // file size
    std::ifstream infile(filename, std::ifstream::ate | std::ifstream::binary);
    size = infile.tellg();

    std::string line;
    std::ifstream file(filename);
    std::string buff;
    while(std::getline(file, line)) {
        if (line.back() != '\r') {
            size++;
        }
    }

    if (!ENDS_WITH_NEWLINE) {
        size++;
    }

    return std::to_string(size);
}


int main(int argc, char* argv[]) {

    if (argc != 2) exit(1);
    std::cout << file_size(argv[1]) << std::endl;

    return 0;
}
    