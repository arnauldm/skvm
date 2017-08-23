//----------------------------------------------------------------------------
// Important note:
//---------------------------------------------------------------------------
// This code is BCC (Bruce's C Compiler) code as GCC can't produce 16 bit 8086
// assembly. That compiler is not well documented, so the best is to look
// for some examples, such as Bochs's rombios.c file.
//---------------------------------------------------------------------------

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define PORT_SERIAL 0x3f8

#define GET_AL() ( AX & 0x00ff )
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
        outb (PORT_SERIAL, *s);
        s++;
    }
}

void int13_c_handler (DS, ES, DI, SI, BP, temp_SP, BX, DX, CX, AX, FLAGS)
  uint16_t DS, ES, DI, SI, BP, temp_SP, BX, DX, CX, AX, FLAGS;
{
    serial_print ("INT 13h\n");

    switch (GET_AH()) {
        case 0x08:
            serial_print ("int13_c_handler - service 0x08\n");
            break;
        case 0x41:
            serial_print ("int13_c_handler - service 0x41\n");
            FLAGS |= 0x0001; // Set CF
            break;
        default:
            break;   
    }
#asm
    hlt
#endasm
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

    pushf 
    pusha // AX, CX, DX, BX, temp, BP, SI, DI
    push es
    push ds

    call _int13_c_handler

    pop ds
    pop es
    popa
    popf

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
    mov dx, #PORT_SERIAL
    out dx, al 
    pop dx 

    pop ds
    iret
.error:
    push #msg10
    call _serial_print
    hlt 
#endasm

