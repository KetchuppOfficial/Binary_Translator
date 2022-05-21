#include "../include/Binary_Translator.h"

//=====================================================================================//
//                                    FIRST PASSING                                    //
//=====================================================================================//

#define DEFCMD_(num, name, n_args) name = num,

enum Instructions
{
    #include "../include/Commands_List.h"
};

#undef DEFCMD_

enum PUSH_POP
{
    EMPTY      =   0,   // pop

    AX         =  10,   // push/pop ax
    BX         =  20,   // push/pop bx
    CX         =  30,   // push/pop cx
    DX         =  40,   // push/pop dx

    RAM_AX     =  11,   // push/pop [ax]
    RAM_BX     =  21,   // push/pop [bx]
    RAM_CX     =  31,   // push/pop [cx]
    RAM_DX     =  41,   // push/pop [dx]

    NUM        = 100,   // push 4
    RAM_NUM    = 101,   // push/pop [4]

    RAM_AX_NUM = 111,   // push/pop [ax + 4]
    RAM_BX_NUM = 121,   // push/pop [bx + 4]
    RAM_CX_NUM = 131,   // push/pop [cx + 4]
    RAM_DX_NUM = 141,   // push/pop [dx + 4]   
};

struct Jump
{
    int from;
    int to;
    int x86_from;
    enum Instructions type;
};

struct Bin_Tr
{
    char *input_buff;
    char *x86_buff;
    int max_ip;
    int x86_max_ip;
};

static struct Jump *First_Passing (struct Bin_Tr *bin_tr, int *const n_jumps)
{
    MY_ASSERT (bin_tr,             "struct Bin_Tr bin_tr",    NULL_PTR, NULL);
    MY_ASSERT (bin_tr->input_buff, "const char *const input", NULL_PTR, NULL);
    MY_ASSERT (n_jumps,            "int *const n_jumps",      NULL_PTR, NULL);

    const int   max_ip   = bin_tr->max_ip;
    const char *proc_buff = bin_tr->input_buff;

    struct Jump *jumps_arr = (struct Jump *)calloc (max_ip, sizeof (struct Jump));
    MY_ASSERT (jumps_arr, "struct Jump *jumps_arr", NE_MEM, NULL);
    int jump_i = 0;

    int x86_ip = 0;
    int rsp = 0;

    for (int ip = 0; ip < max_ip; )
    {
        switch (proc_buff[ip])
        {
            case hlt:
                x86_ip += 10;   // look in Commands.md
                ip++;
                break;

            case call:
            case jmp:
            case jae:
            case ja:
            case jbe:
            case jb:
            case jne:
            case je:
            {
                int jump_to = *(int *)(proc_buff + ip + 1);
                jumps_arr[jump_i].to = jump_to;
                jumps_arr[jump_i].from = ip;

                if (proc_buff[ip] == call || proc_buff[ip] == jmp)
                {
                    jumps_arr[jump_i].x86_from = x86_ip;        // ip of the first byte of jump
                    x86_ip += 5;    // look in Commands.md
                }
                else
                {
                    jumps_arr[jump_i].x86_from = x86_ip + 5;    // ip of the first byte of jump
                    x86_ip += 11;   // look in Commands.md
                    rsp += 16;
                }

                jumps_arr[jump_i++].type = (int)proc_buff[ip];
                ip += 1 + sizeof (int);

                break;
            }

            case ret:
                x86_ip++;   // look in Commands.md
                ip++;
                break;

            case in:
                ip++;
                x86_ip = (rsp % 16 == 0) ? x86_ip + 19 : x86_ip + 21;   // look in Commands.md
                rsp -= 8;
                break;

            case out:
                ip++;
                x86_ip += 19;   // look in Commands.md
                rsp += 8;
                break;

            case push:
            case pop:
            {
                ip++;
                int checksum = proc_buff[ip] + 10 * proc_buff[ip + 1] + 100 * proc_buff[ip + 2];
                //                  |                     |                        |
                //                if RAM                if reg                   if num

                switch (checksum)
                {                    
                    case EMPTY:
                        ip += 3;
                        x86_ip++;       // look in Commands.md
                        break;
                    
                    case NUM:
                        ip += 3 + sizeof (double);
                        x86_ip += 11;   // look in Commands.md
                        break;

                    case RAM_NUM:
                        ip += 3 + sizeof (int);
                        x86_ip += 9;    // look in Commands.md
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        ip += 3;
                        x86_ip++;       // look in Commands.md
                        break;

                    case RAM_AX:
                    case RAM_BX:
                    case RAM_CX:
                    case RAM_DX:
                        ip += 3;
                        x86_ip += 4;    // look in Commands.md
                        break;

                    case RAM_AX_NUM:
                    case RAM_BX_NUM:
                    case RAM_CX_NUM:
                    case RAM_DX_NUM:
                        x86_ip += 8;    // look in Commands.md
                        ip += 3 + sizeof (int);
                        break;

                    default:
                        MY_ASSERT (false, "int checksum", UNEXP_VAL, NULL);
                        break;
                }

                rsp = (proc_buff[ip] == push) ? rsp - 8 : rsp + 8;

                break;
            }

            case add:
            case sub:
            case mul:
            case dvd:
                x86_ip += 24;   // look in Commands.md
                ip++;
                rsp += 8;
                break;

            case Sqrt:
                x86_ip += 14;   // look in Commands.md
                ip++;
                break;

            default: 
                MY_ASSERT (false, "proc_buff[ip]", UNDEF_CMD, NULL);
                break;
        }
    }

