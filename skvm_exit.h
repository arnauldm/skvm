#include "vm.h"

#define SERIAL_PORT 0x3F8
#define HYPERCALL_PORT  0XCAFE
#define HC_BIOS_INT10 0x10
#define HC_BIOS_INT13 0x13
#define HC_BIOS_INT15 0x15
#define HC_BIOS_INT16 0x16
#define HC_BIOS_INT1A 0x1A
#define HC_PANIC 0xFF

#define FLAG_CF 0x0001
#define FLAG_ZF 0x0040
#define CLEAR(reg,flag) reg &= (uint16_t) (~flag)
#define SET(reg,flag) reg |= (uint16_t) flag


#ifdef __EXIT_IO__
void handle_exit_io (struct vm *);
void handle_exit_io_hypercall (struct vm *);
void handle_exit_io_serial (struct vm *);
void handle_bios_int10 (struct vm *);
void handle_bios_int13 (struct vm *);
void handle_bios_int15 (struct vm *);
void handle_bios_int16 (struct vm *);
void handle_bios_int1a (struct vm *);
#else
extern void handle_exit_io (struct vm *);
extern void handle_exit_hlt (void);
#endif

/*
 * INT 13h AH=42h - Extended read
 * (http://www.ctyme.com/intr/rb-0708.htm)
 */
struct disk_address_packet {
    uint8_t size_of;            /* Size of this struct */
    uint8_t reserved;           /* Should be at 0 */
    uint16_t count;             /* Number of sectors to be read */
    uint32_t buffer;            /* Segment:offset pointer to the buffer */
    uint64_t sector;            /* Starting sector number (1st sector has number 0) */
} __attribute__ ((packed));

/*
 * INT 13h AH=48h - Get Drive Parameters
 */
struct drive_parameters {
    uint16_t size_of;           /* = 26 */
    uint16_t flags;
    uint32_t cyl;               /* Number of cylinders */
    uint32_t head;              /* Number of heads */
    uint32_t sectors_per_track; /* Number of sectors per track */
    uint64_t sectors;           /* Absolute number of sectors */
    uint16_t sector_size;       /* Bytes per sector */
} __attribute__ ((packed));

/* flags bits */
#define FLAGS_DMA_BOUNDARY_ERROR	(1 << 0)
#define FLAGS_CHS_IS_VALID		(1 << 1)
#define FLAGS_REMOVABLE			(1 << 2)
#define FLAGS_WRITE_WITH_VERIFY         (1 << 3)
#define FLAGS_CHANGE_LINE_SUPPORT       (1 << 4)
#define FLAGS_LOCKABLE                  (1 << 5)
#define FLAGS_CHS_SET_TO_MAX            (1 << 6)

/* 
 * INT 15h, AX=E820h - Query System Address Map 
 */
struct e820_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} __attribute__ ((packed));

#define E820_RAM      0x01
#define E820_RESERVED 0x02
#define E820_ACPI     0x03
#define E820_ACPI_NVS 0x04
