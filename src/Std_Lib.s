section .text

global In
global Out

;--------------------------------------------------------------------;
; Input:                     
; ~~~~~
;       64-bit value in stack
; Output:
; ~~~~~~
;       None
; Destructed:
; ~~~~~~~~~~
;--------------------------------------------------------------------

Out:
    pop     r15             ; return address

    movsd   xmm0, qword [rsp]

    0xFFFFFFFFFFFFFFF7  ; sign bit mask

    push    r15
    ret

;--------------------------------------------------------------------;
; Input:                                                             ;
; ~~~~~~                                                             ;
;       rsi - address of a character to print                        ;
; Output:                                                            ;
; ~~~~~~                                                             ;
;       None                                                         ;
; Destructed:                                                        ;
; ~~~~~~~~~~                                                         ;
;       rax, rdx, rdi                                                ;
;--------------------------------------------------------------------;
Putchar:
        
    mov     rdi, 1          ; file descriptor (stdout)
    mov     rdx, 1          ; number of charcters to write
                            ; rsi already contains offset of a char
    mov     rax, 1          ; syscall number

    push    rcx
    push    r11
    syscall                 ; fuckes up rcx and r11
    pop     r11
    pop     rcx
    
    ret
;---------------------------------------------------------------
