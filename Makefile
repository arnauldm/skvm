
GCC = gcc -Wall -Wextra -Wconversion -fstack-protector-all
#GCC += -O2
GCC += -g

BUILD = build

SRC = $(wildcard *.c)
INC = $(wildcard *.h)
OBJ = $(patsubst %.c, $(BUILD)/%.o, $(SRC))
BIN = $(BUILD)/skvm
GUEST = $(BUILD)/guest

all: $(BUILD) $(BIN) $(GUEST)

$(BUILD):
	mkdir $@

$(BIN): $(OBJ) 
	$(GCC) -o $@ $^

$(BUILD)/%.o: %.c
	$(GCC) -c -o $@ $^ 

$(GUEST): code16.asm
	nasm -f bin -o $@ $^

indent:
	indent -kr -nut -pcs $(SRC) $(INC)

clean:
	rm -f $(BUILD)/*
