[BITS 64]
SECTION .text

; INPUT FUNCTION
extern kCommonExceptionHandler, kCommonInterruptHandler, kKeyboardHandler
extern kTimerHandler, kDeviceNotAvailableHandler

; OUTPUT FUNCTION
; Process Exception
global kISRDivideError, kISRDebug, kISRNMI, kISRBreakPoint, kISROverflow
global kISRBoundRangeExceeded, kISRInvalidOpcode, kISRDeviceNotAvailable, kISRDoubleFault
global kISRCoprocessorSegmentOverrun, kISRInvalidTSS, kISRSegmentNotPresent
global kISRStackSegmentFault, kISRGeneralProtection, kISRPageFault, kISR15
global kISRFPUError, kISRAlignmentCheck, kISRMachineCheck, kISRSIMDError, kISRETCException

; Process Interrupt
global kISRTimer, kISRKeyboard, kISRSlavePIC, kISRSerial2, kISRSerial1, kISRParallel2
global kISRFloppy, kISRParallel1, kISRRTC, kISRReserved, kISRNotUsed1, kISRNotUsed2
global kISRMouse, kISRCoprocessor, kISRHDD1, kISRHDD2, kISRETCInterrupt

; -----------------------------------------------------------------------------------------

; Store Context & Switch Selector __ (void)
%macro KSAVECONTEXT 0
    ; Push RBP - GS SEGMENT SELECTOR
    push rbp
    mov rbp, rsp

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

    ; SEGMENT
    mov ax, ds
    push rax

    mov ax, es
    push rax

    push fs
    push gs

    ; Switch Segment Selector
    mov ax, 0x10    ; Init AX(64 Kernel Data Segment Descriptor: 0x10-./bits32/Entry.s)
    mov ds, ax      ; DS~FS <- 64 Kernel Data Segment Descriptor(AX)
    mov es, ax
    mov gs, ax
    mov fs, ax
%endmacro

; RESTORE / LOAD Context __ (void)
%macro KLOADCONTEXT 0
    ; LOAD GS SEGMENT SELECTOR - RBP
    pop gs
    pop fs
    pop rax

    ; SEGMENT
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

; EXCEPTION HANDLER(0~31)
;   EX. NO: 0 - DIVIDE ERROR ISR
;   EX. NO: 1 - DEBUG ISR
;   EX. NO: 2 - NMI ISR
;   EX. NO: 3 - BREAKPOINT ISR
;   EX. NO: 4 - OVERFLOW ISR
;   EX. NO: 5 - BOUND RANGE EXCEEDED ISR
;   EX. NO: 6 - INVALID OPCODE ISR
;   EX. NO: 7 - DEVIDE NOT AVAILABLE ISR
;   EX. NO: 8 - DOUBLE FAULT ISR
;   EX. NO: 9 - COPROCESSOR SEGMENT OVERRUN ISR
;   EX. NO: 10 - INVALID TSS ISR
;   EX. NO: 11 - SEGMENT NOT PRESENT ISR
;   EX. NO: 12 - STACK SEGMENT FAULT ISR
;   EX. NO: 13 - GENERAL PROTECTION ISR
;   EX. NO: 14 - PAGE FAULT ISR
;   EX. NO: 15 - RESERVED ISR
;   EX. NO: 16 - FPU ISR
;   EX. NO: 17 - ALIGNMENT CHECK ISR
;   EX. NO: 18 - MACHINE CHECK ISR
;   EX. NO: 19 - SIMD FLOATING POINT EXCEPTION ISR
;   EX. NO: 20~31 - RESERVED ISR

