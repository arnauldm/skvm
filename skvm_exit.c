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

        /* default serial port COM0 */
        case 0x3f8:
            handle_exit_io_serial (vcpu_fd, kvm_run);
            break;

        case PANIC_PORT:
            handle_exit_io_panic (vcpu_fd, kvm_run);
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


void handle_exit_io_panic (int vcpu_fd, struct kvm_run *kvm_run)
{
    fprintf (stderr, "BIOS panic at bios.c, line %d\n",
        *((uint16_t*)((char*) kvm_run + kvm_run->io.data_offset)));

    dump_ebda_regs ();
    dump_real_mode_stack (vcpu_fd);
    exit (1);
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


