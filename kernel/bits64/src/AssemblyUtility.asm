[BITS 64]

SECTION .text

; OBJECT :: ./src/AssemblyUtility.h
global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC

; Read 1 byte to the port
; PARAM: WORD wPort
kInPortByte:
    push rdx

    mov rdx, rdi    ; RDX <- PORT NUMBER
    mov rax, 0      ; RAX INIT.
    in al, dx       ; IN: Input data to AX from port indicated by operand

    pop rdx
    ret

; Write 1 byte to the port
; PARAM: WORD wPort, BYTE bData
kOutPortByte:
    push rdx
    push rax

    mov rdx, rdi
    mov rax, rsi
    out dx, al      ; AX data output to the port indicated by the operand
                    ; Writes one byte stored in the AL register to the port address stored in the DX register
    pop rax
    pop rdx         
    ret

; PARAM: QWORD qwGDTRAddress
kLoadGDTR:
    lgdt [rdi]
    ret

; PARAM: WORD wTSSSegmentOffset
kLoadTR:
    ltr di
    ret

; PARAM: QWORD qwIDTRAddress
kLoadIDTR:
    lidt [rdi]
    ret

kEnableInterrupt:
    sti
    ret

kDisableInterrupt:
    cli
    ret

kReadRFLAGS:
    pushfq
    pop rax

    ret

; READ AND RETURN TSC(TIME STAMP COUNTER)
kReadTSC:
    push rdx

    rdtsc

    shl rdx, 32
    or rax, rdx

    pop rdx
    ret