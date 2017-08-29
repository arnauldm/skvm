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
#include "skvm_exit.h"
#include "skvm_debug.h"
#include "skvm.h"


void handle_exit_io (int vcpu_fd, struct kvm_run *kvm_run)
{
    switch (kvm_run->io.port) {

        case SERIAL_PORT:
            handle_exit_io_serial (vcpu_fd, kvm_run);
            break;

        case HYPERCALL_PORT:
            handle_exit_io_hypercall (vcpu_fd, kvm_run);
            break;

        default:
            fprintf (stderr, "skvm: unhandled KVM_EXIT_IO (port: 0x%x)\n",
                 kvm_run->io.port);
            dump_regs (vcpu_fd);
            dump_sregs (vcpu_fd);
            dump_real_mode_stack (vcpu_fd);
            exit (1);
    }
}


void handle_exit_io_hypercall (int vcpu_fd, struct kvm_run *kvm_run)
{
    uint8_t data = *((uint8_t *) (kvm_run) + kvm_run->io.data_offset);

    switch (data) {
        case HC_PANIC:
            fprintf (stderr, "BIOS panic\n");
            dump_ebda_regs ();
            dump_real_mode_stack (vcpu_fd);
            exit (1);
            break;

        case HC_BIOS:
            handle_bios_interrupt (vcpu_fd);
            break;

        default:
            fprintf (stderr, "HYPERCALL not implemented (0x%x)\n", data);
    }

}


void handle_bios_interrupt (int vcpu_fd)
{
    uint32_t dap_addr, buffer_addr;
    struct disk_address_packet *dap;

    struct ebda_registers *regs = (struct ebda_registers*) 
        (vm_ram + EBDA_ADDR + EBDA_REGS_OFFSET);

    switch (HBYTE(regs->ax)) {
        case 0x41:
            regs->flags &= 0xFFFE; // Clear CF
            regs->ax &= 0x00FF; // AH = 0x00
            regs->bx = 0xAA55;
            regs->cx = 0x0001;
            break;

        case 0x42:
            dap_addr = ((uint32_t) regs->ds << 4) + (uint32_t) regs->si;
            dap = (struct disk_address_packet*) (vm_ram + dap_addr);

            buffer_addr = ((dap->buffer >> 12) & 0xFFFF0) + (dap->buffer & 0xFFFF);

            fprintf (stderr, "disk address packet: 0x%x (%x:%x)\n", dap_addr, regs->ds, regs->si);
            fprintf (stderr, "count: %d, buffer: %x (0x%lx), sector: %ld\n", 
                dap->count, dap->buffer, buffer_addr, dap->sector);


        default:
            fprintf (stderr, "handle_bios_interrupt: INT %xh not implemented\n", HBYTE(regs->ax));
            dump_ebda_regs ();
            dump_real_mode_stack (vcpu_fd);
            exit (1);
    }
}


void handle_exit_io_serial (int vcpu_fd, struct kvm_run *kvm_run)
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


void handle_exit_hlt (void)
{
    fprintf (stderr, "Guest halted\n");
    exit (0);
}


