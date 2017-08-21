[BITS 16]
[ORG 0xF0000]

; Important note: BIOS is loaded in RAM at F0000h

;-----------------------------------------------------------------------------
[SECTION int10_handler, start=0xff065]

int10_handler:
  push dx
  mov dx, 0x3f8
  out dx, al
  pop dx
  iret

;-----------------------------------------------------------------------------
[SECTION int13_handler, start=0xfe3fe]

int13_handler:
  call error

;-----------------------------------------------------------------------------
error:
  push si
  mov si, msg
  call display 
  pop si
  hlt

;-----------------------------------------------------------------------------
display:
  push ax
  push dx
  push ds

  mov ax, 0xf000
  mov ds, ax
  mov dx, 0x3f8
.start:
  lodsb
  cmp al, 0     ; end of string (NULL character) ?
  jz .end
  out dx, al
  jmp .start

.end:
  pop ds
  pop dx
  pop ax
  ret

msg db "minibios: int 13h is not implemented", 10, 0

