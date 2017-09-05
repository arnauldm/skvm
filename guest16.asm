[BITS 16]
[ORG 0x7C00]


;-------------------------------------------
; Guest code call the BIOS INT 10h routine
;-------------------------------------------

_start:
  xor ax, ax
  mov ss, ax
  mov sp, 0x2000

  mov si, hello_msg
  call display

.halt:
  mov si, last_msg
  call display
  hlt
  jmp .halt


display:
  push ax
  push bx
.display_start:
  lodsb         ; ds:si -> al
  cmp al, 0     ; fin chaine ?
  jz .display_end
  mov ah, 0x0E  ; service 0Eh
  mov bx, 0x07  ; bx -> attribut 
  int 0x10
  jmp .display_start
.display_end:
  pop bx
  pop ax
  ret

;--- Variables ---
hello_msg db "guest: Hello, world!", 10, 0
last_msg db "guest: Goodbye", 10, 0
test_msg db "guest: Test", 10, 0

code16_end:
