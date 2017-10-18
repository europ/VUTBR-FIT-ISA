LOGIN=xtotha01
CC=g++
CF=-Wall -Wextra -pedantic -std=c++11 -pthread
PROJECT=popser
TARFILES=$(PROJECT).cpp README Makefile manual.pdf

.PHONY: make
make:
	$(CC) $(CF) $(PROJECT).cpp -o $(PROJECT)

.PHONY: md5
md5:
	$(CC) $(CF) -lcrypto -lssl $(PROJECT).cpp -o $(PROJECT)

.PHONY: tar
tar:
	tar -cvf $(LOGIN).tar $(TARFILES)

.PHONY: test
test:
	@$(CC) $(CF) test.cpp -o test

.PHONY: clean
clean:
	@rm -f $(PROJECT)
	@rm -f $(LOGIN).tar
	@rm -f test
