bits 32

section .text
global inb
global outb

; uint8_t inb(uint16_t port);
inb:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]  ; port number
    in al, dx           ; read byte from port
    leave
    ret

; void outb(uint16_t port, uint8_t value);
outb:
    push ebp
    mov ebp, esp
    mov edx, [ebp + 8]  ; port number
    mov eax, [ebp + 12] ; value
    out dx, al          ; write byte to port
    leave
    ret