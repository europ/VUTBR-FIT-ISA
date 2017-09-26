.PHONY: make

CC=g++
CF=-Wall -Wextra -pedantic -std=c++11
PROJ=popser
TARFILES=$(PROJ).cpp README Makefile manual.pdf
TARNAME=xtotha01

$(PROJ): $(PROJ).cpp
	$(CC) $(CF) $(PROJ).cpp -o $(PROJ)

tar:
	tar -cvf $(TARNAME).tar $(TARFILES)

clean:
	rm -f $(PROJ)
	rm -f $(TARNAME).tar
