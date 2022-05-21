# My processor commands, their nasm equivalents and x86-64 opcodes

## hlt

    MY ASSEMBLER:

        0x00
    
    NASM 64-bit:

        mov     rax, 0x3C
        xor     rdi, rdi
        syscall

    x86-64 OPCODE:  10 bytes

        0xB8 0x3C 0x00 0x00 0x00
        0x48 0x31 0xFF
        0x0F 0x05

## call: 

    MY ASSEMBLER:

        0x01 "ip_to_call"

    NASM:

        call    "procedure"

    x86-64 OPCODE:  5 byte
        
        0xE8 (shift: 4 bytes)

    Comments:

        int shift = x86_ip - x86_proc_ip;

## jmp: 

    MY ASSEMBLER:

        0x02 "ip_to_jump"

    NASM:

        jmp     "label"

    x86-64 OPCODE:  5 bytes

        0xE9 (shift: 4 bytes)

    Comments:

        int shift = x86_ip - x86_ip_to_jump;
        

## Conditional jumps:

    MY ASSEMBLER:

        0x03 "ip_to_jump"       // jae
        0x04     ...            // ja
        0x05     ...            // jbe
        0x06     ...            // jb
        0x07     ...            // je
        0x08     ...            // jne

    NASM:

        pop     rsi
        pop     rdi
        cmp     rdi, rsi
        je      "label"

    x86-64 OPCODE:  11 bytes

        0x5E
        0x5F
        0x48 0x39 0xF7

        0x0F 0x83 (shift: 4 bytes)     // jae
        ...  0x87       ...            // ja
        ...  0x86       ...            // jbe
        ...  0x82       ...            // jb
        ...  0x84       ...            // je
        ...  0x85       ...            // jne

    Comments:

        int shift = x86_ip - x86_ip_to_jump;

## ret: 

    MY ASSEMBLER:

        0x09

    NASM 64-bit:

        ret

    x86-64 OPCODE:  1 byte

        0xC3

## in:

    MY ASSEMBLER:

        0x0A
    
    ALIGNED VERSION:
    
        NASM:

            push    rdi
            
            push    rax
            push    rbx
            push    rcx
            push    rdx
            
            lea     rdi, [rsp + 32] 
            call    In

            pop     rdx
            pop     rcx
            pop     rbx
            pop     rax

        x86-64 OPCODE:      19 bytes

            0x57

            0x50
            0x53
            0x51
            0x52

            0x48 0x8D 0x7C 0x24 0x20
            0xE8 (In relative address: 4 bytes)

            0x5A
            0x59
            0x5B
            0x58

    UNALIGNED VERSION:

        NASM:

            push    rdi
            push    rdi
            
            push    rax
            push    rbx
            push    rcx
            push    rdx
            
            lea     rdi, [rsp + 40]
            call    In

            pop     rdx
            pop     rcx
            pop     rbx
            pop     rax

            pop     rdi

        x86-64 OPCODE:      21 byte

            0x57
            0x57

            0x50
            0x53
            0x51
            0x52

            0x48 0x8D 0x7C 0x24 0x28
            0xE8 (In relative address: 4 bytes)

            0x5A
            0x59
            0x5B
            0x58

            0x5F

## out:

    MY ASSEMBLER:

        0x0B

    NASM:

        movsd   xmm0, qword [rsp]

        push    rax
        push    rbx
        push    rcx
        push    rdx

        call    Out

        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax

        pop     rdi

    x86-64 OPCODE:      19 bytes

        0xF2 0x0F 0x10 0x04 0x24
        0x48 0x83 0xC4 0x08

        0x50
        0x53
        0x51
        0x52

        0xE8 (Out relative address: 4 bytes)

        0x5A
        0x59
        0x5B
        0x58

        0x5F

## push:

### General form in my assembler:

    0x0C "if_ram" "reg_num" "if_num" (number)

### push "number"

    MY ASSEMBLER:

        0x0C 0x00 0x00 0x01 "number"

    NASM:

        mov     rdi, (number: 8 bytes)
        push    rdi

    x86-64 OPCODES:     11 bytes

        0x48 0xBF (number: 8 bytes)
        0x57

### push ["number"]

    MY ASSEMBLER:
    
    0x0C 0x01 0x00 0x01 (number: 4 bytes)

    NASM:
    
        mov     rdi, qword ["number"]
        push    rdi

    x86-64 OPCODES:     9 bytes

        0x48 0x8B 0x3C 0x25 (number: 4 bytes)
        0x57

### push "register"

    MY ASSEMBLER:

        0x0C 0x00 0x01 0x00     // push ax
        ...  ...  0x02 ...      // push bx
        ...  ...  0x03 ...      // push cx
        ...  ...  0x04 ...      // push dx

    NASM:

        push    rax

    x86-64 OPCODES:     1 byte

        0x50                    // rax
        0x53                    // rbx
        0x51                    // rcx
        0x52                    // rdx

