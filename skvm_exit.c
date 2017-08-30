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


#define __EXIT_IO__
#include "vm.h"
#include "util.h"
#include "skvm.h"
#include "skvm_exit.h"
#include "skvm_debug.h"


void handle_exit_io (struct vm *guest)
{
    switch (guest->kvm_run->io.port) {

        case SERIAL_PORT:
            handle_exit_io_serial (guest);
            break;

        case HYPERCALL_PORT:
            handle_exit_io_hypercall (guest);
            break;

        default:
            fprintf (stderr, "unhandled KVM_EXIT_IO (port: 0x%x)\n",
                 guest->kvm_run->io.port);
            dump_regs (guest);
            dump_sregs (guest);
            dump_real_mode_stack (guest);
            exit (1);
    }
}


void handle_exit_io_hypercall (struct vm *guest)
{
    uint8_t data = *((uint8_t *) (guest->kvm_run) + guest->kvm_run->io.data_offset);

    switch (data) {
        case HC_PANIC:
            fprintf (stderr, "BIOS panic\n");
            dump_ebda_regs (guest);
            dump_real_mode_stack (guest);
            exit (1);

        case HC_BIOS_INT10:
            handle_bios_int10 (guest);
            break;

        case HC_BIOS_INT13:
            handle_bios_int13 (guest);
            break;

        case HC_BIOS_INT15:
            handle_bios_int15 (guest);
            break;

        default:
            fprintf (stderr, "HYPERCALL not implemented (0x%x)\n", data);
    }

}


void handle_bios_int10 (struct vm *guest)
{
    struct ebda_registers *regs = (struct ebda_registers*) 
        (guest->vm_ram + EBDA_ADDR + EBDA_REGS_OFFSET);

    switch (HBYTE(regs->ax)) {
        case 0x0E:
            putchar (BYTE(regs->ax));
            break;
        default:
            fprintf (stderr, "handle_bios_int10(): AH=%Xh not implemented\n", HBYTE(regs->ax));
            dump_ebda_regs (guest);
            exit (1);
    }
}


void handle_bios_int13 (struct vm *guest)
{
    uint32_t dap_GPA, buffer_GPA; // Guest Physical Address (GPA)
    struct disk_address_packet *dap;
    int ret;

    struct ebda_registers *regs = (struct ebda_registers*) 
        (guest->vm_ram + EBDA_ADDR + EBDA_REGS_OFFSET);

    switch (HBYTE(regs->ax)) {
        case 0x00:
            fprintf (stderr, "int 13h, ah=00h\n");
            regs->flags &= 0xFFFE; // Clear CF
            regs->ax &= 0x00FF; // AH = 0x00
            break;

        case 0x41:
            fprintf (stderr, "int 13h, ah=41h\n");
            regs->flags &= 0xFFFE; // Clear CF
            regs->ax &= 0x00FF; // AH = 00h
            regs->bx = 0xAA55;
            regs->cx = 0x0001; // extended disk access functions (AH=42h-44h,47h,48h) supported

            // Status of last hard disk drive operation = OK
            *((uint8_t*)GPA_to_HVA(guest, BDA_ADDR + 0x74)) = 0x00; 

            break;

        case 0x42:
            fprintf (stderr, "int 13h, ah=42h\n");
            dap_GPA = ((uint32_t) regs->ds << 4) + (uint32_t) regs->si;
            dap = (struct disk_address_packet*) (guest->vm_ram + dap_GPA);

            buffer_GPA = ((dap->buffer >> 12) & 0xFFFF0) + (dap->buffer & 0xFFFF);

            fprintf (stderr, "disk address packet: 0x%x (%x:%x)\n", dap_GPA, regs->ds, regs->si);
            fprintf (stderr, "count: %d, buffer: %x (0x%x), sector: %ld\n", 
                dap->count, dap->buffer, buffer_GPA, dap->sector);

            ret = disk_read (guest, GPA_to_HVA(guest, buffer_GPA), dap->sector, dap->count);
            if (ret < 0) 
                pexit ("disk_read()");

            regs->flags &= 0xFFFE; // Clear CF
            regs->ax &= 0x00FF; // AH = 0x00

            // Status of last hard disk drive operation = OK
            *((uint8_t*)GPA_to_HVA(guest, BDA_ADDR + 0x74)) = 0x00; 

            break;

        default:
            fprintf (stderr, "handle_bios_int13(): AH=%Xh not implemented\n", HBYTE(regs->ax));
            dump_ebda_regs (guest);
            dump_real_mode_stack (guest);
            exit (1);
    }
}


void handle_bios_int15 (struct vm *guest)
{
    struct ebda_registers *regs = (struct ebda_registers*) 
        (guest->vm_ram + EBDA_ADDR + EBDA_REGS_OFFSET);
    fprintf (stderr, "handle_bios_int15(): AH=%Xh not implemented\n", HBYTE(regs->ax));
    dump_ebda_regs (guest);
    exit (1);
}


void handle_exit_io_serial (struct vm *guest)
{
    if (guest->kvm_run->io.direction == KVM_EXIT_IO_OUT) {

        /* Linux/Documentation/virtual/kvm/api.txt - 
         * "data_offset describes where the data is located
         * (KVM_EXIT_IO_OUT) or where kvm expects application code to
         * place the data for the next KVM_RUN invocation
         * (KVM_EXIT_IO_IN)" */

        write (STDERR_FILENO,
               (char *) (guest->kvm_run) + guest->kvm_run->io.data_offset,
               guest->kvm_run->io.size);

    } else {
        fprintf (stderr, "unhandled KVM_EXIT_IO (port: 0x%x)\n",
                 guest->kvm_run->io.port);
        dump_regs (guest);
        exit (1);
    }
}


void handle_exit_hlt (void)
{
    fprintf (stderr, "Guest halted\n");
    exit (0);
}


