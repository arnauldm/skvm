#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/kvm.h>
#include <x86_64-linux-gnu/asm/kvm.h>

extern void pexit (char *);
extern void dump_sregs (int);
extern void dump_regs (int);
extern char *vm_ram;


void handle_serial (int, struct kvm_run*);
void handle_bios_int13 (int, struct kvm_run*);


void handle_KVM_EXIT_IO (int vcpu_fd, struct kvm_run *kvm_run)
{
    uint16_t cs, ip;

    switch (kvm_run->io.port) {
        /* default serial port COM0 */
        case 0x3f8:
            handle_serial (vcpu_fd, kvm_run);
            break;

        /* called by bios interrupt handler `int13_handler' to implement it in `skvm' */
        case 0xff13:
            handle_bios_int13 (vcpu_fd, kvm_run);
            break;

        default:
            fprintf (stderr, "skvm: unhandled KVM_EXIT_IO (port: 0x%x)\n",
                 kvm_run->io.port);
            dump_regs (vcpu_fd);
            ip = *((uint16_t*) (vm_ram+0xf0002));
            cs = *((uint16_t*) (vm_ram+0xf0004));
            fprintf (stderr, "  saved ip: 0x%x\n", ip);
            fprintf (stderr, "  saved cs: 0x%x\n", cs);
            exit (1);
    }
}

void handle_bios_int13 (int vcpu_fd, struct kvm_run *kvm_run)
{
    struct kvm_regs regs;
    uint8_t ah;
    uint16_t dx;
    uint16_t cs, ip;

    if (ioctl (vcpu_fd, KVM_GET_REGS, &regs) < 0)
        pexit ("KVM_GET_REGS ioctl");

    ah = (uint8_t) (regs.rax >> 8);

    switch (ah) {

        /* INT 13h AH=08h: Read Drive Parameters */
        case 0x08:
            /* Clear CF, no error */
            regs.rflags &= 0xfffe;
            /* AL = 00, disk status is 'no error' */
            regs.rax &= 0xff00;
            /* DH = heads 
             * DL = number of drives attached */
            regs.rdx = 0xff01;
            /* CX = cylinders [7:6] [15:8], sectors [0:5] */
            regs.rcx = 0xffff;

            if (ioctl (vcpu_fd, KVM_SET_REGS, &regs) < 0)
                pexit ("KVM_SET_REGS ioctl");
            break;

        /* INT 13h AH=41h: Check Extensions Present */
        case 0x41:
            regs.rflags |= 0x0001; /* Set CF if not present */
            if (ioctl (vcpu_fd, KVM_SET_REGS, &regs) < 0)
                pexit ("KVM_SET_REGS ioctl");
            break;

        default:
            fprintf (stderr, "skvm: unhandled INT 13h (port: 0x%x)\n",
                 kvm_run->io.port);
            dump_regs (vcpu_fd);
            dx = *((uint16_t*) vm_ram + 0xf0000);
            ip = *((uint16_t*) (vm_ram+0xf0002));
            cs = *((uint16_t*) (vm_ram+0xf0004));
            fprintf (stderr, "  saved dx: 0x%x\n", dx);
            fprintf (stderr, "  saved ip: 0x%x\n", ip);
            fprintf (stderr, "  saved cs: 0x%x\n", cs);
            exit (1);
    }
}


void handle_serial (int vcpu_fd, struct kvm_run *kvm_run)
{
    if (kvm_run->io.direction == KVM_EXIT_IO_OUT) {

        /* Linux/Documentation/virtual/kvm/api.txt - 
         * "data_offset describes where the data is located
         * (KVM_EXIT_IO_OUT) or where kvm expects application code to
         * place the data for the next KVM_RUN invocation
         * (KVM_EXIT_IO_IN)" */

        write (STDERR_FILENO,
               (char *) (kvm_run) + kvm_run->io.data_offset,
               kvm_run->io.size);

    } else {
        fprintf (stderr, "skvm: unhandled KVM_EXIT_IO (port: 0x%x)\n",
                 kvm_run->io.port);
        dump_regs (vcpu_fd);
        exit (1);
    }
}

