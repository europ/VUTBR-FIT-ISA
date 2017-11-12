#include "logger.hpp"

#include <fstream>
#include <dirent.h>

#include "checks.hpp"
#include "constants.hpp"

// Function move file to specified directory
bool move_file(std::string filepath, std::string dirpath) {

    if (filepath.empty() || dirpath.empty()) {
        return false;
    }

    std::string filename, newfilepath;

    filename = filepath.substr(filepath.find_last_of("/")+1, std::string::npos);
    newfilepath = (dirpath.back() == '/') ? dirpath + filename : dirpath + "/" + filename;

    int retval = rename(filepath.c_str(), newfilepath.c_str());
    if (retval != 0) {
        return false;
    }

    return true;
}

// Function load file content line by line as std::string to a std::vector
std::vector<std::string> load_file_lines_to_vector(std::string file) {

    std::vector<std::string> files;

    if (!file_exists(file)) {
        return files; // empty vector
    }

    std::ifstream logfile(file);
    std::string line;

    while (std::getline(logfile, line)) {
        files.push_back(line);
    }

    logfile.close();

    return files;
}

// Reset
void reset() {

    if (file_exists(DATA_FILE_NAME)) { // remove DATA_FILE_NAME
        remove(DATA_FILE_NAME);
    }

    if (file_exists(LOG_FILE_NAME)) { // remove LOG_FILE_NAME

        std::vector<std::string> files;
        files = load_file_lines_to_vector(LOG_FILE_NAME); // load LOG_FILE_NAME

        std::string buff;
        std::string path_cur;
        std::string path_new;

        // calculation of maildir path
        buff = files.front();
        buff = buff.substr(0, buff.find_last_of("/"));
        buff = buff.substr(0, buff.find_last_of("/")+1);

        path_cur = buff + "cur"; // maildir/cur path
        path_new = buff + "new"; // maildir/new path

        std::string filename;
        std::string file_in_new;
        std::string file_in_cur;

        for (auto i = files.begin(); i != files.end(); ++i) {
            file_in_new = *i; // absolute path of file in NEW
            filename = file_in_new.substr(file_in_new.find_last_of("/")+1, std::string::npos); // filename
            file_in_cur = path_cur + "/" + filename; // absolute path of file in CUR
            if (file_exists(file_in_cur)) { // file exists in CUR
                move_file(file_in_cur, path_new); // move file from CUR to NEW
            }
        }

        remove(LOG_FILE_NAME);
    }

    return;
}

// Function generate a random server-determined string for message ID
std::string id_generator(std::string filename) {

    // get int value of filename => count every int value of every char in string
    long double filename_number = 0;
    for(char& c : filename) {
        filename_number += int(c);
    }

    // https://wis.fit.vutbr.cz/FIT/st/phorum-msg-show.php?id=49087
    // Martin Holkovič answered "1. yes" it means that every file will have its UNIQUE name
    srand(filename_number);

    std::string str = "";
    char alphanum[] = ID_CHARS;

    // generate a random string
    for (unsigned int i = 0; i < ID_LENGTH; ++i) {
        str += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    // return UID string
    return str;
}

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

    // file does not end with \n
    if (!ENDS_WITH_NEWLINE) {
        size++;
    }

    return std::to_string(size);
}

// Function return DATA-LINE specified by filename from DATA_FILE_NAME
std::string get_filename_line_from_data(std::string filename) {
    std::ifstream data(DATA_FILE_NAME);
    std::string line, line_filename;
    while(std::getline(data, line)) {
        line_filename = line.substr(0, line.find("/"));
        if (filename.compare(line_filename) == 0) {
            return line;
        }
    }
    data.close();
    return ""; // filename not found in file DATA_FILE_NAME
}

// Function loads the whole file content into std::string
std::string get_file_content(std::string filepath) {

    std::ifstream file(filepath);

    std::string str;
    std::string line;

    while (std::getline(file, line)) {

        // add DOT before DOT => line starts with dot so add a dot before that dot
        if (line.front() == '.')
            line.insert(0, ".");

        // every line must be terminated with CRLF
        if (line.back() != '\r')
            line.append("\r");

        line.append("\n"); // getline() remove \n when it is reading from file

        // line is handled and has \r\n at the end
        str.append(line);
    }

    return str;
}

// Function provide addition of filesizes which are saved in vector (vector = loaded DATA file)
unsigned int get_size_summary(std::vector<std::string>& data) {
    unsigned int octets = 0;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            octets += std::stoi((*i).substr((*i).find_last_of("/")+1, std::string::npos));
        }
    }
    return octets;
}

// Function return file size from vector (vector = loaded DATA file)
unsigned int get_file_size(std::string& filename, std::vector<std::string>& data) {
    unsigned int size = 0;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            if (filename.compare((*i).substr(0, (*i).find("/"))) == 0) {
                size = std::stoi((*i).substr((*i).find_last_of("/")+1, std::string::npos));
            }
        }
    }
    return size;
}