### push ["register"]

    MY ASSEMBLER:

        0x0C 0x01 0x01 0x00     // push [ax]
        ...  ...  0x02 ...      // push [bx]
        ...  ...  0x03 ...      // push [cx]
        ...  ...  0x04 ...      // push [dx]
    NASM:

        mov     rdi, qword [rax] 
        push    rdi

    x86-64 OPCODES:     4 bytes

        0x48 0x8B 0x38          // rax
        ...  ...  0x3B          // rbx
        ...  ...  0x39          // rcx
        ...  ...  0x3A          // rdx

        0x57

### push ["register" + "number"]

    MY ASSEMBLER:
    
        0x0C 0x01 0x01 0x01 (number: 4 bytes)    // push [ax + "number"]
        ...  ...  0x02 ...         ...           // push [bx + "number"]
        ...  ...  0x03 ...         ...           // push [cx + "number"]
        ...  ...  0x04 ...         ...           // push [dx + "number"]

    NASM:
    
        mov     rdi, qword [rax + "number"]
        push    rdi

    x86-64 OPCODES:     8 bytes

        0x48 0x8B 0xB8 (number: 4 bytes)   // rax
        ...  ...  0xBB        ...          // rbx
        ...  ...  0xB9        ...          // rcx
        ...  ...  0xBA        ...          // rdx

        0x57

## pop:

### General form in my assembler:

    0x0D "if_ram" "reg_num" "if_num" (num)

### pop

    MY ASSEMBLER:

        0x0D 0x00 0x00 0x00

    NASM:

        pop     rdi

    x86-64 OPCODE:      1 byte

        0x5F

### pop ["number"]

    MY ASSEMBLER:
    
        0x0D 0x01 0x00 0x01 (number: 4 bytes)

    NASM:
    
        pop     rdi
        mov     ["number"], rdi

    x86-64 OPCODES:     9 bytes

        0x5F
        0x48 0x89 0x3C 0x25 (number: 4 bytes)

### pop "register"

    MY ASSEMBLER:

        0x0D 0x00 0x01 0x00     // pop ax
        ...  ...  0x02 ...      // pop bx
        ...  ...  0x03 ...      // pop cx
        ...  ...  0x04 ...      // pop dx

    NASM:

        pop     rax

    x86-64 OPCODES:     1 byte

        0x58                    // rax
        0x5B                    // rbx
        0x59                    // rcx
        0x5A                    // rdx

### pop ["register"]

    MY ASSEMBLER:

        0x0D 0x01 0x01 0x00     // pop [ax]
        ...  ...  0x02 ...      // pop [bx]
        ...  ...  0x03 ...      // pop [cx]
        ...  ...  0x04 ...      // pop [dx]

    NASM:

        pop     rdi
        mov     [rax], rdi

    x86-64 OPCODES:     4 bytes

        0x5F
        
        0x48 0x89 0x38          // rax
        ...  ...  0x3B          // rbx
        ...  ...  0x39          // rcx
        ...  ...  0x3A          // rdx

### pop ["register" + "number"]

    MY ASSEMBLER:
    
        0x0D 0x01 0x01 0x01 (number: 4 bytes)    // pop [ax + "number"]
        ...  ...  0x02 ...         ...           // pop [bx + "number"]
        ...  ...  0x03 ...         ...           // pop [cx + "number"]
        ...  ...  0x04 ...         ...           // pop [dx + "number"]

    NASM:
    
        pop     rdi
        mov     [rax + "number"], rdi

    x86-64 OPCODES:     8 bytes

        0x5F
        
        0x48 0x89 0xB8 (number: 4 bytes)   // rax
        ...  ...  0xBB        ...          // rbx
        ...  ...  0xB9        ...          // rcx
        ...  ...  0xBA        ...          // rdx

## Math functions

    MY ASSEMBLER:
    
        0x0E                            // add
        0x0F                            // sub
        0x10                            // mul
        0x11                            // div

    NASM:

        movsd   xmm1, qword [rsp + 8]
        movsd   xmm2, qword [rsp]
        add     rsp, 8

        addsd   xmm1, xmm2              // add
        subsd      ...                  // sub
        mulsd      ...                  // mul
        divsd      ...                  // div

        movsd   qword [rsp], xmm1

    x86-64 OPCODES:     24 bytes

        0xF2 0x0F 0x10 0x4C 0x24 0x08
        0xF2 0x0F 0x10 0x14 0x24
        0x48 0x83 0xC4 0x08

        0xF2 0x0F 0x58 0xCA             // add
        ...  ...  0xCA ...              // sub
        ...  ...  0x59 ...              // mul
        ...  ...  0x5E ...              // div

        0xF2 0x0F 0x11 0x0C 0x24


## Sqrt

    MY_ASEMBLER:

        0x12

    NASM:

        movsd   xmm0, qword [rsp]
        sqrtpd  xmm0, xmm0
        movsd   qword [rsp], xmm0

    x86-64 OPCODES:     14 bytes

        0xF2 0x0F 0x10 0x04 0x24
        0x66 0x0F 0x51 0xC0
        0xF2 0x0F 0x11 0x04 0x24
