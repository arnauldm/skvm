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
#define EBDA_REGS_CS    0
#define EBDA_REGS_ES    2
#define EBDA_REGS_DS    4
#define EBDA_REGS_SS    6
#define EBDA_REGS_IP    8
#define EBDA_REGS_FLAGS 10
#define EBDA_REGS_DI    12
#define EBDA_REGS_SI    14
#define EBDA_REGS_BP    16
#define EBDA_REGS_SP    18
#define EBDA_REGS_BX    20
#define EBDA_REGS_DX    22
#define EBDA_REGS_CX    24
#define EBDA_REGS_AX    26

#define SERIAL_PORT 0x3F8
#define HYPERCALL_PORT  0XCAFE
#define HC_BIOS_INT10 0x10
#define HC_BIOS_INT13 0x13
#define HC_BIOS_INT15 0x15
#define HC_PANIC 0xFF

#define SET_AL(val8) *((uint8_t *)&AX) = (val8)
#define SET_BL(val8) *((uint8_t *)&BX) = (val8)
#define SET_CL(val8) *((uint8_t *)&CX) = (val8)
#define SET_DL(val8) *((uint8_t *)&DX) = (val8)
#define SET_AH(val8) *(((uint8_t *)&AX)+1) = (val8)
#define SET_BH(val8) *(((uint8_t *)&BX)+1) = (val8)
#define SET_CH(val8) *(((uint8_t *)&CX)+1) = (val8)
#define SET_DH(val8) *(((uint8_t *)&DX)+1) = (val8)

#define GET_AL() ( AX & 0x00ff )
#define GET_BL() ( BX & 0x00ff )
#define GET_CL() ( CX & 0x00ff )
#define GET_DL() ( DX & 0x00ff )
#define GET_AH() *(((uint8_t *)&AX)+1)
#define GET_BH() *(((uint8_t *)&BX)+1)
#define GET_CH() *(((uint8_t *)&CX)+1)
#define GET_DH() *(((uint8_t *)&DX)+1)

struct hard_disk_parameter {
    uint16_t cyl;    /* Number of cylinders */
    uint8_t head;    /* Number of heads */
    uint8_t unused[11];
    uint8_t sectors; /* Number of sectors per track */
};


//---------------------------------------------------------------------------
#asm
.org 0x0
db 0
#endasm




#asm
.org 0x100
#endasm

// The HYPERCALL macro copy current registers in the Extended Bios Data
// Area (EBDA), mapped in 0x9FC00 in physical memory, to transmit them to the
// skvm VMM. 

#asm
get_ip:
    push bp 
    mov bp, sp
    mov ax, 2[bp]
    pop bp
    ret

MACRO HYPERCALL
    pusha       // AX, CX, DX, BX, orig SP, BP, SI, DI
    pushf 
    call get_ip // IP -> AX
    push ax     // push IP on the stack
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

    mov dx,#HYPERCALL_PORT  // "jump" out to the hypervisor
    mov ax,#?1          // requested service
    out dx,ax

    pop ax // dummy value
    pop es
    pop ds
    pop ss
    pop ax // dummy value
    popf
    popa
MEND
#endasm


void writeb (segment, offset, value)
    uint16_t segment, offset;
    uint8_t value;
{
#asm
    push bp
    mov  bp, sp

    push ax
    push bx
    push ds

    mov ax, 4[bp] // segment
    mov ds, ax
    mov bx, 6[bp] // offset
    mov al, 8[bp] // value
    mov [bx], al
    
    pop ds
    pop bx
    pop ax

    pop bp
#endasm
}


void writew (segment, offset, value)
    uint16_t segment, offset, value;
{
#asm
    push bp
    mov  bp, sp

    push ax
    push bx
    push ds

    mov ax, 4[bp] // segment
    mov ds, ax
    mov bx, 6[bp] // offset
    mov ax, 8[bp] // value
    mov [bx], ax
    
    pop ds
    pop bx
    pop ax

    pop bp
#endasm
}


uint16_t readw (segment, offset)
    uint16_t segment, offset;
{
#asm
    push bp
    mov  bp, sp

    push bx
    push ds

    mov ax, 4[bp] // segment
    mov ds, ax
    mov bx, 6[bp] // offset
    mov ax, [bx]

    pop ds
    pop bx

    pop bp
#endasm
}


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
#asm
    push bp
    mov  bp, sp

    push ax
    push dx
    push si

    mov dx, #SERIAL_PORT
    mov si, 4[bp]   // pointer to the string
.out_start:
    lodsb   // ds:si -> al
    cmp al, #0
    jz .out_end
    out dx, al
    jmp .out_start
.out_end:

    pop si
    pop dx
    pop ax

    pop bp
#endasm
}


void int10_c_handler (ES, DS, FLAGS, DI, SI, BP, orig_SP, BX, DX, CX, AX)
  uint16_t ES, DS, FLAGS, DI, SI, BP, orig_SP, BX, DX, CX, AX;
{
    switch (GET_AH()) {
        // INT 10h, AH=OEh - Video teletype output
        // (http://www.ctyme.com/intr/rb-0106.htm)
        case 0x0E:
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_AX, AX);
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_BX, BX);
            outb (HYPERCALL_PORT, HC_BIOS_INT10);
            break;
        default:
            serial_print ("minibios: INT 10h - AH value not supported\n");
            #asm
            HYPERCALL (HC_PANIC)
            #endasm
    }
}


