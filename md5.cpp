#include <iostream>
#include <string>
#include <openssl/md5.h>


int main(/*int argc, char* argv[]*/) {

	std::string in_data = "adrian";
	std::string out_data = "";

	unsigned char hash[32];
    MD5((const unsigned char*)in_data.c_str(), in_data.length(), hash);

    char buff[32];
    for (int i=0;i<16;i++){
    	sprintf(buff, "%02x", hash[i]);
    	out_data.append(buff);
	}

    std::cout << "DATA =" << in_data << "=" << std::endl;
    std::cout << "MY   HASH =" << out_data << "=" << std::endl;
    std::cout << "REAL HASH =8c4205ec33d8f6caeaaaa0c10a14138c=" << std::endl;

    return 0;
}
