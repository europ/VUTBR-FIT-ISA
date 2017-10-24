#include <iostream>
#include <string>
#include <ctime>

#define ID_LENGTH 20
#define ID_CHARS "!\"#$%&'()*+,-." /* excluding SLASH */   "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"

std::string id_generator() {

    unsigned length = ID_LENGTH;
    char alphanum[] = ID_CHARS;
    std::string str = "";

    srand(std::clock());

    for (unsigned i = 0; i < length; ++i) {
        str += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return str;
}

int main(int argc, char* argv[]) {

    (void)argc;
    (void)argv;

    std::cout << id_generator() << std::endl;

    return 0;
}
