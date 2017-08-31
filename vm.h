#include <stdint.h>

#include <linux/kvm.h>
#include <x86_64-linux-gnu/asm/kvm.h>

#ifndef _STRUCT_VM_
#define _STRUCT_VM_

/* VM */
struct vm {
    size_t ram_size;
    char *vm_ram;               /* Pointer to allocated RAM for the VM */
    int disk_fd;                /* File descriptor to the guest image file in raw format */
    int vm_fd;                  /* File descriptor to the VM created by KVM API */
    int vcpu_fd;                /* File descriptor to VCPU */
    struct kvm_run *kvm_run;    /* Used by KVM to communicate with the application level code */
};

#endif


#ifndef _VM_
extern void *GPA_to_HVA (struct vm *, uint64_t);
extern void set_ivt (struct vm *, uint16_t, uint16_t, uint16_t);
extern int disk_read (struct vm *, char *, size_t, size_t);
extern void console_out (uint8_t);
#endif
