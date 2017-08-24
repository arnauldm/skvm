[BITS 16]
[ORG 0x7C00]


;-------------------------------------------
; Guest code call the BIOS INT 10h routine
;-------------------------------------------

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
  hlt

;--- Variables ---
msg db "Hello, world!", 10, 0

code16_end:
