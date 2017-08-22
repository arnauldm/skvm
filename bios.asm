[BITS 16]
[ORG 0xF0000]

; Important note: BIOS is loaded in RAM at F0000h

;-----------------------------------------------------------------------------
[SECTION error_handler, start=0xf0000]
  mov si, msg
  call error
  hlt


;-----------------------------------------------------------------------------
[SECTION int10_handler, start=0xff065]

int10_handler:
  push dx
  cmp ah, 0x0e
  jnz .error
  mov dx, 0x3f8
  out dx, al
  pop dx
  iret

.error:
  mov si, msg10
  call error
  hlt

;-----------------------------------------------------------------------------
[SECTION int13_handler, start=0xfe3fe]

int13_handler:
  mov si, msg13
  call error
  hlt

;-----------------------------------------------------------------------------
error:
  call display 
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

msg   db "minibios: INT ??h is not implemented", 10, 0
msg10 db "minibios: INT 10h is not fully implemented", 10, 0
msg13 db "minibios: INT 13h is not implemented", 10, 0