    bin_tr->x86_max_ip = x86_ip;

    *n_jumps = jump_i;
    jumps_arr = realloc (jumps_arr, jump_i * sizeof (struct Jump));

    return jumps_arr;
}

//=====================================================================================//

//=====================================================================================//
//                                   SECOND PASSING                                    //
//=====================================================================================//

static inline void Put_In_x86_Buffer (char *const x86_buffer, int *const x86_ip, const char *const opcode, const size_t opcode_size)
{
    memcpy (x86_buffer + *x86_ip, opcode, opcode_size);
    *x86_ip += opcode_size;
}

static inline void Translate_Hlt (char *const x86_buffer, int *const x86_ip)
{
    char opcode[] = {
                        0xB8, 0x3C, 0x00, 0x00, 0x00,   // mov rax, 0x3C
                        0x48, 0x31, 0xFF,               // xor rdi, rdi
                        0x0F, 0x05                      // syscall
                    };

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

static inline void Translate_Ret (char *const x86_buffer, int *const x86_ip)
{
    x86_buffer[(*x86_ip)++] = 0xC3;     // ret
}

static inline void Translate_Call (char *const x86_buffer, int *const x86_ip)
{
    const char opcode[] = {
                            0xE8,                       // call "procedure"
                            0x00, 0x00, 0x00, 0x00      // <-- procedure relative offset (will be changes while 3rd passing)
                          };
    
    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

static inline void Translate_Jmp (char *const x86_buffer, int *const x86_ip)
{
    const char opcode[] = {
                            0xE9,                       // jmp "label"
                            0x00, 0x00, 0x00, 0x00      // <-- label relative offset (will be changes while 3rd passing)
                          };
    
    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

static inline int Translate_Conditional_Jmp (char *const x86_buffer, int *const x86_ip, const enum Instructions jcc)
{
    char opcode[] = {
                        0x5E,                   // pop rsi           
                        0x5F,                   // pop rdi
                        0x48, 0x39, 0xF7,       // cmp rdi, rsi
                        0x0F, 0x00,             // jcc "label"
                        0x00, 0x00, 0x00, 0x00  // <-- label relative offset (will be changed while 3rd passing)
                    };

    switch (jcc)
    {
        case jae:
            opcode[6] = 0x83;
            break;
        case ja:
            opcode[6] = 0x87;
            break;
        case jbe:
            opcode[6] = 0x86;
            break;
        case jb:
            opcode[6] = 0x82;
            break;
        case je:
            opcode[6] = 0x84;
            break;
        case jne:
            opcode[6] = 0x85;
            break;

        default:
            MY_ASSERT (false, "const enum Instructions jcc", UNEXP_VAL, ERROR);
    }

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
    
    return NO_ERRORS;
}

static inline void In (double *num_ptr)
{
    printf ("Write a number: ");
    scanf ("%lf", num_ptr);
}

static inline void Translate_In_Unaligned (char *const x86_buffer, int *const x86_ip)
{
    char opcode[] = {
                        0x57,   // push rdi <---------------------------------------+
                                //                                                  |
                        0x57,   // push rdi <--- for alignment                      |
                                //                                                  |
                        0x50,   // push rax                                         |
                        0x53,   // push rbx                                         | 
                        0x51,   // push rcx                                         |
                        0x52,   // push rdx                                         |
                                                        //                          |
                        0x48, 0x8D, 0x7C, 0x24, 0x28,   // lea rdi, [rsp + 40] -----+
                        0xE8, 0x00, 0x00, 0x00, 0x00,   // call In
                                                        //
                        0x5A,   // pop rdx
                        0x59,   // pop rcx
                        0x5B,   // pop rbx
                        0x58,   // pop rax

                        0x5F    // pop rdi
                    };

    *(uint32_t *)(opcode + 12) = (uint64_t )In - (uint64_t)(x86_buffer + *x86_ip + 12 + sizeof (int));
    // 12 - offset of call relatively to the beginning of opcode

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

static inline void Translate_In_Aligned (char *const x86_buffer, int *const x86_ip)
{
    char opcode[] = {
                        0x57,   // push rdi <---------------------------------------+
                                //                                                  |
                        0x50,   // push rax                                         |
                        0x53,   // push rbx                                         |
                        0x51,   // push rcx                                         |
                        0x52,   // push rdx                                         |
                                                        //                          |
                        0x48, 0x8D, 0x7C, 0x24, 0x20,   // lea rdi, [rsp + 32] -----+
                        0xE8, 0x00, 0x00, 0x00, 0x00,   // call In
                                                        //
                        0x5A,   // pop rdx
                        0x59,   // pop rcx
                        0x5B,   // pop rbx
                        0x58    // pop rax
                    };

    *(uint32_t *)(opcode + 11) = (uint64_t )In - (uint64_t)(x86_buffer + *x86_ip + 11 + sizeof (int));
    // 11 - offset of call relatively to the beginning of opcode

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

static inline void Out (const double number)
{
    printf ("%g\n", number);
}

static inline void Translate_Out (char *const x86_buffer, int *const x86_ip)
{
    char opcode[] = {
                        0xF2, 0x0F, 0x10, 0x04, 0x24,   // movsd   xmm0, qword [rsp]

                        0x50,        // push rax
                        0x53,        // push rbx
                        0x51,        // push rcx
                        0x52,        // push rdx

                        0xE8, 0x00, 0x00, 0x00, 0x00,   // call Out

                        0x5A,       // pop rdx
                        0x59,       // pop rcx
                        0x5B,       // pop rbx
                        0x58,       // pop rax

                        0x5F
                    };

    *(uint32_t *)(opcode + 10) = (uint64_t )Out - (uint64_t)(x86_buffer + *x86_ip + 10 + sizeof (int));
    // 15 - offset of call relatively to the beginning of opcode

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

static inline void Translate_Pop (char *const x86_buff, int *const x86_ip)
{
    x86_buff[(*x86_ip)++] = 0x5F;
}

static inline void Translate_Push_Num (char *const x86_buffer, int *const x86_ip, const double num)
{    
    char opcode[] = {
                        0x48, 0xBF,                                         // mov rdi, 0
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // (0 is changed below)

                        0x57                                                // push rdi
                    };

    *(double *)(opcode + 2) = num;

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

static inline void Translate_Push_RAM_Num (char *const x86_buffer, int *const x86_ip, const int num)
{
    char opcode[] = {
                        0x48, 0x8B, 0x3C, 0x25,     // mov rdi, qword [0]
                        0x00, 0x00, 0x00, 0x00,     // (0 is changed below)

                        0x57                        // push rdi
                    };

    *(int *)(opcode + 4) = num;

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

static inline void Translate_Pop_RAM_Num (char *const x86_buffer, int *const x86_ip, const int num)
{
    char opcode[] = {
                        0x5F,                       // pop rdi

                        0x48, 0x89, 0x3C, 0x25,     // mov [0], rdi
                        0x00, 0x00, 0x00, 0x00,     // (0 is changed below)
                    };

    *(int *)(opcode + 5) = num;

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

enum Registers
{
    ax = 0x01,
    bx = 0x02,
    cx = 0x03,
    dx = 0x04
};

static inline int Translate_Push_Reg (char *const x86_buffer, int *const x86_ip, const enum Registers reg)
{
    char opcode = 0;

    switch (reg)
    {
        case ax:
            opcode = 0x50;      // push rax
            break;
        case bx:
            opcode = 0x53;      // push rbx
            break;
        case cx:
            opcode = 0x51;      // push rcx
            break;
        case dx:
            opcode = 0x52;      // push rdx
            break;

        default:
            MY_ASSERT (false, "const enum Registers reg", UNEXP_VAL, ERROR);
            break;
    }

    x86_buffer[(*x86_ip)++] = opcode;

    return NO_ERRORS;
}

static inline int Translate_Pop_Reg (char *const x86_buffer, int *const x86_ip, const enum Registers reg)
{
    char opcode = 0;

    switch (reg)
    {
        case ax:
            opcode = 0x58;      // pop rax
            break;
        case bx:
            opcode = 0x5B;      // pop rbx
            break;
        case cx:
            opcode = 0x59;      // pop rcx
            break;
        case dx:
            opcode = 0x5A;      // pop rdx
            break;

        default:
            MY_ASSERT (false, "const enum Registers reg", UNEXP_VAL, ERROR);
            break;
    }

    x86_buffer[(*x86_ip)++] = opcode;

    return NO_ERRORS;
}

static inline int Translate_Push_RAM_Reg (char *const x86_buffer, int *const x86_ip, const enum Registers reg)
{
    char opcode[] = {
                        0x48, 0x8B, 0x00,     // mov rdi, qword [r?x]
                                //    |
                        0x57    //    +--- 0 is changed below  
                    //    | 
                    //    +---- push rdi              
                    };

    switch (reg)
    {
        case ax:
            opcode[2] = 0x38;      // rax
            break;
        case bx:
            opcode[2] = 0x3B;      // rbx
            break;
        case cx:
            opcode[2] = 0x39;      // rcx
            break;
        case dx:
            opcode[2] = 0x3A;      // rdx
            break;

        default:
            MY_ASSERT (false, "const enum Registers reg", UNEXP_VAL, ERROR);
            break;   
    }

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);

    return NO_ERRORS;
}

static inline int Translate_Pop_RAM_Reg (char *const x86_buffer, int *const x86_ip, const enum Registers reg)
{
    char opcode[] = {
                        0x5F,               // pop rdi
                        
                        0x48, 0x89, 0x00,   // mov rdi, qword [r?x]
                        //           |
                        //           +--- 0 is changed below               
                    };

    switch (reg)
    {
        case ax:
            opcode[3] = 0x38;      // rax
            break;
        case bx:
            opcode[3] = 0x3B;      // rbx
            break;
        case cx:
            opcode[3] = 0x39;      // rcx
            break;
        case dx:
            opcode[3] = 0x3A;      // rdx
            break;

        default:
            MY_ASSERT (false, "const enum Registers reg", UNEXP_VAL, ERROR);
            break;   
    }

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);

    return NO_ERRORS;
}

static inline int Translate_Push_RAM_Reg_Num (char *const x86_buffer, int *const x86_ip, const enum Registers reg, const int num)
{    
    char opcode[] = {
                        0x48, 0x8B, 0x00,       // mov rdi, qword [r?x + num]
                        0x00, 0x00, 0x00, 0x00, // <-- num
                        0x57                    // push rdi
                    }; 
    
    switch (reg)
    {
        case ax:
            opcode[2] = 0xB8;      // rax
            break;
        case bx:
            opcode[2] = 0xBB;      // rbx
            break;
        case cx:
            opcode[2] = 0xB9;      // rcx
            break;
        case dx:
            opcode[2] = 0xBA;      // rdx
            break;

        default:
            MY_ASSERT (false, "const enum Registers reg", UNEXP_VAL, ERROR);  
            break;
    }

    *(int *)(opcode + 3) = num;

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);

    return NO_ERRORS;
}

static inline int Translate_Pop_RAM_Reg_Num (char *const x86_buffer, int *const x86_ip, const enum Registers reg, const int num)
{    
    char opcode[] = {
                        0x5F,                   // pop rdi
                        0x48, 0x89, 0x00,       // mov rdi, qword [r?x + num]
                        0x00, 0x00, 0x00, 0x00, // <-- num
                    }; 

    switch (reg)
    {
        case ax:
            opcode[3] = 0xB8;      // rax
            break;
        case bx:
            opcode[3] = 0xBB;      // rbx
            break;
        case cx:
            opcode[3] = 0xB9;      // rcx
            break;
        case dx:
            opcode[3] = 0xBA;      // rdx
            break;

        default:
            MY_ASSERT (false, "const enum Registers reg", UNEXP_VAL, ERROR);  
            break;
    }
    
    *(int *)(opcode + 4) = num;

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);

    return NO_ERRORS;
}

static int Translate_Arithmetics (char *const x86_buffer, int *const x86_ip, const enum Instructions instruction)
{
    const char first_part[] = {
                                0xF2, 0x0F, 0x10, 0x4C, 0x24, 0x08,     // movsd   xmm1, qword [rsp + 8]
                                0xF2, 0x0F, 0x10, 0x14, 0x24,           // movsd   xmm2, qword [rsp]
                                0x48, 0x83, 0xC4, 0x08                  // add     rsp, 8
                              };

    Put_In_x86_Buffer (x86_buffer, x86_ip, first_part, sizeof first_part);
    
    char math_instruction[] = {0xF2, 0x0F, 0x00, 0xCA};
    //                                       |
    //           this byte will be changed --+

    switch (instruction)
    {
        case add:
            math_instruction[2] = 0x58;     // addsd   xmm1, xmm2
            break;
        case sub:
            math_instruction[2] = 0x5C;     // subsd   xmm1, xmm2
            break;
        case mul:
            math_instruction[2] = 0x59;     // mulsd   xmm1, xmm2
            break;
        case dvd:
            math_instruction[2] = 0x5E;     // divsd   xmm1, xmm2
            break;

        default:
            MY_ASSERT (false, "const enum Instructions instruction", UNEXP_VAL, ERROR);
    }

    Put_In_x86_Buffer (x86_buffer, x86_ip, math_instruction, sizeof math_instruction);

    const char last_part[] = {
                                0xF2, 0x0F, 0x11, 0x0C, 0x24    // movsd   qword [rsp], xmm1
                             };

    Put_In_x86_Buffer (x86_buffer, x86_ip, last_part, sizeof last_part);

    return NO_ERRORS;
}

static inline void Translate_Sqrt (char *const x86_buffer, int *const x86_ip)
{
    const char opcode[] = {
                            0xF2, 0x0F, 0x10, 0x04, 0x24,   // movsd   xmm0, qword [rsp]
                            0x66, 0x0F, 0x51, 0xC0,         // sqrtpd  xmm0, xmm0
                            0xF2, 0x0F, 0x11, 0x04, 0x24    // movsd   qword [rsp], xmm0
                          };
    
    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

#define PAGE_SIZE 4096
static void *aligned_calloc (const size_t n_elems, const size_t one_elem_size)
{
    size_t new_size = ((n_elems * one_elem_size >> 12) + 1) << 12;     // 2^12 = 4096 - page size in Linux
    
    void *array = aligned_alloc (PAGE_SIZE, new_size);

    memset (array, 0, new_size);

    return array;
}
#undef PAGE_SIZE

int Second_Passing (struct Bin_Tr *const bin_tr)
{
    MY_ASSERT (bin_tr, "struct Bin_Tr *const bin_tr", NULL_PTR, ERROR);
    
    bin_tr->x86_buff = (char *)aligned_calloc (bin_tr->x86_max_ip, sizeof (char *));
    MY_ASSERT (bin_tr->x86_buff, "bin_tr->x86_buffer", NE_MEM, ERROR);

    const char *proc_buff   = bin_tr->input_buff;
    char       *x86_buffer = bin_tr->x86_buff;
    const int  max_ip      = bin_tr->max_ip;

    int rsp = 0;
    for (int ip = 0, x86_ip = 0; ip < max_ip; )
    {
        switch (proc_buff[ip])
        {
            case hlt:
                Translate_Hlt (x86_buffer, &x86_ip);
                ip++;
                break;
            
            case call:
                Translate_Call (x86_buffer, &x86_ip);
                ip += 1 + sizeof (int);
                break;

            case jmp:
                Translate_Jmp (x86_buffer, &x86_ip);
                ip += 1 + sizeof (int);
                break;
            
            case jae:
            case ja:
            case jbe:
            case jb:
            case jne:
            case je:
            {
                int jcc = (int)proc_buff[ip];
                
                Translate_Conditional_Jmp (x86_buffer, &x86_ip, jcc);
                ip += 1 + sizeof (int);
                rsp += 16;

                break;
            }

            case ret:
                Translate_Ret (x86_buffer, &x86_ip);
                ip++;
                break;
            
            case in:
                if (rsp % 16 == 0)
                    Translate_In_Aligned (x86_buffer, &x86_ip);
                else
                    Translate_In_Unaligned (x86_buffer, &x86_ip);

                ip++;
                rsp -= 8;
                break;

            case out:
                Translate_Out (x86_buffer, &x86_ip);

                ip++;
                rsp += 8;
                break;

            case push:
            case pop:
            {
                const char instr_name = proc_buff[ip];

                ip++;
                int checksum = proc_buff[ip] + 10 * proc_buff[ip + 1] + 100 * proc_buff[ip + 2];
                //                  |                     |                        |
                //                if RAM                if reg                   if num
                
                switch (checksum)
                {
                    case EMPTY:
                        Translate_Pop (x86_buffer, &x86_ip);
                        ip += 3;
                        break;
                    
                    case NUM:
                    {
                        const double num = *(double *)(proc_buff + ip + 3);

                        Translate_Push_Num (x86_buffer, &x86_ip, num);
                        ip += (3 + sizeof (double));

                        break;
                    }

                    case RAM_NUM:
                    {
                        const int num = *(int *)(proc_buff + ip + 3);
                        
                        if (instr_name == push)
                            Translate_Push_RAM_Num (x86_buffer, &x86_ip, num);
                        else
                            Translate_Pop_RAM_Num  (x86_buffer, &x86_ip, num);
                        ip += (3 + sizeof (int));

                        break;
                    }

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                    {
                        const char reg = proc_buff[ip + 1];

                        if (instr_name == push)
                            Translate_Push_Reg (x86_buffer, &x86_ip, (int)reg);
                        else
                            Translate_Pop_Reg  (x86_buffer, &x86_ip, (int)reg);
                        ip += 3;

                        break;
                    }

                    case RAM_AX:
                    case RAM_BX:
                    case RAM_CX:
                    case RAM_DX:
                    {
                        const char reg = proc_buff[ip + 1];

                        if (instr_name == push)
                            Translate_Push_RAM_Reg (x86_buffer, &x86_ip, (int)reg);
                        else
                            Translate_Pop_RAM_Reg  (x86_buffer, &x86_ip, (int)reg);
                        ip += 3;
                        
                        break;
                    }

                    case RAM_AX_NUM:
                    case RAM_BX_NUM:
                    case RAM_CX_NUM:
                    case RAM_DX_NUM:
                    {
                        const char reg = proc_buff[ip + 1];
                        const int num  = *(int *)(proc_buff + ip + 3);

                        if (instr_name == push)
                            Translate_Push_RAM_Reg_Num (x86_buffer, &x86_ip, (int)reg, num);
                        else
                            Translate_Pop_RAM_Reg_Num  (x86_buffer, &x86_ip, (int)reg, num);
                        ip += (3 + sizeof (int));

                        break;
                    }

                    default:
                        MY_ASSERT (false, "int checksum", UNEXP_VAL, ERROR);
                        break;
                }

                rsp = (proc_buff[ip] == push) ? rsp - 8 : rsp + 8;

                break;
            }           

            case add:
            case sub:
            case mul:
            case dvd:
                Translate_Arithmetics (x86_buffer, &x86_ip, proc_buff[ip]);
                ip++;
                rsp += 8;
                break;

            case Sqrt:
                Translate_Sqrt (x86_buffer, &x86_ip);
                ip++;
                break;

            default: MY_ASSERT (false, "proc_buff[ip]", UNDEF_CMD, ERROR);
        }
    }

    return NO_ERRORS;
}

static int Compare_Jumps (const void *jump_v1, const void *jump_v2)
{
    const struct Jump *jump_1 = (const struct Jump *)jump_v1;
    const struct Jump *jump_2 = (const struct Jump *)jump_v2;
    
    if (jump_1->to < jump_2->to)
        return -1;
    else if (jump_1->to > jump_2->to)
        return 1;
    else
        return 0;
}

static void Sort_Jumps (struct Jump *const jumps_arr, const int n_jumps)
{
    qsort (jumps_arr, n_jumps, sizeof (struct Jump), Compare_Jumps);
}

static int Third_Passing (struct Bin_Tr *const bin_tr, struct Jump *const jumps_arr, const int n_jumps)
{
    MY_ASSERT (bin_tr,    "struct Bin_Tr *const bin_tr",         NULL_PTR, ERROR);
    MY_ASSERT (jumps_arr, "const struct Jumps *const jumps_arr", NULL_PTR, ERROR);

    int max_ip      = bin_tr->max_ip;
    char *proc_buff = bin_tr->input_buff;
    char *x86_buff  = bin_tr->x86_buff;
    
    Sort_Jumps (jumps_arr, n_jumps);    // sorts by "to" field
    
    int jump_i = 0;
    int ip = 0, x86_ip = 0;

    int rsp = 0;

    while (ip < max_ip)
    {
        while (jump_i < n_jumps && ip == jumps_arr[jump_i].to)
        {
            int x86_from = jumps_arr[jump_i].x86_from;

            switch (jumps_arr[jump_i].type)
            {
                case call:
                case jmp:
                    *(int *)(x86_buff + x86_from + 1) = x86_ip - (x86_from + 1 + sizeof (int));
                    break;
                case jae:
                case ja:
                case jbe:
                case jb:
                case je:
                case jne:
                    *(int *)(x86_buff + x86_from + 2) = x86_ip - (x86_from + 2 + sizeof (int));
                    break;

                default:
                    MY_ASSERT (false, "jumps_arr[jump_i].type", UNEXP_VAL, ERROR);
                    break;
            }
            
            jump_i++;
        }
        
        switch (proc_buff[ip])
        {
            case hlt:
                x86_ip += 10;   // look in Commands.md
                ip++;
                break;

            case call:
            case jmp:
            case jae:
            case ja:
            case jbe:
            case jb:
            case jne:
            case je:
            {
                if (proc_buff[ip] == call || proc_buff[ip] == jmp)
                    x86_ip += 5;    // look in Commands.md
                else
                    x86_ip += 11;   // look in Commands.md
                ip += 1 + sizeof (int);

                break;
            }

            case ret:
                x86_ip++;   // look in Commands.md
                ip++;
                break;

            case in:
                ip++;
                x86_ip = (rsp % 16 == 0) ? x86_ip + 19 : x86_ip + 21;   // look in Commands.md
                rsp -= 8;
                break;

            case out:
                ip++;
                x86_ip += 19;   // look in Commands.md
                rsp += 8;
                break;

            case push:
            case pop:
            {
                ip++;
                int checksum = proc_buff[ip] + 10 * proc_buff[ip + 1] + 100 * proc_buff[ip + 2];
                //                  |                     |                        |
                //                if RAM                if reg                   if num

                switch (checksum)
                {                    
                    case EMPTY:
                        ip += 3;
                        x86_ip++;       // look in Commands.md
                        break;
                    
                    case NUM:
                        ip += 3 + sizeof (double);
                        x86_ip += 11;   // look in Commands.md
                        break;

                    case RAM_NUM:
                        ip += 3 + sizeof (int);
                        x86_ip += 9;    // look in Commands.md
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        ip += 3;
                        x86_ip++;       // look in Commands.md
                        break;

                    case RAM_AX:
                    case RAM_BX:
                    case RAM_CX:
                    case RAM_DX:
                        ip += 3;
                        x86_ip += 4;    // look in Commands.md
                        break;

                    case RAM_AX_NUM:
                    case RAM_BX_NUM:
                    case RAM_CX_NUM:
                    case RAM_DX_NUM:
                        x86_ip += 8;    // look in Commands.md
                        ip += 3 + sizeof (int);
                        break;

                    default:
                        MY_ASSERT (false, "int checksum", UNEXP_VAL, ERROR);
                        break;
                }
                break;
            }

            case add:
            case sub:
            case mul:
            case dvd:
                x86_ip += 24;   // look in Commands.md
                ip++;
                break;

            case Sqrt:
                x86_ip += 14;   // look in Commands.md
                ip++;
                break;

            default: MY_ASSERT (false, "proc_buff[ip]", UNDEF_CMD, ERROR);
        }
    }
    
    return NO_ERRORS;
}

static int Translate (struct Bin_Tr *const bin_tr)
{
    MY_ASSERT (bin_tr,             "struct Bin_Tr *const bin_tr", NULL_PTR, ERROR);
    MY_ASSERT (bin_tr->input_buff, "const char *const input",     NULL_PTR, ERROR);

    int n_jumps = 0;
    struct Jump *jumps_arr = First_Passing (bin_tr, &n_jumps);
    MY_ASSERT ((n_jumps == 0 && jumps_arr == NULL) || (n_jumps > 0 && jumps_arr != NULL), "First_Passing ()", FUNC_ERROR, ERROR);

    #if 0
    for (int i = 0; i < n_jumps; i++)
        printf ("TYPE: %X\n"
                "from: %d\n"
                "to: %d\n"
                "x86_from: %d\n\n", 
                jumps_arr[i].type, jumps_arr[i].from, jumps_arr[i].to, jumps_arr[i].x86_from);      
    #endif

    #ifdef DEBUG
    int SP_status = Second_Passing (bin_tr);
    #else
    Second_Passing (bin_tr);
    #endif

    MY_ASSERT (SP_status != ERROR, "Second_Passing ()", FUNC_ERROR, ERROR);

    if (n_jumps > 0)
    {
        #ifdef DEBUG
        int TP_status = Third_Passing (bin_tr, jumps_arr, n_jumps);
        #else
        Third_Passing (bin_tr, jumps_arr, n_jumps);
        #endif

        MY_ASSERT (TP_status != ERROR, "Third_Passing ()", FUNC_ERROR, ERROR);

        free (jumps_arr);
    }

    return NO_ERRORS;
}

static int Extract_Code (struct Bin_Tr *const bin_tr, const char *const input_name)
{
    MY_ASSERT (bin_tr,     "struct Bin_Tr *const bin_tr",  NULL_PTR, ERROR);
    MY_ASSERT (input_name, "const char *const input_name", NULL_PTR, ERROR);
    
    FILE *file_ptr = Open_File (input_name, "rb");
    MY_ASSERT (file_ptr, "Open_File ()", FUNC_ERROR, ERROR);

    long n_symbs = Define_File_Size (file_ptr);
    MY_ASSERT (n_symbs > 0, "Define_File_Size ()", FUNC_ERROR, ERROR);
    bin_tr->max_ip = n_symbs;

    bin_tr->input_buff = Make_Buffer (file_ptr, n_symbs);
    MY_ASSERT (bin_tr->input_buff, "Make_Buffer ()", FUNC_ERROR, ERROR);

    Close_File (file_ptr, input_name);

    return NO_ERRORS;
}

static int JIT (struct Bin_Tr *bin_tr)
{
    #ifdef DEBUG
    int mprotect_res = mprotect (bin_tr->x86_buff, bin_tr->x86_max_ip, PROT_EXEC);
    #else
    mprotect (bin_tr->x86_buff, bin_tr->x86_max_ip, PROT_EXEC);
    #endif

    MY_ASSERT (mprotect_res == 0, "mprotect ()", FUNC_ERROR, ERROR);

    void (* executor)(void) = (void (*)(void))(bin_tr->x86_buff);
    executor();

    return NO_ERRORS;
}

int Binary_Translator (const char *const input_name)
{
    MY_ASSERT (input_name, "const char *const input_name", NULL_PTR, ERROR);

    struct Bin_Tr bin_tr = {};
    
    #ifdef DEBUG
    int EC_status = Extract_Code (&bin_tr, input_name);
    #else
    Extract_Code (&bin_tr, input_name);
    #endif

    MY_ASSERT (EC_status != ERROR, "Extract_Code ()", FUNC_ERROR, ERROR);

    #ifdef DEBUG
    int Tr_status = Translate (&bin_tr);
    #else
    Translate (&bin_tr);
    #endif

    free (bin_tr.input_buff);
    MY_ASSERT (Tr_status != ERROR, "Translate ()", FUNC_ERROR, ERROR);

    #if 0
    FILE *output = Open_File (output_name, "wb");
    fwrite (bin_tr.x86_buff, sizeof (char), bin_tr.x86_max_ip, output);
    Close_File (output, output_name);
    #endif

    JIT (&bin_tr);

    free (bin_tr.x86_buff);

    printf ("Thanks for choosing Ketchupp_JIT!\n");

    return NO_ERRORS;
}
