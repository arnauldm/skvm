
GCC = gcc -Wall -Wextra -Wconversion -fstack-protector-all
#GCC += -O2
GCC += -g

BUILD = build

BIN = $(BUILD)/skvm
GUEST = $(BUILD)/guest
BIOS = $(BUILD)/minibios

all: $(BUILD) $(BIN) $(GUEST) $(BIOS)

$(BUILD):
	mkdir $@

$(BIN): $(BUILD)/skvm.o $(BUILD)/skvm_exit.o $(BUILD)/skvm_debug.o
	$(GCC) -o $@ $^

$(BUILD)/%.o: %.c
	$(GCC) -c -o $@ $^ 

$(GUEST): guest16.asm
	nasm -f bin -o $@ $^

$(BIOS): bios/bios.c
	bcc -W -0 -S -o $(BUILD)/bios.s $^
	as86 -b $@ $(BUILD)/bios.s

indent:
	indent -kr -nut -pcs *.c 

clean:
	rm -f $(BUILD)/* $(BIOS) *~

run: $(BIN) $(GUEST) $(BIOS)
	@echo launch: $(BIN) --guest $(GUEST) --bios $(BIOS)
	@$(BIN) --guest $(GUEST) --bios $(BIOS)
