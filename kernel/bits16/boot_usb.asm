[ORG 0x00]
[BITS 16]

SECTION .text
jmp 0x07C0:START

TOTALSECTORCOUNT: dw 0x02
BOOTSTRAPPROCESSOR: db 0x01
STARTGRAPHICMODE: db 0x01

START:
    mov ax, 0x07C0
    mov ds, ax
    mov ax, 0xB800
    mov es, ax

    mov ax, 0x0000
    mov ss, ax
    mov sp, 0xFFFE
    mov bp, 0xFFFE


    mov si, 0

CLEARSCREENLOOP:
    mov byte[es:si], 0
    mov byte[es:si+1], 0x0A

    add si, 2
    cmp si, 80 * 25 * 2
    jl CLEARSCREENLOOP

RESETDISK:
    mov ax, 0
    mov dl, byte[BOOTDRIVE]
    int 0x13
    jc HANDLEDISKERROR

    mov ah, 0x08
    mov dl, byte[BOOTDRIVE]
    int 0x13
    jc HANDLEDISKERROR  

    mov byte[LASTHEAD], dh
    mov al, cl
    mov al, 0x3f

    mov byte[LASTSECTOR], al
    mov byte[LASTTRACK], ch

    mov si, 0x1000
    mov es, si


    mov bx, 0x0000
    mov di, word[TOTALSECTORCOUNT]
    mov di, 1146

READDATA:
    cmp di, 0
    je READEND
    sub di, 0x1

    mov ah, 0x02
    mov al, 0x1
    mov ch, byte[TRACKNUMBER]
    mov cl, byte[SECTORNUMBER]
    mov dh, byte[HEADNUMBER]
    mov dl, byte[BOOTDRIVE]

    int 0x13
    jc HANDLEDISKERROR

    add si, 0x0020
    mov es, si

    mov al, byte[SECTORNUMBER]
    add al, 0x01

    mov byte[SECTORNUMBER], al
    cmp al, byte[LASTSECTOR]
    jbe READDATA

    add byte[HEADNUMBER], 0x01
    mov byte[SECTORNUMBER], 0x01

    mov al, byte[LASTHEAD]
    cmp byte[HEADNUMBER], al
    ja ADDTRACK

    jmp READDATA

ADDTRACK:
    mov byte[HEADNUMBER], 0x00
    add byte[TRACKNUMBER], 0x01
    jmp READDATA

READEND:
    jmp 0x1000:0x0000

HANDLEDISKERROR:
    jmp $

SECTORNUMBER: db 0x02
HEADNUMBER: db 0x00
TRACKNUMBER: db 0x00

BOOTDRIVE: db 0x00
LASTSECTOR: db 0x00
LASTHEAD: db 0x00
LASTTRACK: db 0x00

times 510 - ($ - $$) db 0x00
db 0x55, 0xAA