#include <iostream>
#include <string>
#include <openssl/md5.h>

std::string get_md5_hash(std::string& greeting_banner, std::string& password) {

    std::string hash;
    std::string input;

    input.append(greeting_banner);
    input.append(password);

    unsigned char result[32];
    MD5((const unsigned char*)input.c_str(), input.length(), result);

    char buffer[32];
    for (int i=0;i<16;i++){
        sprintf(buffer, "%02x", result[i]);
        hash.append(buffer);
    }

    return hash;
}


int main(int argc, char* argv[]) {

	std::string greeting_banner;
	std::string password;
	std::string hash;

	if (argc != 3) {
		fprintf(stderr, "Usage: ./hash \"greeting_banner\" \"password\"");
		exit(1);
	}
	greeting_banner = argv[1];
	password = argv[2];

	hash = get_md5_hash(greeting_banner, password);

	std::cout << hash << std::endl;

    return 0;
}