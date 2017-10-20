#include <iostream>
#include <string>

#include <fstream>
#include <sstream>

std::string file_size(std::string filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    std::stringstream ss;
    std::string str;
    ss << in.tellg();
    ss >> str;
    return str; 
}

int main(int argc, char* argv[]) {
    
    if (argc != 2) {
        fprintf(stderr, "USAGE: ./test <filepath>");
        exit(1);
    }

    std::string size = file_size(argv[1]);

    std::cout << size;

    return 0;
}
