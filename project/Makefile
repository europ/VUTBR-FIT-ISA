CC=g++
CF=-Wall -Wextra -pedantic -std=c++11 -pthread -lcrypto -lssl
PROJECT=popser

.PHONY: make
make:
	$(CC) $(CF) *.cpp -o $(PROJECT)

.PHONY: clean
clean:
	@rm -f $(PROJECT)
