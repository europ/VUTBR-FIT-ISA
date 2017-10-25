// Function appends a one line (string) to a file
void append_line_to_file(std::string data, std::string filepath) {
    std::ofstream file;
    file.open(filepath, std::fstream::out | std::fstream::app);
    file << data << std::endl;
    file.close();
}

// TODO FIXIT REMOVE THIS
void print_status(State S, Command C) {

    std::string state, command;

    if (S == AUTHORIZATION) state = "AUTHORIZATION";
    else if (S == TRANSACTION) state = "TRANSACTION";
    else if (S == UPDATE) state = "UPDATE";
    else state = "\033[1m\033[31mSTATE_ERROR\033[0m";

    if (C == QUIT) command = "QUIT";
    else if (C == STAT) command = "STAT";
    else if (C == LIST) command = "LIST";
    else if (C == RETR) command = "RETR";
    else if (C == DELE) command = "DELE";
    else if (C == NOOP) command = "NOOP";
    else if (C == RSET) command = "RSET";
    else if (C == UIDL) command = "UIDL";
    else if (C == USER) command = "USER";
    else if (C == PASS) command = "PASS";
    else if (C == APOP) command = "APOP";
    else command = "\033[1m\033[31mCOMMAND_ERROR\033[0m";

    std::cout << "\033[33m" << "STATE = " << state << std::endl << "COMMAND = " << command << "\033[0m" << std::endl << std::endl;
    return;
}

// Function check if filename is in directory
bool filename_is_in_dir(std::string filename, std::string directory) {
    std::string data;
    data = (directory.back() == '/') ? directory : directory + "/";
    data.append(filename);
    return (file_exists(data)) ? true : false;
}

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

unsigned int get_cur_size(std::vector<std::string>& files) {

    if (files.empty()) return 0;

    std::vector<std::string> data_lines = load_file_lines_to_vector(DATA_FILE_NAME);

    std::string buff;
    std::string name;
    std::string size;
    std::vector<std::string> sizes;

    for (auto i = data_lines.begin(); i != data_lines.end(); ++i) {
        buff = *i;
        name = buff.substr(0, buff.find("/"));
        size = buff.substr(buff.find_last_of("/")+1, std::string::npos);
        if (!is_file_deleted(name)) { // maybe not useful TODO FIXIT ... input files are filtered searching for removed files so these are without deleted files
            if (std::find(files.begin(), files.end(), name) != files.end()) {
                sizes.push_back(size);
            }
        }
    }

    unsigned int octets = 0;
    for (auto i = sizes.begin(); i != sizes.end(); ++i) {
        octets += std::stoi(*i);
    }

    return octets;
}
void remove_deleted_files(std::vector<std::string>& files) {
    if (files.empty()) return;
    std::vector<std::string> del_file_content = load_file_lines_to_vector(DEL_FILE_NAME);
    remove_matches_from_vector(files, del_file_content);
    return;
}
