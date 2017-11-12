#ifndef __argpar_hpp
#define __argpar_hpp

#include "datatypes.hpp" // Args

// Print the help message to stdout and terminate the program
void usage();

// Function loads username and password from authentication file
void load_auth_file(Args* args);

// Parsing arguments and initializating class args
void argpar(int* argc, char* argv[], Args* args);

#endif