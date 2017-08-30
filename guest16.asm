[BITS 16]
[ORG 0x7C00]


;-------------------------------------------
; Guest code call the BIOS INT 10h routine
;-------------------------------------------

_start:
  xor ax, ax
  mov ss, ax
  mov sp, 0x2000

hello:
  mov si, msg
.start:
  lodsb         ; ds:si -> al
  cmp al, 0     ; fin chaine ?
  jz .end
  mov ah, 0x0E  ; service 0Eh
  mov bx, 0x07  ; bx -> attribut 
  int 0x10
  jmp .start
.end:

  mov ax, 0xAAAA
  mov bx, 0xBBBB
  mov cx, 0xCCCC
  mov dx, 0xDDDD
  mov bp, 0x1234
  mov si, 0x5678
  mov di, 0x9ABC
  int 0x15

  hlt

;--- Variables ---
msg db "Hello, world!", 10, 0

code16_end:
