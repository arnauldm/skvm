[BITS 16]
[ORG 0x1000]

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

  hlt

;--- Variables ---
msg db "Hello, world!", 10, 0

code16_end:
