
GCC = gcc -Wall -Wextra -Wconversion -fstack-protector-all
GCC += -O2

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
INC = $(wildcard *.h)

all: skvm

main: $(OBJ) skvm.c
	$(GCC) $^ -o $@

%.o: %.c
	$(GCC) -c $^ 

indent:
	indent -kr -nut -pcs $(SRC) $(INC)

clean:
	rm $(OBJ) main
