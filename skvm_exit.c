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
#include <sys/times.h>
#include <unistd.h>

#include <linux/kvm.h>
#include <x86_64-linux-gnu/asm/kvm.h>


#define __EXIT_IO__
#include "vm.h"
#include "util.h"
#include "skvm.h"
#include "skvm_exit.h"
#include "skvm_debug.h"

#define false 0
#define true  1
#define DEBUG_INT10 false
#define DEBUG_INT13 false
#define DEBUG_INT15 true
#define DEBUG_INT16 true
#define DEBUG_INT1A true


void handle_exit_io (struct vm *guest)
{
    switch (guest->kvm_run->io.port) {

    case SERIAL_PORT:
        handle_exit_io_serial (guest);
        break;

    case HYPERCALL_PORT:
        handle_exit_io_hypercall (guest);
        break;

    default:
        fprintf (stderr, "unhandled KVM_EXIT_IO (port: 0x%x)\n",
                 guest->kvm_run->io.port);
        dump_regs (guest);
        dump_sregs (guest);
        dump_real_mode_stack (guest);
        exit (1);
    }
}


void handle_exit_io_hypercall (struct vm *guest)
{
    uint8_t data =
        *((uint8_t *) (guest->kvm_run) + guest->kvm_run->io.data_offset);

    switch (data) {
    case HC_PANIC:
        fprintf (stderr, "BIOS panic\n");
        dump_ebda_regs (guest);
        //dump_real_mode_stack (guest);
        exit (1);

    case HC_BIOS_INT10:
        handle_bios_int10 (guest);
        break;

    case HC_BIOS_INT13:
        handle_bios_int13 (guest);
        break;

    case HC_BIOS_INT15:
        handle_bios_int15 (guest);
        break;

    case HC_BIOS_INT16:
        handle_bios_int16 (guest);
        break;

    case HC_BIOS_INT1A:
        handle_bios_int1a (guest);
        break;

    default:
        fprintf (stderr, "HYPERCALL not implemented (0x%x)\n", data);
    }

}


void handle_bios_int10 (struct vm *guest)
{
    struct ebda_registers *regs = (struct ebda_registers *)
        gpa_to_hva (guest, EBDA_ADDR + EBDA_REGS_OFFSET);
    int i;

    switch (HBYTE (regs->ax)) {

    /* 
     * INT 10h, AH=O1h - Video set text-mode cursor shape
     */
    case 0x01:
        if (DEBUG_INT10) fprintf (stderr, "int 10h, ah=01h\n");
        break;

    /* 
     * INT 10h, AH=O2h - Set cursor position
     *
     * Return
     * ------
     * BH = Page Number, DH = Row, DL = Column
     */
    case 0x02:
        if (DEBUG_INT10) fprintf (stderr, "int 10h, ah=02h\n");
        break;

    /* 
     * INT 10h, AH=O3h - Video Read Cursor Position and Size
     * (http://vitaly_filatov.tripod.com/ng/asm/asm_023.4.html) 
     *
     * Return
     * ------
     * AX = 0, CH = Start scan line, CL = End scan line, DH = Row, DL = Column
     */
    case 0x03:
        if (DEBUG_INT10) fprintf (stderr, "int 10h, ah=03h\n");
        regs->ax = 0x0000;
        regs->cx = 0x0007;
        regs->dx = 0x0000;
        break;

    /* 
     * INT 10h, AH=O9h - Write character and attribute at cursor position
     *
     * Return
     * ------
     * AL = Character
     * BH = Page Number, BL = Color
     * CX = Number of times to print character
     */
    case 0x09:
        for (i=0;i<regs->cx;i++)
            console_out (BYTE (regs->ax));
        break;

    /* 
     * INT 10h, AH=OEh - Video teletype output
     * (http://www.ctyme.com/intr/rb-0106.htm)
     */
    case 0x0E:
        console_out (BYTE (regs->ax));
        break;

    default:
        fprintf (stderr, "handle_bios_int10(): ah=%xh not implemented\n",
                 HBYTE (regs->ax));
        dump_ebda_regs (guest);
        exit (1);
    }
}


