#include <stdint.h>
#include <sys/times.h>

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
    clock_t clock_start;        /* Start time in clock ticks */
};

#endif


#ifndef _VM_
extern void *gpa_to_hva (struct vm *, uint64_t);
uint32_t rmode_to_gpa (uint16_t, uint16_t);
extern void set_ivt (struct vm *, uint16_t, uint16_t, uint16_t);
#endif
