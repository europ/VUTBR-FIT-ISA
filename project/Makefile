CC=g++
CFLAGS=-Wall -Wextra -pedantic -std=c++11 -pthread -lcrypto -lssl
PROJECT=popser

all: $(PROJECT)

checks.o: checks.cpp checks.hpp
	$(CC) $(CFLAGS) -c checks.cpp -o checks.o

md5.o: md5.cpp md5.hpp
	$(CC) $(CFLAGS) -c md5.cpp -o md5.o

$(PROJECT).o: $(PROJECT).cpp checks.hpp md5.hpp
	$(CC) $(CFLAGS) -c $(PROJECT).cpp -o $(PROJECT).o

$(PROJECT): checks.o md5.o  $(PROJECT).o
	$(CC) $(CFLAGS) checks.o md5.o $(PROJECT).o -o $(PROJECT)

clean:
	rm -f *.o
	rm -f $(PROJECT)