void handle_bios_int13 (struct vm *guest)
{
    uint32_t buffer_GPA;    /* Guest Physical Address (GPA) */
    struct disk_address_packet *dap;
    struct drive_parameters *params;
    struct ebda_drive_chs_params *ebda_chs_params;
    off_t disk_size;
    ssize_t ret;

    struct ebda_registers *regs = (struct ebda_registers *)
        gpa_to_hva (guest, EBDA_ADDR + EBDA_REGS_OFFSET);

    switch (HBYTE (regs->ax)) {

    /* INT 13h, AH=00h - Reset Disk Drive */
    case 0x00:
        if (DEBUG_INT13) fprintf (stderr, "int 13h, ah=00h\n");
        CLEAR (regs->flags, FLAG_CF);
        regs->ax &= 0x00FF;     /* AH = 0x00 */
        break;

    /* INT 13h, AH=08h - Read drive parameters */
    case 0x08:
        if (DEBUG_INT13) fprintf (stderr, "int 13h, ah=08h\n");

        ebda_chs_params = (struct ebda_drive_chs_params *)
            gpa_to_hva (guest, EBDA_ADDR + EBDA_DISK1_OFFSET);

        /* DH   last index of heads = number_of - 1
         * DL   number of hard disk drives
         * CX [7:6] [15:8]  last index of cylinders = number_of - 1 
         *    [5:0]         last index of sectors per track = number_of */
        regs->dx = (uint16_t)
           ((((uint16_t) ebda_chs_params->head - 1) << 8) + 1);
        regs->cx = (uint16_t)
           (((ebda_chs_params->cyl & 0x00FF) << 8) +
            ((ebda_chs_params->cyl & 0x0300) >> 2) +
            ((uint16_t) ebda_chs_params->sectors_per_track & 0x003F));

        /* Status of last hard disk drive operation = OK */
        CLEAR (regs->flags, FLAG_CF);
        regs->ax = 0x0000;
        *((uint8_t *) gpa_to_hva (guest, BDA_ADDR + 0x74)) = 0x00;

    /*
     * INT 13h, AH=41h - Check extensions present
     * (http://www.ctyme.com/intr/rb-0706.htm) 
     */
    case 0x41:
        if (DEBUG_INT13) fprintf (stderr, "int 13h, ah=41h\n");
        regs->ax = 0x0100;      /* 1.x */
        regs->bx = 0xAA55;

        /* extended disk access functions (AH=42h-44h,47h,48h) supported */
        regs->cx = 0x0001;

        /* Status of last hard disk drive operation = OK */
        CLEAR (regs->flags, FLAG_CF);
        *((uint8_t *) gpa_to_hva (guest, BDA_ADDR + 0x74)) = 0x00;

        break;

    /*
     * INT 13h AH=42h - Extended read
     *    (http://www.ctyme.com/intr/rb-0708.htm)
     * result  
     * ------  
     *    CF clear if successful
     *    AH = 00h if successful or error code
     *    disk address packet's block count field set to number of blocks
     *    successfully transferred
     */
    case 0x42:
        dap = (struct disk_address_packet *)
            gpa_to_hva (guest, rmode_to_gpa (regs->ds, regs->si));

        buffer_GPA =
            ((dap->buffer >> 12) & 0xFFFF0) + (dap->buffer & 0xFFFF);

        //fprintf (stderr, "disk address packet: %x:%x\n", regs->ds, regs->si);
        if (DEBUG_INT13) fprintf (stderr, "int 13h, ah=42h: sector: %ld, count: %d, buffer: %x (0x%x)\n",
                 dap->sector, dap->count, dap->buffer, buffer_GPA);

        ret =
            disk_read (guest, gpa_to_hva (guest, buffer_GPA), dap->sector,
                       dap->count);
        if (ret < 0)
            pexit ("disk_read()");

        /* Status of last hard disk drive operation = OK */
        CLEAR (regs->flags, FLAG_CF);
        regs->ax &= 0x00FF;     /* AH = 0x00 */
        *((uint8_t *) gpa_to_hva (guest, BDA_ADDR + 0x74)) = 0x00;

        break;

    /* 
     * INT 13h AH=48h - Get Drive Parameters
     *
     * Input
     * -----
     * DL   drive number
     * DS:SI    address of result buffer
     *
     * Return
     * ------
     * AH   0
     * DS:SI    result
     */
    case 0x48:
        if (DEBUG_INT13) fprintf (stderr, "int 13h, ah=48h\n");

        ebda_chs_params = (struct ebda_drive_chs_params *)
            gpa_to_hva (guest, EBDA_ADDR + EBDA_DISK1_OFFSET);

        params = (struct drive_parameters *)
            gpa_to_hva (guest, rmode_to_gpa (regs->ds, regs->si));

        params->size_of = 26;
        params->flags   = FLAGS_CHS_IS_VALID;
        params->cyl     = ebda_chs_params->cyl;
        params->head    = ebda_chs_params->head;
        params->sectors_per_track = ebda_chs_params->sectors_per_track;

        disk_size = lseek (guest->disk_fd, 0, SEEK_END);
        if (disk_size < 0)
            pexit ("lseek");
        params->sectors = (uint64_t) disk_size / 512;

        params->sector_size = 512;

        /* Status of last hard disk drive operation = OK */
        CLEAR (regs->flags, FLAG_CF);
        regs->ax &= 0x00FF;     /* AH = 0x00 */
        *((uint8_t *) gpa_to_hva (guest, BDA_ADDR + 0x74)) = 0x00;

        break;

    /* 
     * INT 13h AH=4Bh - Bootable CD-ROM - get status
     */
    case 0x4b:
        if (DEBUG_INT13) fprintf (stderr, "int 13h, ah=4bh\n");
        SET (regs->flags, FLAG_CF); 
        break;

    default:
        fprintf (stderr, "handle_bios_int13(): AH=%Xh not implemented\n",
                 HBYTE (regs->ax));
        dump_ebda_regs (guest);
        dump_real_mode_stack (guest);
        exit (1);
    }
}