// Function return file id from vector (vector = loaded DATA file)
std::string get_file_id(std::string& filename, std::vector<std::string>& data) {
    std::string id = "";
    std::size_t first_slash;
    std::size_t last_slash;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            if (filename.compare((*i).substr(0, (*i).find("/"))) == 0) {
                first_slash = (*i).find("/");
                last_slash  = (*i).find_last_of("/");
                id = (*i).substr(first_slash+1, (last_slash-first_slash)-1);
            }
        }
    }
    return id;
}

// Function return file names from vector (vector = loaded DATA file)
std::vector<std::string> get_files(std::vector<std::string>& data) {
    std::vector<std::string> files;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            files.push_back((*i).substr(0, (*i).find("/")));
        }
    }
    return files;
}

// Function return vector size (count of messages which are not marked as deleted)
unsigned int vector_size(std::vector<std::string>& data) {
    unsigned int counter = 0;
    for (auto i = data.begin(); i != data.end(); ++i) {
        if ((*i).compare("") != 0) {
            counter++;
        }
    }
    return counter;
}

void remove_file(std::string& filename, Args* args) {

    std::string content_line_filename;
    std::vector<std::string> vector;
    std::vector<std::string> content;
    std::ofstream output_file;

    //remove from file DATA_FILE_NAME
    content = load_file_lines_to_vector(DATA_FILE_NAME);
    for (auto i = content.begin(); i != content.end(); ++i) {
        content_line_filename = (*i).substr(0, (*i).find("/"));
        if (filename.compare(content_line_filename) != 0) {
            vector.push_back(*i);
        }
    }
    output_file.open(DATA_FILE_NAME);
    for (auto i = vector.begin(); i != vector.end(); ++i) {
        output_file << *i << std::endl;
    }

    //remove from file LOG_FILE_NAME
    content = load_file_lines_to_vector(LOG_FILE_NAME);
    for (auto i = content.begin(); i != content.end(); ++i) {
        content_line_filename = (*i).substr((*i).find_last_of("/")+1, std::string::npos);
        if (filename.compare(content_line_filename) != 0) {
            vector.push_back(*i);
        }
    }
    output_file.open(LOG_FILE_NAME);
    for (auto i = vector.begin(); i != vector.end(); ++i) {
        output_file << *i << std::endl;
    }

    //remove physically maildir/cur/filename
    std::string filepath;
    filepath = (args->path_maildir_cur.back() == '/') ? args->path_maildir_cur + filename : args->path_maildir_cur + "/" + filename;
    remove(filepath.c_str());
}

// Function get the realpath of every file (only file) in directory
std::vector<std::string> get_file_paths_in_directory(std::string dirpath) {

    std::vector<std::string> files;

    struct dirent *entry;
    DIR *dir = opendir(dirpath.c_str());
    if (dir == NULL) { // check directory
        return files;
    }

    std::string data;

    while ((entry = readdir(dir)) != NULL) { // read files from directory
        data = (dirpath.back() == '/') ? dirpath + entry->d_name : dirpath + "/" + entry->d_name; // create relative path
        if (file_exists(data)) { // check file
            data = realpath(data.c_str(), NULL); // create absolute path
            if (!data.empty()) { // file is alright
                files.push_back(data);
            }
        }
    }
    closedir(dir);

    return files;
}

// Function move content of maildir/new to maildir/curr
void move_new_to_curr(Args* args) {

    //##########################################
    // ABSOLUTE PATHS OF ALL FILES in maildir/NEW
    //##########################################
    std::vector<std::string> files;
    files = get_file_paths_in_directory(args->path_maildir_new); // returns ABSOLUTE paths of every files in directory

    if (files.empty()) return; // 0 files in maildir/NEW



    //##########################################
    // CREATE DATA FILE
    //##########################################
    std::ofstream datafile;
    datafile.open(DATA_FILE_NAME, std::fstream::out | std::fstream::app);

    std::string f_name;
    std::string f_id;
    std::string f_size;

    for (auto i = files.begin(); i != files.end(); ++i) {

        f_name = (*i).substr((*i).find_last_of("/")+1, std::string::npos); // filename
        f_id   = id_generator(f_name);                                     // file unique-id
        f_size = file_size(*i);                                            // file size

        // [filename][/][unique-id][/][size]["\n"]
        datafile << f_name << DATA_FILE_DELIMITER << f_id << DATA_FILE_DELIMITER << f_size << std::endl;
    }

    datafile.close();



    //##########################################
    // CREATE LOG FILE
    //##########################################
    std::ofstream logfile;
    logfile.open(LOG_FILE_NAME, std::fstream::out | std::fstream::app);

    for (auto i = files.begin(); i != files.end(); ++i) {
        if (move_file(*i, args->path_maildir_cur)) {
            // [file absolute path][\n]
            logfile << *i << std::endl;
        }
    }

    logfile.close();



    return;
}
