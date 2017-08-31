
extern void dump_sregs (struct vm *);
extern void dump_regs (struct vm *);
extern void dump_ebda_regs (struct vm *);
extern void dump_real_mode_stack (struct vm *);

#ifdef __DEBUG__
#define DEBUG(fmt, ...) \
            do { fprintf(stderr, fmt, __VA_ARGS__); } while (0)
#else
#define DEBUG(fmt, ...)
#endif
