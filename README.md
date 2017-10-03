# skvm

THAT PROJECT IS IN EARLY DEVELOPMENT PHASE AND DOES NOT WORK

## Description

Playing with KVM API and trying to implement a tiny *Virtual Machine
Monitor (VMM)*, in userland, just like Qemu.

## Compile

You will need :

  * `gcc`, to compile the VMM
  * `nasm`, to compile guest example code
  * `bcc`, aka. *Bruce's C Compiler*, to compile the example BIOS

The command `make` will compile the project, without launching it :

```Shell
  gcc -Wall -Wextra -Wconversion -fstack-protector-all -g -c -o build/skvm.o skvm.c 
  gcc -Wall -Wextra -Wconversion -fstack-protector-all -g -c -o build/skvm_exit.o skvm_exit.c 
  gcc -Wall -Wextra -Wconversion -fstack-protector-all -g -c -o build/skvm_debug.o skvm_debug.c 
  gcc -Wall -Wextra -Wconversion -fstack-protector-all -g -o build/skvm build/skvm.o build/skvm_exit.o build/skvm_debug.o
  nasm -f bin -o build/guest guest16.asm
  bcc -W -0 -S -o build/bios.s bios/bios.c
  as86 -b build/minibios build/bios.s
```

## Execute

To compile and launch our own virtual machine monitor (VMM) in a row, just type
and execute :

```Shell
  make run
```

To launch it, you can execute `make run` or, at command line :

```Shell
  ./build/skvm --guest build/guest --bios build/minibios --serial /tmp/skvm.sock
```

Note that you can play with some other guest and/or BIOS implementations. For
example :

```Shell
  ./build/skvm \
  --bios /opt/bochs-2.6.9/share/bochs/BIOS-bochs-legacy \
  --guest ~/vm/guest.raw \
  --serial /tmp/skvm.sock
```

You can also use `minicom` to display guest usage of the serial port :

```Shell
  minicom -D unix\#/tmp/skvm.sock
```

# Components

The project compile 3 differents elements :

  * `build/skvm` is the VMM, that uses KVM API and run in Linux user space
  * `build/guest` is a tiny guest code, loaded and executed at address 0x7c00 
  * `build/minibios` is a tiny BIOS implementation. It's goal is to respond to
    INT instructions executed by the guest code.

Note that *minibios* only implements BIOS interrupts code (to respond to INT
instructions). Routines that set *Interrupt Vectors Table (IVT)*, *Extended
Bios Data Area (EBDA)* and some other stuffs are managed by the VMM.