; INTERRUPT HANDLER
;   INT. NO: 32 - TIMER ISR
;   INT. NO: 33 - KEYBOARD ISR
;   INT. NO: 34 - SLAVE PIC ISR
;   INT. NO: 35 - SERIAL PORT 2 ISR
;   INT. NO: 36 - SERIAL PORT 1 ISR
;   INT. NO: 37 - PARALLEL PORT 2 ISR
;   INT. NO: 38 - FLOPPY DISK CONTROLLER ISR
;   INT. NO: 39 - PARALLEL PORT 1 ISR
;   INT. NO: 40 - RTC ISRR
;   INT. NO: 41 - RESERVED INTERRUPT ISR
;   INT. NO: 42 - NOT USE
;   INT. NO: 43 - NOT USE
;   INT. NO: 44 - MOUSE ISR
;   INT. NO: 45 - CO-PROCESSOR ISR
;   INT. NO: 46 - HARD DISK 1 ISR
;   INT. NO: 47 - HARD DISK 2 ISR
;   INT. NO: 48 - ANOTHER INTERRUPT ISR

; ---------------------------------------
; EXCEPTION HADNLER
kISRDivideError:                ; EX. NO: 0 - DIVIDE ERROR ISR
    KSAVECONTEXT

    mov rdi, 0                      ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRDebug:                       ; EX. NO: 1 - DEBUG ISR
    KSAVECONTEXT

    mov rdi, 1                      ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRNMI:                        ; EX. NO: 2 - NMI ISR
    KSAVECONTEXT

    mov rdi, 2                      ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRBreakPoint:                 ; EX. NO: 3 - BREAKPOINT ISR
    KSAVECONTEXT

    mov rdi, 3                      ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISROverflow:                   ; EX. NO: 4 - OVERFLOW ISR
    KSAVECONTEXT

    mov rdi, 4                      ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRBoundRangeExceeded:         ; EX. NO: 5 - BOUND RANGE EXCEEDED ISR
    KSAVECONTEXT

    mov rdi, 5                      ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRInvalidOpcode:              ; EX. NO: 6 - INVALID OPCODE ISR
    KSAVECONTEXT

    mov rdi, 6                      ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRDeviceNotAvailable:         ; EX. NO: 7 - DEVIDE NOT AVAILABLE ISR
    KSAVECONTEXT

    mov rdi, 7                      ; SET EXCEPTION NUMBER
    call kDeviceNotAvailableHandler  

    KLOADCONTEXT
    iretq

kISRDoubleFault:                ; EX. NO: 8 - DOUBLE FAULT ISR
    KSAVECONTEXT

    mov rdi, 8                      ; SET EXCEPTION NUMBER
    mov rsi, qword[rbp + 8]         ; SET ERROR CODE
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    add rsp, 8
    iretq

kISRCoprocessorSegmentOverrun:  ; EX. NO: 9 - CO-PROCESSOR SEGMENT OVERRUN ISR
    KSAVECONTEXT

    mov rdi, 9                      ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRInvalidTSS:                 ; EX. NO: 10 - INVALID TSS ISR
    KSAVECONTEXT

    mov rdi, 10                     ; SET EXCEPTION NUMBER
    mov rsi, qword[rbp + 8]         ; SET ERROR CODE
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    add rsp, 8
    iretq

kISRSegmentNotPresent:           ; EX. NO: 11 - SEGMENT NOT PRESENT ISR
    KSAVECONTEXT

    mov rdi, 11                     ; SET EXCEPTION NUMBER
    mov rsi, qword[rbp + 8]         ; SET ERROR CODE
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    add rsp, 8
    iretq

kISRStackSegmentFault:          ; EX. NO: 12 - STACK SEGMENT FAULT ISR
    KSAVECONTEXT

    mov rdi, 12                     ; SET EXCEPTION NUMBER
    mov rsi, qword[rbp + 8]         ; SET ERROR CODE
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    add rsp, 8
    iretq

kISRGeneralProtection:          ; EX. NO: 13 - GENERAL PROTECTION ISR
    KSAVECONTEXT

    mov rdi, 13                     ; SET EXCEPTION NUMBER
    mov rsi, qword[rbp + 8]         ; SET ERROR CODE
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    add rsp, 8
    iretq

kISRPageFault:                  ; EX. NO: 14 - PAGE FAULT ISR
    KSAVECONTEXT

    mov rdi, 14                     ; SET EXCEPTION NUMBER
    mov rsi, qword[rbp + 8]         ; SET ERROR CODE
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    add rsp, 8
    iretq

