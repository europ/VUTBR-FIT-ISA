CC=g++
CFLAGS=-Wall -Wextra -pedantic -std=c++11 -pthread -lcrypto -lssl
PROJECT=popser

all: $(PROJECT)

fsm.o: fsm.cpp fsm.hpp
	$(CC) $(CFLAGS) -c fsm.cpp -o fsm.o

md5.o: md5.cpp md5.hpp
	$(CC) $(CFLAGS) -c md5.cpp -o md5.o

checks.o: checks.cpp checks.hpp
	$(CC) $(CFLAGS) -c checks.cpp -o checks.o

argpar.o: argpar.cpp argpar.hpp
	$(CC) $(CFLAGS) -c argpar.cpp -o argpar.o

logger.o: logger.cpp logger.hpp
	$(CC) $(CFLAGS) -c logger.cpp -o logger.o

datatypes.o: datatypes.cpp datatypes.hpp
	$(CC) $(CFLAGS) -c datatypes.cpp -o datatypes.o

$(PROJECT).o: $(PROJECT).cpp fsm.hpp md5.hpp checks.hpp argpar.hpp logger.hpp datatypes.hpp
	$(CC) $(CFLAGS) -c $(PROJECT).cpp -o $(PROJECT).o

$(PROJECT): fsm.o md5.o checks.o argpar.o logger.o datatypes.o  $(PROJECT).o
	$(CC) $(CFLAGS) fsm.o md5.o checks.o argpar.o logger.o datatypes.o $(PROJECT).o -o $(PROJECT)

clean:
	rm -f $(PROJECT) *.o

clean-o:
	rm -f *.o