void int13_c_handler (ES, DS, FLAGS, DI, SI, BP, orig_SP, BX, DX, CX, AX)
  uint16_t ES, DS, FLAGS, DI, SI, BP, orig_SP, BX, DX, CX, AX;
{
    switch (GET_AH()) {
        case 0x00:
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_AX, AX);
            outb (HYPERCALL_PORT, HC_BIOS_INT13);
            AX = readw (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_AX);
            FLAGS = readw (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_FLAGS);
            break;

        case 0x08:
            serial_print ("minibios: INT 13h - AH=08h not implemented\n");
            #asm
                hlt
            #endasm
            break;
        
        // INT 13h AH=41h: Check extensions present
        //    (http://www.ctyme.com/intr/rb-0706.htm)
        case 0x41:
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_AX, AX);
            outb (HYPERCALL_PORT, HC_BIOS_INT13);

            AX = readw (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_AX);
            BX = readw (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_BX);
            CX = readw (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_CX);
            FLAGS = readw (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_FLAGS);

            break;

        // INT 13h AH=42h: Extended read
        //    (http://www.ctyme.com/intr/rb-0708.htm)
        // result: 
        //    CF clear if successful
        //    AH = 00h if successful or error code
        //    disk address packet's block count field set to number of blocks
        //    successfully transferred
        case 0x42:
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_AX, AX);
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_DX, DX);
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_DS, DS);
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_SI, SI);
            outb (HYPERCALL_PORT, HC_BIOS_INT13);

            AX = readw (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_AX);
            FLAGS = readw (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_FLAGS);
            
            break; 
        default:
            serial_print ("minibios: INT 13h - AH value not supported\n");
            #asm
            HYPERCALL (HC_PANIC)
            #endasm
    }
}


void int15_c_handler (ES, DS, FLAGS, DI, SI, BP, orig_SP, BX, DX, CX, AX)
  uint16_t ES, DS, FLAGS, DI, SI, BP, orig_SP, BX, DX, CX, AX;
{
    switch (AX) {
        // INT 15h, AX=E820h - Query System Address Map
        case 0xE820:
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_AX, AX);
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_BX, BX);
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_CX, CX);
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_DX, DX);
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_ES, ES);
            writew (EBDA_SEG, EBDA_REGS_OFFSET + EBDA_REGS_SI, SI);
            outb (HYPERCALL_PORT, HC_BIOS_INT15);
            #asm
            hlt
            #endasm
            break;

        default:
            serial_print ("minibios: INT 15h - AH value not supported\n");
            #asm
            HYPERCALL (HC_PANIC)
            #endasm
    }
}

#asm

//---------------------------------------------------------------------------
.org 0x1000 
debug_handler:
    push ds 

    push #0xf000
    pop ds

    push #no_handler
    call _serial_print 

    HYPERCALL (HC_PANIC)

    pop ds // dummy
    pop ds
    hlt

no_handler:
    .ascii "minibios: unknown interrupt\n"
    db 0

//---------------------------------------------------------------------------
.org 0x1800 
test_handler:
    push ds 

    push #0xf000
    pop ds

    push ax
    push #test_msg
    call _serial_print 
    pop ax
    pop ax
    HYPERCALL (HC_PANIC)

    pop ds // dummy
    pop ds
    hlt

test_msg:
    .ascii "minibios: I was here !\n"
    db 0

//---------------------------------------------------------------------------
.org 0x2000 
int06_handler:
    push ds 

    push #0xf000
    pop ds

    push #exc_opcode
    call _serial_print 

    pop ds // dummy
    pop ds
    hlt

exc_opcode:
    .ascii "minibios: INVALID OPCODE\n"
    db 0


//---------------------------------------------------------------------------
.org 0x3000 
int15_handler:
    push ds 

    // Set function parameters
    pusha // AX, CX, DX, BX, orig_SP, BP, SI, DI
    pushf 
    push ds
    push es

    // We set DS here not to overlap the 'DS' parameter passed to the
    // _int15_c_handler
    push #0xf000
    pop ds

    call _int15_c_handler

    pop es
    pop ds
    popf
    popa

    pop ds
    iret

//---------------------------------------------------------------------------
.org 0xe000 
int13_handler:
    push ds

    // Set function parameters
    pusha // AX, CX, DX, BX, orig_SP, BP, SI, DI
    pushf 
    push ds
    push es

    // We set DS here not to overlap the 'DS' parameter passed to the
    // _int13_c_handler
    push #0xf000
    pop ds

    call _int13_c_handler

    pop es
    pop ds
    popf
    popa

    pop ds
    iret

//---------------------------------------------------------------------------
.org 0xf000 // INT 10 h Video Support Service Entry Point 
int10_handler:
    // Set DS
    push ds

    // Set function parameters
    pusha // AX, CX, DX, BX, orig_SP, BP, SI, DI
    pushf 
    push ds
    push es

    // We set DS here not to overlap the 'DS' parameter passed to the
    // _int15_c_handler
    push #0xf000
    pop ds

    call _int10_c_handler

    pop es
    pop ds
    popf
    popa

    pop ds
    iret

#endasm

