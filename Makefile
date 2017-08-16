
GCC = gcc -Wall -Wextra -Wconversion -fstack-protector-all
GCC += -O2

BUILD = build

SRC = $(wildcard *.c)
INC = $(wildcard *.h)
OBJ = $(patsubst %.c, $(BUILD)/%.o, $(SRC))
BIN = $(BUILD)/skvm

all: $(BUILD) $(BIN)

$(BUILD):
	mkdir $@

$(BIN): $(OBJ) 
	$(GCC) $^ -o $@

$(BUILD)/%.o: %.c
	$(GCC) -c -o $@ $^ 

indent:
	indent -kr -nut -pcs $(SRC) $(INC)

clean:
	rm -f $(BUILD)/*
