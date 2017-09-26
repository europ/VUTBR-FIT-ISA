#include <iostream>
#include <string>
#include <string.h>

using namespace std;

class Args {

	public:

		bool h = false;
		bool a = false;
		bool c = false;
		bool p = false;
		bool d = false;
		bool r = false;
		string path_a = "";
		string port = "";
		string path_d = "";

		// DEBUG INFO
		void print() {
			cout << "h = " << (h ? "T" : "False") << endl;
			cout << "a = " << (a ? "T" : "False") << endl;
			cout << "c = " << (c ? "T" : "False") << endl;
			cout << "p = " << (p ? "T" : "False") << endl;
			cout << "d = " << (d ? "T" : "False") << endl;
			cout << "r = " << (r ? "T" : "False") << endl;
			cout << "path_a = " << path_a << endl;
			cout << "port   = " << port   << endl;
			cout << "path_d = " << path_d << endl;
		}

};

void usage() {
	string message =
		"This\n"
		"is\n"
		"help\n"
		"message\n"
	;
	cout << message;
	exit(0);
}

void error(int exitcode) {
	switch (exitcode) {
		case 1:
			cerr << "Invalid arguments!\n";
			exit(1);
			break;
		case 2:
			cerr << "Argument value is missing!\n";
			exit(2);
			break;
		case 3:
			cerr << "Arguments are missing!\n";
			exit(3);
			break;
		case 4:
			cerr << "Wrong combination of \"-h\"!\n";
			exit(4);
			break;
		default: 
			break;
	}
}

bool isarg(char* str) {
	if (strcmp(str, "-h") == 0) return true;
	else if (strcmp(str, "-a") == 0) return true;
	else if (strcmp(str, "-c") == 0) return true;
	else if (strcmp(str, "-p") == 0) return true;
	else if (strcmp(str, "-d") == 0) return true;
	else if (strcmp(str, "-r") == 0) return true;
	else return false;
}

void argpar(int* argc, char* argv[], Args* args) {
	if (*argc <= 1) {
		error(3);
	}
	for(int i=1; i<*argc; i++) {
		if (strcmp(argv[i], "-h") == 0) {
			args->h = true;
			if (args->a == true || args->c == true || args->p == true || args->d == true || args->r == true) {
				error(4);
			}
			else if (*argc > 2) {
				error(4);
			}
			else {
				usage();
			}
		}
		else if (strcmp(argv[i], "-a") == 0) {
			args->a = true;
			if (i+1 < *argc && !isarg(argv[i+1])) {
				args->path_a = argv[i+1];
				i++;
			}
			else {
				error(2);
			}
		}
		else if (strcmp(argv[i], "-c") == 0) {
			args->c = true;
		}
		else if (strcmp(argv[i], "-p") == 0) {
			if (i+1 < *argc && !isarg(argv[i+1])) {
				args->p = true;
				args->port = argv[i+1];
				i++;
			}
			else {
				error(2);
			}
		}
		else if (strcmp(argv[i], "-d") == 0) {
			if (i+1 < *argc && !isarg(argv[i+1])) {
				args->d = true;
				args->path_d = argv[i+1];
				i++;
			}
			else {
				error(2);
			}
		}
		else if (strcmp(argv[i], "-r") == 0) {
			args->r = true;
		}
		else {
			error(1);
		}
	}
}

int main(int argc, char* argv[]) {

	Args args;
	argpar(&argc, argv, &args);

	args.print();
	cout << endl;
	return 0;
}
