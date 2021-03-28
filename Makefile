CC=gcc
CFLAGS= -O3

BIN_DIR = bin
SRC_DIR = src

$(info $(shell mkdir -p $(BIN_DIR)))

default: compile

compile:
	$(CC) $(CFLAGS) -o $(BIN_DIR)/prod-cons $(SRC_DIR)/prod-cons.c -lpthread
	@printf "\n"

run:
	./$(BIN_DIR)/prod-cons
	@printf "\n"

clean: 
	rm -rf $(BIN_DIR)
