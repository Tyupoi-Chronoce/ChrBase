

char msg[] = "test";

__asm__
(
    mov dx, [msg]
    mov ah, 0x09
    int 21h

    mov ah, 4ch
    int 21h
);