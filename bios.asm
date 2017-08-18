[BITS 16]
[ORG 0x0]

; Important note: BIOS is loaded in F0000h

times 0xf065 nop

int10_handler:
  mov dx, 0x3f8
  out dx, al
  iret

