.PHONY: make

CC=g++
CF=-Wall -Wextra -pedantic -std=c++11 -pthread
PROJ=popser
TARFILES=$(PROJ).cpp README Makefile manual.pdf
TARNAME=xtotha01

$(PROJ): $(PROJ).cpp
	$(CC) $(CF) $(PROJ).cpp -o $(PROJ)

tar:
	tar -cvf $(TARNAME).tar $(TARFILES)

md5:
	g++ -Wall -Wextra -pedantic -std=c++11 -pthread -lcrypto -lssl md5.cpp -o md5

clean:
	rm -f $(PROJ)
	rm -f $(TARNAME).tar
	rm -f md5
