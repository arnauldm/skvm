[BITS 16]
[ORG 0x7C00]

;-------------------------------------------
; Guest code write to the COM0 serial port
;-------------------------------------------

code16:
  mov dx, 0x3f8
  mov si, msg
.display:
  lodsb         ; al <- ds:si, si <- si + 1
  cmp al, 0     ; end of string ?
  jz .display_end
  out dx, al
  jmp .display
.display_end:

;-------------------------------------------
; Guest code call the BIOS INT 10h routine
;-------------------------------------------

  mov si, msg2
.testing_int10:
  lodsb         ; ds:si -> al
  cmp al, 0     ; fin chaine ?
  jz .end
  mov ah, 0x0E  ; service 0Eh
  mov bx, 0x07  ; bx -> attribut 
  int 0x10
  jmp .testing_int10

.end:
  hlt

;--- Variables ---
msg db "Hello, world!", 10, 0
msg2 db "Goodbye.", 10, 0

code16_end:
