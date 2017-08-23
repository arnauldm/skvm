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

#include "skvm.h"

extern void pexit (char *);

/* 
 * Dump registers
 */

void dump_sregs (int vcpu_fd)
{
    struct kvm_sregs sregs;

    if (ioctl (vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
        pexit ("KVM_GET_SREGS ioctl");

    fprintf (stderr,
             "  cs: 0x%x,\tds: 0x%x\n"
             "  es: 0x%x,\tss: 0x%x\n"
             "  fs: 0x%x,\tgs: 0x%x\n",
             sregs.cs.selector, sregs.ds.selector,
             sregs.es.selector, sregs.ss.selector,
             sregs.fs.selector, sregs.gs.selector);
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

void dump_real_mode_stack (int vcpu_fd)
{
    struct kvm_regs regs;
    struct kvm_sregs sregs;

    char* addr;

    if (ioctl (vcpu_fd, KVM_GET_REGS, &regs) < 0)
        pexit ("KVM_GET_REGS ioctl");

    if (ioctl (vcpu_fd, KVM_GET_SREGS, &sregs) < 0)
        pexit ("KVM_GET_SREGS ioctl");

    addr = vm_ram;
    addr += ((sregs.ss.selector & 0xffff) << 8);
    addr += (regs.rsp & 0xffff);

    for (int i=0;i<32;i+=2) 
        fprintf (stderr, "%p: 0x%x\n", 
            (void*) (&addr[i] - vm_ram), 
            (*(uint16_t*)(addr+i)) & 0xffff);

}

