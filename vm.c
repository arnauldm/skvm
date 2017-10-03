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
