
#ifdef __EXIT_IO__
  void handle_exit_io (int, struct kvm_run *);
  void handle_exit_io_serial (int, struct kvm_run*);
  void handle_exit_io_bios_int13 (int, struct kvm_run*);
#else
  extern void handle_exit_io (int, struct kvm_run *);
  extern void handle_exit_hlt (int);
#endif
