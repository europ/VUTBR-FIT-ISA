#ifndef __logger_hpp
#define __logger_hpp

#include <string> // std::string
#include <vector> // std::vector<>

#include "datatypes.hpp" // Args

// Function move file to specified directory
bool move_file(std::string filepath, std::string dirpath);

// Function load file content line by line as std::string to a std::vector
std::vector<std::string> load_file_lines_to_vector(std::string file);

// Reset
void reset();

// Function generate a random server-determined string for message ID
std::string id_generator(std::string filename);

// Function get the file size in octets
std::string file_size(std::string filename);

// Function return DATA-LINE specified by filename from DATA_FILE_NAME
std::string get_filename_line_from_data(std::string filename);

// Function loads the whole file content into std::string
std::string get_file_content(std::string filepath);

// Function provide addition of filesizes which are saved in vector (vector = loaded DATA file)
unsigned int get_size_summary(std::vector<std::string>& data);

// Function return file size from vector (vector = loaded DATA file)
unsigned int get_file_size(std::string& filename, std::vector<std::string>& data);

// Function return file id from vector (vector = loaded DATA file)
std::string get_file_id(std::string& filename, std::vector<std::string>& data);

// Function return file names from vector (vector = loaded DATA file)
std::vector<std::string> get_files(std::vector<std::string>& data);

// Function return vector size (count of messages which are not marked as deleted)
unsigned int vector_size(std::vector<std::string>& data);

void remove_file(std::string& filename, Args* args);

// Function get the realpath of every file (only file) in directory
std::vector<std::string> get_file_paths_in_directory(std::string dirpath);

// Function move content of maildir/new to maildir/curr
void move_new_to_curr(Args* args);

#endif
