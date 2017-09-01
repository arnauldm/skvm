#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/kvm.h>
#include <x86_64-linux-gnu/asm/kvm.h>

#define __DEBUG__

#include "vm.h"
#include "skvm.h"
#include "skvm_exit.h"
#include "util.h"



int main (int argc, char **argv)
{
    struct vm guest;
    char *guest_file = NULL;
    char *bios_file = NULL;

    int bios_fd;                /* File descriptor to the BIOS rom file */
    int kvm_fd;                 /* Mainly file descriptor to "/dev/kvm" file */

    /* Guest registers 
     * (structs defined in /usr/include/x86_64-linux-gnu/asm/kvm.h) */
    struct kvm_sregs sregs;
    struct kvm_regs regs;

    /* The following KVM structs are defined in /usr/include/linux/kvm.h */
    /* struct kvm_pit_config pit_config = {.flags = 0 }; */
    struct kvm_userspace_memory_region region;

    /* Guest hard disk parameters */
    off_t disk_size;
    struct ebda_drive_chs_params *hd;

    int mmap_size;
    int ret, i;

    /*********************** 
     * Parsing command line 
     ***********************/

    while (1) {
        static struct option long_options[] = {
            {"bios", required_argument, 0, 'b'},
            {"guest", required_argument, 0, 'g'},
            {0, 0, 0, 0}
        };

        int index = 0;
        int c = getopt_long (argc, argv, "b:g:", long_options, &index);

        if (c == -1)
            break;

        switch (c) {
        case 'b':
            bios_file = optarg;
            break;
        case 'g':
            guest_file = optarg;
            break;
        default:
            break;
        }
    }

    if (bios_file == NULL || guest_file == NULL) {
        fprintf (stderr,
                 "usage: [OPTIONS]\n--bios, -b\tbios file\n"
                 "--guest, -g\tguest file\n");
        exit (1);
    }

    /*********************** 
     * Create and set a VM
     ***********************/

    /* Open "/dev/kvm" */
    kvm_fd = open (KVM_FILE, O_RDWR);
    if (kvm_fd == -1)
        pexit (KVM_FILE);

    /* Check for some KVM extensions */
    ret = ioctl (kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_PIT2);
    if (ret != 1)
        pexit ("KVM_CAP_PIT2 must be supported");

    ret = ioctl (kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_IRQCHIP);
    if (ret != 1)
        pexit ("KVM_CAP_IRQCHIP must be supported");

    ret = ioctl (kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_HLT);
    if (ret != 1)
        pexit ("KVM_CAP_HLT must be supported");

    /* Create the vm */
    guest.vm_fd = ioctl (kvm_fd, KVM_CREATE_VM, 0);
    if (guest.vm_fd < 0)
        pexit ("KVM_CREATE_VM ioctl");

    /* Enable in-kernel irqchip (PIC) and PIT */

    /*
       ret = ioctl (guest.vm_fd, KVM_CREATE_PIT2, &pit_config);
       if (ret < 0)
       pexit ("KVM_CREATE_PIT2 ioctl");

       ret = ioctl (guest.vm_fd, KVM_CREATE_IRQCHIP);
       if (ret < 0)
       pexit ("KVM_CREATE_IRQCHIP ioctl");
     */

    /* "This ioctl defines the physical address of a three-page region in the
     * guest physical address space.  The region must be within the first 4GB
     * of the guest physical address space and must not conflict with any
     * memory slot or any mmio address."
     * Note - shouldn't be needed without guest user tasks executing in ring3
     */
    ret = ioctl (guest.vm_fd, KVM_SET_TSS_ADDR, 0xfffbd000);
    if (ret < 0)
        pexit ("KVM_SET_TSS_ADDR ioctl");

    /* Reserve and init RAM */
    /* mmap - MAP_NORESERVE to do not reserve swap space for this mapping */
    guest.ram_size = RAM_SIZE;
    guest.vm_ram =
        mmap (NULL, guest.ram_size, PROT_READ | PROT_WRITE,
              MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    if ((void *) guest.vm_ram == MAP_FAILED)
        pexit ("mmap");

    memset (guest.vm_ram, 0, guest.ram_size);

    /* We don't want to use KMS as it can leads to side channel attacks */
    madvise (guest.vm_ram, guest.ram_size, MADV_UNMERGEABLE);

    /* Allocate physical memory to the guest 
     * Note - BIOS memory region is set as read-only for the guest kernel. It's
     * not really needeed as there's not real threat, but it's more "clean"
     * like that. */
    region = (struct kvm_userspace_memory_region) {
        .slot = 0,          /* bits 0-15 of "slot" specifies the slot id */
        .flags = 0,         /* none or KVM_MEM_READONLY or KVM_MEM_LOG_DIRTY_PAGES */
        .guest_phys_addr = 0,       /* start of the VM physical memory */
        .memory_size = guest.ram_size,      /* bytes */
        .userspace_addr = (uint64_t) guest.vm_ram,  /* start of the userspace allocated memory */
    };

    ret = ioctl (guest.vm_fd, KVM_SET_USER_MEMORY_REGION, &region);
    if (ret < 0)
        pexit ("KVM_SET_USER_MEMORY_REGION ioctl");

    /* Linux/Documentation/virtual/kvm/api.txt - "Application code obtains a
     * pointer to the kvm_run structure by mmap()ing a vcpu fd.  From that
     * point, application code can control execution by changing fields in
     * kvm_run prior to calling the KVM_RUN ioctl, and obtain information about
     * the reason KVM_RUN returned by looking up structure members." */

    guest.vcpu_fd = ioctl (guest.vm_fd, KVM_CREATE_VCPU, 0);    /* last param is the cpu_id */
    if (guest.vcpu_fd < 0)
        pexit ("KVM_CREATE_VCPU ioctl");

    mmap_size = ioctl (kvm_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (mmap_size < 0)
        pexit ("KVM_GET_VCPU_MMAP_SIZE ioctl");

    guest.kvm_run =
        mmap (NULL, (size_t) mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
              guest.vcpu_fd, 0);
    if (guest.kvm_run == MAP_FAILED)
        pexit ("unable to mmap vcpu fd");

    /* Set guest registers */
    ret = ioctl (guest.vcpu_fd, KVM_GET_SREGS, &sregs);
    if (ret < 0)
        pexit ("KVM_GET_SREGS ioctl");

    sregs.cs.base = 0;
    sregs.cs.selector = 0;

    ret = ioctl (guest.vcpu_fd, KVM_SET_SREGS, &sregs);
    if (ret < 0)
        pexit ("KVM_SET_SREGS ioctl");

    ret = ioctl (guest.vcpu_fd, KVM_GET_REGS, &regs);
    if (ret < 0)
        pexit ("KVM_GET_REGS ioctl");

    regs.rip = LOAD_ADDR;
    regs.rflags = 0x0002;       /* bit 1 is reserved and must be set */
    regs.rdx = 0x0080;          /* DL = 80h - 1st hard disk */

    ret = ioctl (guest.vcpu_fd, KVM_SET_REGS, &regs);
    if (ret < 0)
        pexit ("KVM_SET_REGS ioctl");

    /************************ 
     * Set BIOS environement 
     ************************/

    /* Set the real mode Interrupt Vector Table */
    /* selector, offset, vector */
    set_ivt (&guest, 0xf000, 0x1000, 0x00);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x01);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x02);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x03);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x04);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x05);     /* ? */
    set_ivt (&guest, 0xf000, 0x2000, 0x06);     /* Invalid opcode */
    set_ivt (&guest, 0xf000, 0x1000, 0x07);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x08);     /* System timer */
    set_ivt (&guest, 0xf000, 0x1000, 0x09);     /* Keyboard */
    set_ivt (&guest, 0xf000, 0x1000, 0x0A);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x0B);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x0C);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x0D);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x0E);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x0F);     /* ? */

    set_ivt (&guest, 0xf000, 0xf000, 0x10);     /* VGA */

    set_ivt (&guest, 0xf000, 0x1000, 0x11);     /* Equipment list */
    set_ivt (&guest, 0xf000, 0x1000, 0x12);     /* Memory size */

    set_ivt (&guest, 0xf000, 0xe000, 0x13);     /* disk */

    set_ivt (&guest, 0xf000, 0x1000, 0x14);     /* Serial communication */
    set_ivt (&guest, 0xf000, 0x3000, 0x15);     /* System services */
    set_ivt (&guest, 0xf000, 0x1800, 0x16);     /* Key stroke */
    set_ivt (&guest, 0xf000, 0x1000, 0x17);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x18);     /* ? */
    set_ivt (&guest, 0xf000, 0x1000, 0x19);     /* Boot load */
    set_ivt (&guest, 0xf000, 0x4000, 0x1A);     /* Real Time Clock Services */

    /* Copy BIOS in memory */
    bios_fd = open (bios_file, O_RDONLY);
    if (bios_fd == -1)
        pexit (bios_file);

    i = BIOS_ADDR;
    do {
        ret = (int) read (bios_fd, &guest.vm_ram[i], 4096);
        if (ret < 0)
            pexit ("read");
        i += ret;

        /* BIOS binary must not overlap 0x100000 barrier */
        if (i > 0x100000)
            break;
    } while (ret > 0);

    /* Open the guest disk image */
    guest.disk_fd = open (guest_file, O_RDONLY);
    if (guest.disk_fd == -1)
        pexit (guest_file);

    /* Load the disk's MBR (the first 512 bytes) at 0x7C00 */
    ret = disk_read (&guest, gpa_to_hva (&guest, LOAD_ADDR), 0, 1);
    if (ret < 0)
        pexit ("read");

    /* Copy hard disk parameters to the Extended BIOS Data Area (EBDA) */
    disk_size = lseek (guest.disk_fd, 0, SEEK_END);
    if (disk_size < 0)
        pexit ("lseek");

    fprintf (stderr, "disk size: %ld bytes (%ld MB)\n",
        disk_size, disk_size / 0x100000);

    hd = (struct ebda_drive_chs_params *)
        gpa_to_hva (&guest, EBDA_ADDR + EBDA_DISK1_OFFSET);

    hd->head = 255;
    hd->sectors_per_track = 63;
    hd->cyl = (uint16_t)
        (disk_size / (hd->head * hd->sectors_per_track * 512));

    if (hd->cyl > 1023)
        pexit ("disk too big");

    fprintf (stderr, "disk geometry: CHS = %d / %d / %d (absolutes sectors: %ld)\n",
        hd->cyl, hd->head, hd->sectors_per_track, disk_size/512);

    /* Set starting time (in clock ticks) */
    struct tms dummy;
    guest.clock_start = times (&dummy);

    /**************
     * Run the VM 
     **************/

    while (1) {
        ret = ioctl (guest.vcpu_fd, KVM_RUN, 0);
        if (ret < 0)
            pexit ("KVM_RUN ioctl");

        switch (guest.kvm_run->exit_reason) {

        case KVM_EXIT_HLT:
            handle_exit_hlt ();
            return 0;

        case KVM_EXIT_IO:
            handle_exit_io (&guest);
            break;

        case KVM_EXIT_FAIL_ENTRY:
            fprintf (stderr,
                     "KVM_EXIT_FAIL_ENTRY: fail_entry.hardware_entry_failure_reason: = 0x%llx\n",
                     guest.kvm_run->fail_entry.
                     hardware_entry_failure_reason);
            return 1;

        case KVM_EXIT_MMIO:
            fprintf (stderr,
                     "KVM_EXIT_MMIO: guest trying to overwrite read-only memory\n");
            return 1;

        default:
            fprintf (stderr, "exit_reason = 0x%x\n",
                     guest.kvm_run->exit_reason);
            return 1;
        }
    }

    return 0;
}
