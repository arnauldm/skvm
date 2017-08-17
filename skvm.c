#include <fcntl.h>
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

#define KBYTE (1<<10)
#define MBYTE (1<<20)
#define GBYTE (1<<30)

/* CONFIGURATION BEGIN */

#define RAM_SIZE (32*MBYTE)    /* Memory in bytes */
#define KVM_FILE "/dev/kvm"     /* KVM special file */

#define IVT_ADDR        0x00000000
#define LOAD_ADDR       0x00001000  /* Guest */
#define BIOS_ADDR       0x000f0000
#define PCI_HOLE_ADDR   0xc0000000  /* PCI hole physical address */

/* CONFIGURATION END */

char *vm_ram;             /* Pointer to allocated RAM for the VM */

void pexit (char *s)
{
    perror (s);
    exit (1);
}

/* Guest physical address to host virtual address */
void *GPA_to_HVA (uint64_t offset)
{
    return vm_ram + offset;
}

int main (int argc, char **argv)
{
    int guest_file_fd;               /* File descriptor to the guest image file in raw format */
    int kvm_fd;                 /* Mainly file descriptor to "/dev/kvm" file */
    int vm_fd;                  /* File descriptor to the VM created by KVM API */
    int vcpu_fd;                /* File descriptor to VCPU */

    /* Used by KVM to communicate with the application level code. Should be
     * mmaped at offset 0 */
    struct kvm_run *kvm_run;
    int mmap_size;

    /* Guest registers (structs defined in /usr/include/x86_64-linux-gnu/asm/kvm.h) */
    struct kvm_sregs sregs;
    struct kvm_regs regs;

    /* The following KVM specific structs are defined in /usr/include/linux/kvm.h */
    struct kvm_pit_config pit_config = {.flags = 0 };
    struct kvm_userspace_memory_region region;

    int ret;
    int i;


    if (argc < 2) {
        fprintf (stderr, "usage : <img>\n");
        exit (1);
    }

    guest_file_fd = open (argv[1], O_RDONLY);
    if (guest_file_fd == -1)
        pexit ("open");

    /* Open "/dev/kvm" */
    kvm_fd = open (KVM_FILE, O_RDWR);
    if (kvm_fd == -1)
        pexit ("open");

    /* Create a vm */
    vm_fd = ioctl (kvm_fd, KVM_CREATE_VM, 0);
    if (vm_fd < 0)
        pexit ("KVM_CREATE_VM ioctl");

    /* Checking for some extensions */
    ret = ioctl (kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_PIT2);
    if (ret != 1)
        pexit ("KVM_CAP_PIT2 must be supported");

    ret = ioctl (kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_IRQCHIP);
    if (ret != 1)
        pexit ("KVM_CAP_IRQCHIP must be supported");

    ret = ioctl (kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_HLT);
    if (ret != 1)
        pexit ("KVM_CAP_HLT must be supported");

    /* Enabling in-kernel irqchip (PIC) and PIT */

    /*
    ret = ioctl (vm_fd, KVM_CREATE_PIT2, &pit_config);
    if (ret < 0)
        pexit ("KVM_CREATE_PIT2 ioctl");

    ret = ioctl (vm_fd, KVM_CREATE_IRQCHIP);
    if (ret < 0)
        pexit ("KVM_CREATE_IRQCHIP ioctl");
    */

    /* "This ioctl defines the physical address of a three-page region in the
     * guest physical address space.  The region must be within the first 4GB
     * of the guest physical address space and must not conflict with any
     * memory slot or any mmio address."
     * Note - shouldn't be needed without guest user tasks
     */
    ret = ioctl (vm_fd, KVM_SET_TSS_ADDR, 0xfffbd000);
    if (ret < 0)
        pexit ("KVM_SET_TSS_ADDR ioctl");

    /* Reserve and init RAM */
    /* mmap - MAP_NORESERVE to do not reserve swap space for this mapping */
    vm_ram =
        mmap (NULL, RAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    if ((void *) vm_ram == MAP_FAILED)
        pexit ("mmap");

    memset (vm_ram, 0, RAM_SIZE);

    /* We don't want to use KMS as it can leads to side channel attacks */
    madvise (vm_ram, RAM_SIZE, MADV_UNMERGEABLE);

    /* Allocating physical memory to the guest */
    region = (struct kvm_userspace_memory_region) {
        .slot = 0,                  /* bits 0-15 of "slot" specifies the slot id */
        .flags = 0,
        .guest_phys_addr = 0,       /* start of the VM physical memory */
        .memory_size = RAM_SIZE,    /* bytes */
        .userspace_addr = (uint64_t) vm_ram,  /* start of the userspace allocated memory */
    };

    ret = ioctl (vm_fd, KVM_SET_USER_MEMORY_REGION, &region);
    if (ret < 0)
        pexit ("KVM_SET_USER_MEMORY_REGION ioctl");

    /* Linux/Documentation/virtual/kvm/api.txt - "Application code obtains a
     * pointer to the kvm_run structure by mmap()ing a vcpu fd.  From that
     * point, application code can control execution by changing fields in
     * kvm_run prior to calling the KVM_RUN ioctl, and obtain information about
     * the reason KVM_RUN returned by looking up structure members." */

    vcpu_fd = ioctl (vm_fd, KVM_CREATE_VCPU, 0);        /* last param is the cpu_id */
    if (vcpu_fd < 0)
        pexit ("KVM_CREATE_VCPU ioctl");

    mmap_size = ioctl (kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (mmap_size < 0)
        pexit ("KVM_GET_VCPU_MMAP_SIZE ioctl");

    kvm_run =
        mmap (NULL, (size_t) mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu_fd,
              0);
    if (kvm_run == MAP_FAILED)
        pexit ("unable to mmap vcpu fd");


    /* Settings guest registers */
    ret = ioctl(vcpu_fd, KVM_GET_SREGS, &sregs);
    if (ret < 0)
        pexit ("KVM_GET_SREGS ioctl");

    sregs.cs.base = 0; /* FIXME */
    sregs.cs.selector = 0;

    ret = ioctl(vcpu_fd, KVM_SET_SREGS, &sregs);
    if (ret < 0)
        pexit ("KVM_SET_SREGS ioctl");

    ret = ioctl(vcpu_fd, KVM_GET_REGS, &regs);
    if (ret < 0)
        pexit ("KVM_GET_REGS ioctl");

    regs.rip = LOAD_ADDR;
    regs.rflags = 2; /* bit 1 is reserved and must be set */

    ret = ioctl(vcpu_fd, KVM_SET_REGS, &regs);
    if (ret < 0)
        pexit ("KVM_SET_REGS ioctl");

    /* Copy virtualized code here */
    i = LOAD_ADDR;
    do {
        ret = (int) read (guest_file_fd, &vm_ram[i], 4096);
        if (ret < 0)
            pexit ("read");
        i += ret;
    } while (ret > 0);


    while (1) {
        ret = ioctl(vcpu_fd, KVM_RUN, 0);
        if (ret < 0)
            pexit ("KVM_RUN ioctl");
    
        switch (kvm_run->exit_reason) {

            case KVM_EXIT_HLT:
                fprintf (stderr, "guest halted\n");
                return 0;

            case KVM_EXIT_IO:
                /* if output to default serial port COM0 */
                if (kvm_run->io.port == 0x3f8 && kvm_run->io.direction == KVM_EXIT_IO_OUT) {
                    write (STDERR_FILENO, (char*) (kvm_run) + kvm_run->io.data_offset, kvm_run->io.size);
                }
                else {
                    fprintf (stderr, "unhandled KVM_EXIT_IO (port: 0x%x)\n", kvm_run->io.port);
                }
                break;

            case KVM_EXIT_FAIL_ENTRY:
                fprintf (stderr, "KVM_EXIT_FAIL_ENTRY: fail_entry.hardware_entry_failure_reason: = 0x%llx\n",
                    kvm_run->fail_entry.hardware_entry_failure_reason);
                return 1;

            default:
                fprintf (stderr, "exit_reason = 0x%x\n", kvm_run->exit_reason);
                return 1;
        }
    }

    return 0;
}
