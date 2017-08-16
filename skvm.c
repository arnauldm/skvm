#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/kvm.h>
#include <x86_64-linux-gnu/asm/kvm.h>

#define KBYTE (1<<10)
#define MBYTE (1<<20)
#define GBYTE (1<<30)

/* CONFIGURATION BEGIN */

#define RAM_SIZE (512*MBYTE)    /* Memory in bytes */
#define KVM_FILE "/dev/kvm"     /* KVM special file */

#define IVT_ADDR        0x00000000
#define BIOS_ADDR       0x000f0000
#define PCI_HOLE_ADDR   0xc0000000 /* PCI hole physical address */

/* CONFIGURATION END */


char *vm_ram_start;     /* pointer to allocated RAM for the VM */


void pexit (char *s) {
    perror(s);
    exit(1);
}

/* guest physical address to host virtual address */
void* GPA_to_HVA (uint64_t offset) {
    return vm_ram_start + (char*) offset;
}

int main(int argc, char **argv) {
    char *guest_image_file; /* guest image file in raw format */
    int kvm_fd;             /* mainly file descriptor to "/dev/kvm" file */
    int vm_fd;              /* file descriptor to the VM created by KVM API */
    int vcpu_fd;            /* file descriptor to VCPU */

    /* used by KVM to communicate with the application level code. should be
     * mmaped at offset 0 */
    struct kvm_run *kvm_run;
    int mmap_size;

    int ret;


    /* The following KVM specific structs are defined in /usr/include/linux/kvm.h */
    struct kvm_pit_config pit_config = { .flags = 0 };
    struct kvm_userspace_memory_region region;


    if (argc<2) {
        fprintf (stderr, "usage : <img>\n");
        exit (1);
    }

    guest_image_file = argv[1];

    /* open "/dev/kvm" */
    kvm_fd = open (KVM_FILE, O_RDWR); 
    if (kvm_fd == -1) 
        pexit ("open");

    /* create a vm */
    vm_fd = ioctl (kvm_fd, KVM_CREATE_VM, 0);
    if (vm_fd < 0) 
        pexit ("KVM_CREATE_VM ioctl");

    /* setting TSS address. I do not know why it's needed. Cf.
     * linux/Documentation */
    ret = ioctl (vm_fd, KVM_SET_TSS_ADDR, 0xfffbd000);
    if (ret < 0) 
        pexit ("KVM_SET_TSS_ADDR ioctl");

    /* enabling in-kernel irqchip */
    ret = ioctl (vm_fd, KVM_CREATE_IRQCHIP);
    if (ret < 0) 
        pexit ("KVM_CREATE_IRQCHIP ioctl");

    // [FIXME] why we need this ?
    ret = ioctl (vm_fd, KVM_CREATE_PIT2, &pit_config);
    if (ret < 0) 
        pexit ("KVM_CREATE_PIT2 ioctl");

    /*  
     *  Reserve and init RAM  
     */

    /* Note - if the VM memory (RAM) size is greater than 3GB, we must add some
     * extra space for the PCI MMIO hole (cf. e820 table) */

    /* mmap - MAP_NORESERVE to do not reserve swap space for this mapping */
    vm_ram_start = mmap(NULL, RAM_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_NORESERVE, -1, 0); 
    if ((void*) vm_ram_start < MAP_FAILED)
        pexit ("mmap");

    memset (vm_ram_start, 0, RAM_SIZE);

    /* we don't want to use KMS */
    madvise(vm_ram_start, RAM_SIZE, MADV_UNMERGEABLE);

    /* allocating physical memory to the guest */

    region = (struct kvm_userspace_memory_region) {
        .slot = 0,                                  /* bits 0-15 of "slot" specifies the slot id */
        .flags = 0,
        .guest_phys_addr = 0,                       /* start of the VM physical memory */
        .memory_size = RAM_SIZE,                    /* bytes */
        .userspace_addr = (uint64_t) vm_ram_start,  /* start of the userspace allocated memory */
    };    

    ret = ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region);
    if (ret < 0) 
        pexit ("KVM_SET_USER_MEMORY_REGION ioctl");


    /* copy bios rom */

    /* setup some simple E820 memory map */

    /* setup VGA ROM */

    /* setup the real mode vector table */


    /*
     *  Setup the vCPU 
     */

    /* Linux/Documentation/virtual/kvm/api.txt - "Application code obtains a
     * pointer to the kvm_run structure by mmap()ing a vcpu fd.  From that
     * point, application code can control execution by changing fields in
     * kvm_run prior to calling the KVM_RUN ioctl, and obtain information about
     * the reason KVM_RUN returned by looking up structure members." */

    vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 0); /* last param is the cpu_id */
    if (vcpu_fd < 0) 
        pexit ("KVM_CREATE_VCPU ioctl");

    mmap_size = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (mmap_size < 0)
        pexit ("KVM_GET_VCPU_MMAP_SIZE ioctl");

    kvm_run = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, vcpu_fd, 0);
    if (kvm_run == MAP_FAILED)
        pexit ("unable to mmap vcpu fd");


    return 0;
}

