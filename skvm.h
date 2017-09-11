
#define KBYTE (1<<10)
#define MBYTE (1<<20)
#define GBYTE (1<<30)

#define RAM_SIZE (128*MBYTE)    /* Memory in bytes */
#define KVM_FILE "/dev/kvm"     /* KVM special file */

#define BYTE(x) ((uint8_t) x & 0xFF)
#define HBYTE(x) ((x >> 8) & 0xFF)
#define WORD(x) (x & 0xFFFF)

/********************
 * BIOS environment *
 ********************/

#define IVT_ADDR        0x00000000      /* Bios Interrupt Vectors Table */
#define BDA_ADDR	0x00000400      /* Bios Data Area */
#define LOAD_ADDR       0x00007C00      /* Guest MBR loading address */
#define EBDA_ADDR       0x0009FC00      /* Extended Bios Data Area */
#define BIOS_ADDR       0x000F0000
#define PCI_HOLE_ADDR   0xC0000000      /* PCI hole physical address */

/* Real mode interrupt vector table */

struct ivt_entry {
    uint16_t offset;
    uint16_t cs;
} __attribute__ ((packed));


/* EBDA data offsets */

#define EBDA_SIZE_OFFSET	0x00
#define EBDA_DISK1_OFFSET	0x3D
#define EBDA_DISK_NUM_OFFSET	0x70

/* Bios can save some registers here before a call to the VMM */
#define EBDA_REGS_OFFSET	0x200

/* EBDA - Hard disk drive parameter table */
struct ebda_drive_chs_params {
    uint16_t cyl;               /* Number of cylinders */
    uint8_t head;               /* Number of heads */
    uint8_t unused[11];
    uint8_t sectors_per_track;  /* Number of sectors per track */
} __attribute__ ((packed));

/* EBDA - Registers transmitted by the BIOS */
struct ebda_registers {
    uint16_t cs;
    uint16_t es;
    uint16_t ds;
    uint16_t ss;
    uint16_t ip;
    uint16_t flags;
    uint16_t di;
    uint16_t si;
    uint16_t bp;
    uint16_t sp;
    uint16_t bx;
    uint16_t dx;
    uint16_t cx;
    uint16_t ax;
} __attribute__ ((packed));
