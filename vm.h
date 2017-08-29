#include <linux/kvm.h>
#include <x86_64-linux-gnu/asm/kvm.h>

#ifndef _VM_
#define _VM_

/* VM */
struct vm {
    size_t ram_size;
    char *vm_ram;   /* Pointer to allocated RAM for the VM */
    int disk_fd;   /* File descriptor to the guest image file in raw format */
    int vm_fd;      /* File descriptor to the VM created by KVM API */
    int vcpu_fd;    /* File descriptor to VCPU */
    struct kvm_run *kvm_run; /* Used by KVM to communicate with the application level code */
};

#endif
