#include <stdio.h>
#include <stdlib.h>

#include <linux/kvm.h>
#include <x86_64-linux-gnu/asm/kvm.h>

/* CONFIGURATION BEGIN */

/* Memory in MBytes */
#define RAM_SIZE 512 

/* CONFIGURATION END */

#define KVM_FILE "/dev/kvm"

void pexit (char *s) {
    perror(s);
    exit(1);
}

int main(int ac, char **av) {
    char *guest_file;
    int kvm_fd;
    int vm_fd;
    struct kvm_pit_config pit_config = { .flags = 0 };
    char vm_ram_start;

    if (ac<2) {
        fprintf (stderr, "usage : <img>\n");
        exit (1);
    }

    guest_file = argv[1];

    /* open "/dev/kvm" */
    kvm_fd = open (KVM_FILE, O_RDWR); 
    if (kvm_fd == -1) 
        pexit ("open");

    /* create a vm */
    vm_fd = ioctl (kvm_fd, KVM_CREATE_VM, 0);
    if (vm_fd < 0) 
        pexit ("KVM_CREATE_VM ioctl");

    /* setting TSS address. I do not know why it's needed. Cf.
     * linux/Documentation 
     */
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

    /* reserve RAM 
     *   MAP_NORESERVE - Do not reserve swap space for this mapping.
     */
    vm_ram_start = mmap(NULL, RAM_SIZE, PROT_RW, MAP_ANON_NORESERVE, -1, 0); 
    if (vm_ram_start < MAP_FAILED)
        pexit ("mmap");

    madvise(vm_ram_start, RAM_SIZE, MADV_UNMERGEABLE);



    // kvm__init_ram()
    // kvm__load_kernel / kvm__load_firmware

    return 0;
}
