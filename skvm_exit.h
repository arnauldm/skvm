#include "vm.h"

#define SERIAL_PORT 0x3F8
#define HYPERCALL_PORT  0XCAFE
#define HC_BIOS_INT10 0x10
#define HC_BIOS_INT13 0x13
#define HC_BIOS_INT15 0x15
#define HC_PANIC 0xFF

#ifdef __EXIT_IO__
void handle_exit_io (struct vm *);
void handle_exit_io_hypercall (struct vm *);
void handle_exit_io_serial (struct vm *);
void handle_bios_int10 (struct vm *);
void handle_bios_int13 (struct vm *);
void handle_bios_int15 (struct vm *);
#else
extern void handle_exit_io (struct vm *);
extern void handle_exit_hlt (void);
#endif

/* int 13h, ah=42h - Extended read */
struct disk_address_packet {
    uint8_t size_of;            // Size of this struct 
    uint8_t reserved;           // Should be at 0
    uint16_t count;             // Number of sectors to be read
    uint32_t buffer;            // Segment:offset pointer to the buffer
    uint64_t sector;            // Starting sector number (1st sector has number 0)
} __attribute__ ((packed));

/* int 15h, ax=e820h - Query System Address Map */
struct e820_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} __attribute__ ((packed));

#define E820_RAM      0x01
#define E820_RESERVED 0x02
#define E820_ACPI     0x03
#define E820_ACPI_NVS 0x04