void set_e820_entry (struct vm *guest, uint16_t segment, uint16_t offset,
                     uint64_t base, uint64_t limit, uint8_t type)
{
    struct e820_entry *entry;
    entry =
        (struct e820_entry *) gpa_to_hva (guest,
                                          rmode_to_gpa (segment, offset));
    entry->base = base;
    entry->length = limit - base;
    entry->type = type;
}


void handle_bios_int15 (struct vm *guest)
{
    struct ebda_registers *regs = (struct ebda_registers *)
        gpa_to_hva (guest, EBDA_ADDR + EBDA_REGS_OFFSET);

    if (WORD (regs->ax) != 0xE820) {
        fprintf (stderr, "handle_bios_int15(): AH=%Xh not implemented\n",
                 HBYTE (regs->ax));
        dump_ebda_regs (guest);
        exit (1);
    }

    /*
     * INT 15h, AX=E820h - Query System Address Map 
     * ============================================
     * Input 
     * -----
     * EDX = 534D4150h ('SMAP')
     * EBX = continuation value or 00000000h to start at beginning of map
     * ECX = size of buffer for result, in bytes (should be >= 20 bytes)
     * ES:DI -> buffer for result
     * 
     * Return   
     * ------
     * CF clear if successful, set on error
     * EAX = 534D4150h ('SMAP') or error code (86h)
     * ES:DI buffer filled
     * EBX = next offset from which to copy or 00000000h if all done
     * ECX = actual length returned in bytes
     * 
     * Memory map address descriptor
     * ------------------------------ 
     * Offset  Size    Description  
     * 00h    QWORD   base address
     * 08h    QWORD   length in bytes
     * 10h    DWORD   type of address 
     * 
     * Type of address
     * ---------------
     * 01h    memory, available to OS
     * 02h    reserved, not available (e.g. system ROM, memory-mapped device)
     * 03h    ACPI Reclaim Memory (usable by OS after reading ACPI tables)
     * 04h    ACPI NVS Memory (OS is required to save this memory between NVS
     */
    if (DEBUG_INT15) fprintf (stderr, "int 15h, ax=e820h\n");

    switch (regs->bx) {
    case 0:
        set_e820_entry (guest, regs->es, WORD (regs->di), 0, 0xA0000,
                        E820_RAM);
        regs->bx = 1;
        break;
    case 1:
        set_e820_entry (guest, regs->es, WORD (regs->di), 0xA0000,
                        0x100000, E820_RESERVED);
        regs->bx = 2;
        break;
    case 2:
        set_e820_entry (guest, regs->es, WORD (regs->di), 0x100000,
                        guest->ram_size, E820_RAM);
        regs->bx = 3;
        break;
    default:
        SET (regs->flags, FLAG_CF); 
        regs->ax = 0x86;        /* Function not supported */
        return;
    }

    CLEAR (regs->flags, FLAG_CF);
    regs->ax = 0x4150;
    regs->cx = 0x14;
}


