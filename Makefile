CC=g++
CF=-Wall -Wextra -pedantic -std=c++11 -pthread -lcrypto -lssl
LOGIN=xtotha01
PROJECT=popser
TARFILES=$(PROJECT).cpp README Makefile manual.pdf

.PHONY: make
make:
	$(CC) $(CF) $(PROJECT).cpp -o $(PROJECT)

.PHONY: hash
hash:
	$(CC) $(CF) -lcrypto -lssl stuff/hash.cpp -o hash
	@echo "USAGE:  ./hash \"greeting_banner\" \"password\""
	@echo "OUTPUT: \"MD5_HASH_STRING\""

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
	@rm -f hash
	@rm -f test
