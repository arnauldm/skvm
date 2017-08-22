//----------------------------------------------------------------------------
// Important note:
//---------------------------------------------------------------------------
// This code is BCC (Bruce's C Compiler) code as GCC can't produce 16 bit 8086
// assembly. That compiler is not well documented, so the best is to look
// for some examples, such as Bochs's rombios.c file.
//---------------------------------------------------------------------------

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define GET_AL() ( AX & 0x00ff )
#define GET_AH() *(((uint8_t *)&AX)+1)

//---------------------------------------------------------------------------
#asm
.org 0x0
// INT 13h treatment takes place in `skvm.c' VMM. The call is made via a
// "special" OUT instruction, thus clobbering dx register. That variable is
// mapped at a fixed offset: it's a convenient way for the hypervisor to
// retrieve it
saved_dx:
    dw 0

saved_ip:
    dw 0

saved_cs:
    dw 0

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


void outb (port, val)
    uint16_t port;
    uint8_t val;
{
#asm
    push ax 
    push dx 
    mov dx, 4[bp]
    mov al, 6[bp]
    out dx, al 
    pop dx 
    pop ax
#endasm
}


//---------------------------------------------------------------------------
#asm

.org 0x1000 
debug_handler:
    push bp
    mov bp, sp

    push ax
    push ds 

    mov ax, #0xf000
    mov ds, ax 

    mov ax, 2[bp]
    mov saved_ip, ax
    mov ax, 4[bp]
    mov saved_cs, ax

    pop ds
    pop ax
    pop bp

    push si
    mov si, #opcode
    call display 
    pop si
    mov dx, #0xff00 // Special port created on purpose
    out dx, al 
    hlt

//---------------------------------------------------------------------------
.org 0x2222 
default_handler:
    push si
    mov si, #msg
    call display 
    pop si
    hlt

//---------------------------------------------------------------------------
.org 0xe000 
int13_handler:

    push bp
    mov bp, sp

    push dx

    push ax
    push ds 

    mov ax, #0xf000
    mov ds, ax 

    mov ax, 2[bp]
    mov saved_ip, ax
    mov ax, 4[bp]
    mov saved_cs, ax
    mov saved_dx, dx // `saved_dx' variable is mapped at a fixed offset, thus
                     // skvm can easily retrieve it
    pop ds
    pop ax

    mov dx, #0xff13 // Special port created on purpose
    out dx, al 

    pop dx
    pop bp

    iret

//---------------------------------------------------------------------------
.org 0xf000 // INT 10 h Video Support Service Entry Point 
int10_handler:
    push dx 
    cmp ah, #0x0e
    jnz .error 
    mov dx, #0x3f8
    out dx, al 
    pop dx 
    iret
.error:
    mov si, #msg10
    call display
    hlt 

display:
    push ax
    push dx 
    push ds 
    mov ax, #0xf000
    mov ds, ax 
    mov dx, #0x3f8
.start:
    lodsb 
    cmp al, #0    // end of string (NULL character) ?
    jz .end 
    out dx, al 
    jmp .start
.end:
    pop ds 
    pop dx 
    pop ax 
    ret
#endasm

