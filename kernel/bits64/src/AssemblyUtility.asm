[BITS 64]

SECTION .text

; OBJECT :: ./src/AssemblyUtility.h
global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
global kSwitchContext
global kHlt

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

;;;;;;;;;;;;;;;;
; Task Function
;;;;;;;;;;;;;;;;


; Macro : Save Context & Selector Change
%macro KSAVECONTEXT 0 
    push rbp ; Insert Stack
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov ax, ds
    push rax
    mov ax, es
    push rax
    push fs
    push gs
%endmacro

; Macro : Restore Context
%macro KLOADCONTEXT 0
    pop gs
    pop fs
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
%endmacro

; Save Context in Curren Context
; Restore Context in Next task
; PARAM: Current Context, Next Context
kSwitchContext:
    push rbp
    mov rbp, rsp

    ; if Current Context is 'Null' -> no need to Save
    pushfq          ; register RFLAGS Save in Stack
    cmp rdi, 0      ; if not Curren Context is 'Null' -> move Context restore
    je .LoadContext
    popfq           ; Restore RFLAGS register

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Save the Context of the Curren Task
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    push rax

    mov ax, ss
    mov qword[rdi + (23 * 8)], rax

    mov rax, rbp
    add rax, 16
    mov qword[rdi + (22 * 8)], rax

    pushfq
    pop rax
    mov qword[rdi + (21 * 8)], rax

    mov ax, cs
    mov qword[rdi + (20 * 8)], rax

    mov rax, qword[rbp + 8]
    mov qword[rdi + (19 * 8)], rax

    pop rax
    pop rbp

    add rdi, (19 * 8)
    mov rsp, rdi
    sub rdi, (19 * 8)

    KSAVECONTEXT

    .LoadContext:
        mov rsp, rsi
        
        ; restore Register from Context data structure
        KLOADCONTEXT
        iretq

; Params: void
; Make CPU to State.IDLE
kHlt:
    hlt
    hlt
    ret