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


void pexit (char *s)
{
    perror (s);
    exit (1);
}


/* 
 * Dump registers
 */

void dump_sregs (int vcpu_fd)
{
    struct kvm_sregs sregs;

    if (ioctl (vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
        pexit ("KVM_GET_SREGS ioctl");

    fprintf (stderr, "  cs: 0x%x\n", sregs.cs.selector);
}


void dump_regs (int vcpu_fd)
{
    struct kvm_regs regs;

    if (ioctl (vcpu_fd, KVM_GET_REGS, &regs) < 0)
        pexit ("KVM_GET_REGS ioctl");

    fprintf (stderr,
             "  rax: 0x%llx,\trbx: 0x%llx\n"
             "  rcx: 0x%llx,\trdx: 0x%llx\n"
             "  rsi: 0x%llx,\trdi: 0x%llx\n"
             "  rsp: 0x%llx,\trbp: 0x%llx\n"
             "  rip: 0x%llx,\trflags: 0x%llx\n",
             regs.rax, regs.rbx, regs.rcx, regs.rdx, regs.rsi, regs.rdi,
             regs.rsp, regs.rbp, regs.rip, regs.rflags);
}


