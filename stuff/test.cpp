#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

// Function load file content line by line as std::string to a std::vector
std::vector<std::string> load_file_lines_to_vector(std::string file) {

    std::ifstream logfile(file);
    std::vector<std::string> files;
    std::string line;

    while (std::getline(logfile, line)) {
        files.push_back(line);
    }

    logfile.close();

    return files;
}

void remove_matches_from_vector(std::vector<std::string>& vector, std::vector<std::string>& data) {
    if (vector.empty() || data.empty()) return;
    for (auto i = data.begin(); i != data.end(); ++i) {
        vector.erase(std::remove(vector.begin(), vector.end(), (*i).c_str()), vector.end());
    }
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "USAGE: ./test \"filename\"");
        exit(1);
    }

    std::vector<std::string> v = { "adrian", "adam", "jakub", "peter", "filip", "asdf", "juraj", "martin", "david" };
    std::vector<std::string> content = load_file_lines_to_vector(argv[1]);

    std::cout << std::endl;
    std::cout << "FILE CONTENT ==================" << std::endl;
    for (auto i = content.begin(); i != content.end(); ++i) {
        std::cout << *i << std::endl;
    } 

    std::cout << std::endl;
    std::cout << "VECTOR BEFORE =================" << std::endl;
    for (auto i = v.begin(); i != v.end(); ++i) {
        std::cout << *i << std::endl;
    }

    std::cout << std::endl;
    std::cout << "VECTOR AFTER ==================" << std::endl;
    
    remove_matches_from_vector(v, content);

    for (auto i = v.begin(); i != v.end(); ++i) {
            std::cout << *i << std::endl;
    }

    return 0;
}