void handle_bios_int16 (struct vm *guest)
{
    struct ebda_registers *regs = (struct ebda_registers *)
        gpa_to_hva (guest, EBDA_ADDR + EBDA_REGS_OFFSET);

    switch (HBYTE (regs->ax)) {

    /* 
     * INT 16h, AH=O0h - Read keystroke
     * AH = BIOS scan code
     * AL = ASCII character
     */
    case 0x00:
        regs->ax = 0x1C0A; /* ENTER key press */
        break;

    /* 
     * INT 16h, AH=O1h - Check for keystroke
     * ZF clear if keystroke available
     * AH = BIOS scan code
     * AL = ASCII character
     */
    case 0x01:
        if (DEBUG_INT16) fprintf (stderr, "int 16h, ah=01h\n");
        CLEAR (regs->flags, FLAG_ZF); 
        regs->ax = 0x1C0A; /* ENTER key press */
        break;

    default:
        fprintf (stderr, "handle_bios_int16(): ah=%xh not implemented\n",
                 HBYTE (regs->ax));
        dump_ebda_regs (guest);
        exit (1);
    }
}


void handle_bios_int1a (struct vm *guest)
{
    struct tms dummy;
    clock_t elapsed;
    struct ebda_registers *regs = (struct ebda_registers *)
        gpa_to_hva (guest, EBDA_ADDR + EBDA_REGS_OFFSET);

    switch (HBYTE (regs->ax)) {

    /*
     * INT 1Ah AH=00h - Read current time
     * result:
     *    CX High word of tick count
     *    DX Low word of tick count
     *    AL 00h = Day rollover has not occurred 
     */
    case 0x00:
        if (DEBUG_INT1A) fprintf (stderr, "int 1ah, ax=00h\n");
        elapsed = times (&dummy) - guest->clock_start;
        if (elapsed > 0) {
            regs->cx = (uint16_t) (elapsed & 0xFFFF);
            regs->dx = (uint16_t) ((elapsed & 0xFFFF0000) >> 16);
        } else {
            regs->cx = 0x0000;
            regs->dx = 0x0000;
        }
        regs->ax = 0x0000;
        break;

    default:
        fprintf (stderr, "handle_bios_int1a(): ah=%xh not implemented\n",
                 HBYTE (regs->ax));
        dump_ebda_regs (guest);
        exit (1);
    }
}


void handle_exit_io_serial (struct vm *guest)
{
    // fprintf (stderr, "handle_exit_io_serial()\n");

    if (guest->kvm_run->io.direction == KVM_EXIT_IO_OUT) {

        /* Linux/Documentation/virtual/kvm/api.txt - 
         * "data_offset describes where the data is located
         * (KVM_EXIT_IO_OUT) or where kvm expects application code to
         * place the data for the next KVM_RUN invocation
         * (KVM_EXIT_IO_IN)" */

        write (STDERR_FILENO,
               (char *) (guest->kvm_run) + guest->kvm_run->io.data_offset,
               guest->kvm_run->io.size);

    } else {
        fprintf (stderr, "unhandled KVM_EXIT_IO (port: 0x%x)\n",
                 guest->kvm_run->io.port);
        dump_regs (guest);
        exit (1);
    }
}


void handle_exit_hlt (void)
{
    fprintf (stderr, "Guest halted\n");
    exit (0);
}
