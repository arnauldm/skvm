//----------------------------------------------------------------------------
// Important note:
//---------------------------------------------------------------------------
// This code is BCC (Bruce's C Compiler) code as GCC can't produce 16 bit 8086
// assembly. That compiler is not well documented, so the best is to look
// for some examples, such as Bochs's rombios.c file.
//---------------------------------------------------------------------------

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define EBDA_SEG    0x9FC0
#define EBDA_DISK1_OFFSET   0x3D
#define EBDA_REGS_OFFSET    0x200

#define SERIAL_PORT 0x3F8
#define PANIC_PORT  0XDEAD

#define GET_AL() ( AX & 0x00FF )
#define GET_AH() *(((uint8_t *)&AX)+1)

//---------------------------------------------------------------------------
#asm
.org 0x0
msg:
    .ascii "minibios: INT ??h is not implemented\n"
    db 0

opcode:
    .ascii "minibios: INVALID OPCODE\n"
    db 0

msg10:
    .ascii "minibios: INT 10h is not fully implemented\n"
    db 0
#endasm


#asm
.org 0x100
#endasm

// the HALT macro is called with the line number of the HALT call.
// The line number is then sent to the PANIC_PORT, causing Bochs/Plex
// to print a BX_PANIC message.  This will normally halt the simulation
// with a message such as "BIOS panic at bios.c, line 4091".
#asm
MACRO PANIC
    pusha       // AX, CX, DX, BX, orig SP, BP, SI, DI
    pushf 
    call next   // push IP register on the stack
next:
    push ss
    push ds
    push es
    mov ax, cs
    push ax

    mov ax, ss          // set ds:si = ss:sp
    mov ds, ax
    mov si, sp
    mov ax, #EBDA_SEG   // set es:di = #EBDA_SEG:#EBDA_REGS_OFFSET
    mov es, ax
    mov di, #EBDA_REGS_OFFSET
    mov cx, #14
    rep 
        movsw            // copy ds:si -> es:di

    mov dx,#PANIC_PORT  // "jump" out to the hypervisor
    mov ax,#?1
    out dx,ax

    pop ax 
    pop es
    pop ds
    pop ss
    ret
    popf
    popa
MEND
#endasm


void outb (port, val)
    uint16_t port;
    uint8_t val;
{
#asm
    push bp
    mov  bp, sp

    push ax
    push dx
    mov  dx, 4[bp]
    mov  al, 6[bp]
    out  dx, al
    pop  dx
    pop  ax

    pop  bp
#endasm
}

void serial_print (s)
    uint8_t *s;
{
    while (*s) {
        outb (SERIAL_PORT, *s);
        s++;
    }
}


void int13_c_handler (DS, ES, FLAGS, DI, SI, BP, orig_SP, BX, DX, CX, AX)
  uint16_t DS, ES, FLAGS, DI, SI, BP, orig_SP, BX, DX, CX, AX;
{
    switch (GET_AH()) {
        case 0x41:
            serial_print ("INT 13h - 41h: not implemented\n");
#asm
            hlt;
#endasm
        default:
#asm
            PANIC (__LINE__)
#endasm
    }
}


//---------------------------------------------------------------------------
#asm

.org 0x1000 
debug_handler:
    push ds 
    push ax
    mov ax, #0xf000
    mov ds, ax 
    pop ax

    push #opcode
    call _serial_print 

    push dx
    mov dx, #0xff00 // Special port created on purpose
    out dx, al 
    pop dx

    pop ds
    hlt

//---------------------------------------------------------------------------
.org 0xe000 
int13_handler:
    push ds
    push ax
    mov ax, #0xf000 
    mov ds, ax 
    pop ax

    pusha // AX, CX, DX, BX, orig_SP, BP, SI, DI
    pushf 
    push es
    push ds

    call _int13_c_handler

    pop ds
    pop es
    popf
    popa

    pop ds
    iret

//---------------------------------------------------------------------------
.org 0xf000 // INT 10 h Video Support Service Entry Point 
int10_handler:
    // set DS
    push ds
    push ax
    mov ax, #0xf000 
    mov ds, ax 
    pop ax

    cmp ah, #0x0e
    jnz .error 

    push dx 
    mov dx, #SERIAL_PORT
    out dx, al 
    pop dx 

    pop ds
    iret
.error:
    push #msg10
    call _serial_print
    hlt 
#endasm

