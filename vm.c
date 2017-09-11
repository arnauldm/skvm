#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define _VM_
#include "vm.h"
#include "skvm.h"

/* Guest physical address (GPA) to host virtual address (HVA) */
void *gpa_to_hva (struct vm *guest, uint64_t offset)
{
    return guest->vm_ram + offset;
}

/* Setting Interrupt Vector Table (IVT) entry */
void set_ivt (struct vm *guest, uint16_t cs, uint16_t offset,
              uint16_t vector)
{
    struct ivt_entry *ivt = (struct ivt_entry *) gpa_to_hva (guest, 0);
    ivt[vector].cs = cs;
    ivt[vector].offset = offset;
}

uint32_t rmode_to_gpa (uint16_t segment, uint16_t offset)
{
    return ((uint32_t) segment << 4) + (uint32_t) offset;
}


/* Read sectors of the guest's disk 
 * buffer - HVA of the buffer
 * sector - starting sector (1st sector is 0)
 * count  - how many sectors 
 */
ssize_t disk_read (struct vm * guest, char *buffer, size_t sector,
                   size_t count)
{
    if (lseek (guest->disk_fd, (off_t) sector * 512, SEEK_SET) < 0)
        return -1;

    return read (guest->disk_fd, buffer, count * 512);
}

void console_out (uint8_t c)
{
    putchar (c);
}
