#define SERIAL_PORT 0x3F8
#define HYPERCALL_PORT  0XCAFE
#define HC_PANIC 0x01
#define HC_BIOS 0x02

#ifdef __EXIT_IO__
  void handle_exit_io (int, struct kvm_run*);
  void handle_exit_io_hypercall (int, struct kvm_run*);
  void handle_exit_io_serial (int, struct kvm_run*);
  void handle_bios_interrupt (int);
#else
  extern void handle_exit_io (int, struct kvm_run*);
  extern void handle_exit_hlt (void);
#endif

struct disk_address_packet {
    uint8_t size_of; // Size of this struct 
    uint8_t reserved; // Should be at 0
    uint16_t count; // Number of sectors to be read
    uint32_t buffer; // Segment:offset pointer to the buffer
    uint64_t sector; // Starting sector number (1st sector has number 0)
} __attribute__ ((packed));

