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

static char msg[] = "minibios: INT ??h is not implemented\n";
static char msg10[] = "minibios: INT 10h is not fully implemented\n";
static char msg13[] = "minibios: INT 13h is not implemented\n";


//---------------------------------------------------------------------------
#asm
.org 0x0000
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

#asm

//---------------------------------------------------------------------------
.org 0x2222 
default_handler:
    mov si, #_msg
    call display 
    hlt

//---------------------------------------------------------------------------
.org 0xe3fe 
int13_handler:
    push si 
    mov si, #_msg13
    call display 
    pop si 
    mov dx, #0xffff
    out dx, al 
    hlt

//---------------------------------------------------------------------------
.org 0xf065 // INT 10 h Video Support Service Entry Point 
int10_handler:
    push dx 
    cmp ah, #0x0e
    jnz .error 
    mov dx, #0x3f8
    out dx, al 
    pop dx 
    iret
.error:
    mov si, #_msg10
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