kISR15:                         ; EX. NO: 15 - RESERVED ISR
    KSAVECONTEXT

    mov rdi, 15                     ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRFPUError:                    ; EX. NO: 16 - FPU ERROR ISR
    KSAVECONTEXT

    mov rdi, 16                     ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRAlignmentCheck:             ; EX. NO: 17 - ALIGNMENT CHECK ISR
    KSAVECONTEXT

    mov rdi, 17                     ; SET EXCEPTION NUMBER
    mov rsi, qword[rbp + 8]         ; SET ERROR CODE
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    add rsp, 8
    iretq

kISRMachineCheck:               ; EX. NO: 18 - MACHINE CHECK ISR
    KSAVECONTEXT

    mov rdi, 18                     ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRSIMDError:                  ; EX. NO: 19 - SIMD FLOATING POINT EXCEPTION ISR
    KSAVECONTEXT

    mov rdi, 19                     ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq

kISRETCException:               ; EX. NO: 20~31 - RESERVED ISR
    KSAVECONTEXT

    mov rdi, 20                     ; SET EXCEPTION NUMBER
    call kCommonExceptionHandler    ; CALL HANDLER

    KLOADCONTEXT
    iretq


; INTERRUPT HANDLER
kISRTimer:                      ; INT. NO: 32 - TIMER ISR
    KSAVECONTEXT

    mov rdi, 32                     ; SET INTERRUPT NUMBER
    call kTimerHandler              ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRKeyboard:                   ; INT. NO: 33 - KEYBOARD ISR
    KSAVECONTEXT

    mov rdi, 33                 ; SET INTERRUPT NUMBER
    call kKeyboardHandler       ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRSlavePIC:                   ; INT. NO: 34 - SLAVE PIC ISR
    KSAVECONTEXT

    mov rdi, 34                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRSerial2:                    ; INT. NO: 35 - SERIAL PORT 2 ISR
    KSAVECONTEXT

    mov rdi, 35                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRSerial1:                    ; INT. NO: 36 - SERIAL PORT 1 ISR
    KSAVECONTEXT

    mov rdi, 36                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRParallel2:                  ; INT. NO: 37 - PARALLEL PORT 2 ISR
    KSAVECONTEXT

    mov rdi, 37                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRFloppy:                     ; INT. NO: 38 - FLOPPY DISK CONTROLLER ISR
    KSAVECONTEXT

    mov rdi, 38                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRParallel1:                  ; INT. NO: 39 - PARALLEL PORT 1 ISR
    KSAVECONTEXT

    mov rdi, 39                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRRTC:                        ; INT. NO: 40 - RTC ISR
    KSAVECONTEXT

    mov rdi, 40                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRReserved:                   ; INT. NO: 41 - RESERVED INTERRUPT ISR
    KSAVECONTEXT

    mov rdi, 41                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRNotUsed1:                   ; INT. NO: 42 - NOT USE
    KSAVECONTEXT

    mov rdi, 42                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRNotUsed2:                   ; INT. NO: 43 - NOT USE
    KSAVECONTEXT

    mov rdi, 43                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRMouse:                      ; INT. NO: 44 - MOUSE ISR
    KSAVECONTEXT

    mov rdi, 44                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRCoprocessor:                ; INT. NO: 45 - CO-PROCESSOR ISR
    KSAVECONTEXT

    mov rdi, 45                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRHDD1:                       ; INT. NO: 46 - HARD DISK 1 ISR
    KSAVECONTEXT

    mov rdi, 46                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRHDD2:                       ; INT. NO: 47 - HARD DISK 2 ISR
    KSAVECONTEXT

    mov rdi, 47                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq

kISRETCInterrupt:               ; INT. NO: 48 - ANOTHER INTERRUPT ISR
    KSAVECONTEXT

    mov rdi, 48                     ; SET INTERRUPT NUMBER
    call kCommonInterruptHandler    ; RUN HANDLER

    KLOADCONTEXT
    iretq