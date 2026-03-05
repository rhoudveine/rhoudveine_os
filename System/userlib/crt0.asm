; crt0.asm - C runtime startup for userspace programs
; This is the entry point that the kernel jumps to

section .text
global _start
extern main

_start:
    ; Clear frame pointer for stack traces
    xor rbp, rbp
    
    ; Arguments (argc, argv) are passed in RDI, RSI by kernel
    ; Just call main directly
    call main
    
    ; Return to the kernel (the loader called us with 'call')
    ret
