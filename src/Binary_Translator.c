#include "../include/Binary_Translator.h"

//=====================================================================================//
//                                    FIRST PASSING                                    //
//=====================================================================================//

struct Instruction
{
    int name;
    int proc_num;
    int proc_sz;
    int x86_sz;
};

static const struct Instruction ISA_Consts[N_INSTRUCTIONS] =
{
    {hlt,               HLT,  1,  1},
    {call,             CALL,  5,  5},
    {jmp,               JMP,  5,  5},
    {jae,               JAE,  5, 11},
    {ja,                 JA,  5, 11},
    {jbe,               JBE,  5, 11},
    {jb,                 JB,  5, 11},
    {je,                 JE,  5, 11},
    {jne,               JNE,  5, 11},
    {ret,               RET,  1,  1},
    {in,                 IN,  1, 19},
    {out,               OUT,  1, 19},
    {push_num,         PUSH, 12, 11},
    {push_ram_num,     PUSH,  8,  9},
    {push_reg,         PUSH,  4,  1},
    {push_ram_reg,     PUSH,  4,  4},
    {push_ram_reg_num, PUSH,  8,  8},
    {pop,               POP,  4,  1},
    {pop_ram_num,       POP,  8,  9},
    {pop_reg,           POP,  4,  1},
    {pop_ram_reg,       POP,  4,  4},
    {pop_ram_reg_num,   POP,  8,  8},
    {add,               ADD,  1, 24},
    {sub,               SUB,  1, 24},
    {mul,               MUL,  1, 24},
    {dvd,               DVD,  1, 24},
    {Sqrt,             SQRT,  1, 14}
};

struct Bin_Tr
{
    char *input_buff;
    char *x86_buff;
    long  max_ip;
    long  x86_max_ip;
};

static int Handle_Push_Pop_First (const char *const proc_buff, int *const ip, int *const x86_ip)
{    
    MY_ASSERT (proc_buff, "const char *const proc_buff", NULL_PTR, ERROR);
    MY_ASSERT (ip,        "int *const ip",               NULL_PTR, ERROR);
    MY_ASSERT (x86_ip,    "int *const x86_ip",           NULL_PTR, ERROR);
    
    const int checksum = proc_buff[*ip + 1] + 10 * proc_buff[*ip + 2] + 100 * proc_buff[*ip + 3];
    //                         |                        |                         |
    //                       if RAM                   if reg                    if num

    switch (checksum)
    {                    
        case EMPTY:
            *ip     += ISA_Consts[pop].proc_sz;
            *x86_ip += ISA_Consts[pop].x86_sz;
            break;
        
        case NUM:
            *ip     += ISA_Consts[push_num].proc_sz;
            *x86_ip += ISA_Consts[push_num].x86_sz;
            break;

        case RAM_NUM:
            *ip     += ISA_Consts[push_ram_num].proc_sz;
            *x86_ip += ISA_Consts[push_ram_num].x86_sz;
            break;

        case AX:
        case BX:
        case CX:
        case DX:
            *ip     += ISA_Consts[push_reg].proc_sz;
            *x86_ip += ISA_Consts[push_reg].x86_sz;
            break;

        case RAM_AX:
        case RAM_BX:
        case RAM_CX:
        case RAM_DX:
            *ip     += ISA_Consts[push_ram_reg].proc_sz;
            *x86_ip += ISA_Consts[push_ram_reg].x86_sz;
            break;

        case RAM_AX_NUM:
        case RAM_BX_NUM:
        case RAM_CX_NUM:
        case RAM_DX_NUM:
            *ip     += ISA_Consts[push_ram_reg_num].proc_sz;
            *x86_ip += ISA_Consts[push_ram_reg_num].x86_sz;
            break;

        default:
            MY_ASSERT (false, "int checksum", UNEXP_VAL, ERROR);
            break;
    }

    return NO_ERRORS;
}

struct Jump
{
    int from;
    int to;
    int x86_from;
    enum Instructions type;
};

static int Fill_Jumps_Arr (const char *const proc_buff, const int ip, const int x86_ip, struct Jump *const jumps_arr, const int jump_i)
{
    MY_ASSERT (proc_buff, "const char *const proc_buff", NULL_PTR, ERROR);
    MY_ASSERT (jumps_arr, "char *const jumps_arr",       NULL_PTR, ERROR);
    
    char instr_code = proc_buff[ip];
    int jump_to = *(int *)(proc_buff + ip + 1);

    jumps_arr[jump_i].from = ip;
    jumps_arr[jump_i].to   = jump_to;

    if (instr_code == CALL || instr_code == JMP)
        jumps_arr[jump_i].x86_from = x86_ip;
    else
        jumps_arr[jump_i].x86_from = x86_ip + 5;    
        // the first byte of x86-64 jump is 5 bytes further than the beginning of instruction

    jumps_arr[jump_i].type = (int)proc_buff[ip];

    return NO_ERRORS;
}

static struct Jump *First_Passing (struct Bin_Tr *bin_tr, int *const n_jumps)
{
    MY_ASSERT (bin_tr,             "struct Bin_Tr bin_tr",    NULL_PTR, NULL);
    MY_ASSERT (bin_tr->input_buff, "const char *const input", NULL_PTR, NULL);
    MY_ASSERT (n_jumps,            "int *const n_jumps",      NULL_PTR, NULL);

    const int   max_ip    = bin_tr->max_ip;
    const char *proc_buff = bin_tr->input_buff;

    struct Jump *jumps_arr = (struct Jump *)calloc (max_ip, sizeof (struct Jump));
    MY_ASSERT (jumps_arr, "struct Jump *jumps_arr", NE_MEM, NULL);
    int jump_i = 0;

    int x86_ip = 0;

    for (int ip = 0; ip < max_ip; )
    {
        switch (proc_buff[ip])
        {
            case HLT:
                ip     += ISA_Consts[hlt].proc_sz;
                x86_ip += ISA_Consts[hlt].x86_sz;
                break;

            case CALL:
            case JMP:
            {
                Fill_Jumps_Arr (proc_buff, ip, x86_ip, jumps_arr, jump_i);
                jump_i++;

                ip     += ISA_Consts[call].proc_sz;
                x86_ip += ISA_Consts[call].x86_sz;

                break;
            }

            case JAE:
            case JA:
            case JBE:
            case JB:
            case JNE:
            case JE:
            {
                Fill_Jumps_Arr (proc_buff, ip, x86_ip, jumps_arr, jump_i);
                jump_i++;

                ip     += ISA_Consts[jae].proc_sz;
                x86_ip += ISA_Consts[jae].x86_sz;

                break;
            }

            case RET:
                ip     += ISA_Consts[ret].proc_sz;
                x86_ip += ISA_Consts[ret].x86_sz;
                break;

            case IN:
                ip     += ISA_Consts[in].proc_sz;
                x86_ip += ISA_Consts[in].x86_sz;

                break;

            case OUT:
                ip     += ISA_Consts[out].proc_sz;
                x86_ip += ISA_Consts[out].x86_sz;
                break;

            case PUSH:
            case POP:
                Handle_Push_Pop_First (proc_buff, &ip, &x86_ip);
                break;
                
            case ADD:
            case SUB:
            case MUL:
            case DVD:
                ip     += ISA_Consts[add].proc_sz;
                x86_ip += ISA_Consts[add].x86_sz;
                break;

            case SQRT:
                ip     += ISA_Consts[Sqrt].proc_sz;
                x86_ip += ISA_Consts[Sqrt].x86_sz;
                break;

            default: 
                MY_ASSERT (false, "proc_buff[ip]", UNEXP_VAL, NULL);
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

static inline void Translate_Ret (char *const x86_buffer, int *const x86_ip)
{
    x86_buffer[(*x86_ip)++] = 0xC3;     // ret
}

static inline void Translate_Call (char *const x86_buffer, int *const x86_ip)
{
    x86_buffer[(*x86_ip)++] = 0xE8; // call
    
    (*x86_ip) += 4; // making free space of 4 bytes for call argument (relative offset)
}

static inline void Translate_Jmp (char *const x86_buffer, int *const x86_ip)
{
    x86_buffer[(*x86_ip)++] = 0xE9; // jmp
    
    (*x86_ip) += 4; // making free space of 4 bytes for jump argument (relative offset)
}

static inline int Translate_Conditional_Jmp (char *const x86_buffer, int *const x86_ip, const enum Instructions jcc)
{
    char opcode[] = {
                        0x5E,                   // pop rsi           
                        0x5F,                   // pop rdi
                        0x48, 0x39, 0xF7,       // cmp rdi, rsi
                        0x0F, 0x00,             // jcc (can be jae, ja, jbe, jb, jne or je)
                    };

    switch (jcc)
    {
        case JAE:
            opcode[6] = 0x8D;   // jge
            break;
        case JA:
            opcode[6] = 0x8F;   // jg
            break;
        case JBE:
            opcode[6] = 0x8E;   // jle
            break;
        case JB:
            opcode[6] = 0x8C;   // jl
            break;
        case JE:
            opcode[6] = 0x84;   // je
            break;
        case JNE:
            opcode[6] = 0x85;   // jne
            break;

        default:
            MY_ASSERT (false, "const enum Instructions jcc", UNEXP_VAL, ERROR);
            break;
    }

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);

    (*x86_ip) += 4; // making free space of 4 bytes for jump argument (relative offset)
    
    return NO_ERRORS;
}

static inline void In (double *num_ptr)
{
    printf ("Write a number: ");
    scanf ("%lf", num_ptr);
}

static inline void Translate_In_Align_16 (char *const x86_buffer, int *const x86_ip)
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
    // 11 - offset of call argument relatively to the beginning of opcode

    Put_In_x86_Buffer (x86_buffer, x86_ip, opcode, sizeof opcode);
}

static inline void Out (const double number)
{
    printf ("%g\n", number);
}

static inline void Translate_Out_Align_8 (char *const x86_buffer, int *const x86_ip)
{
    char opcode[] = {
                        0xF2, 0x0F, 0x10, 0x04, 0x24,   // movsd xmm0, qword [rsp]

                        0x50,        // push rax
                        0x53,        // push rbx
                        0x51,        // push rcx
                        0x52,        // push rdx

                        0xE8, 0x00, 0x00, 0x00, 0x00,   // call Out

                        0x5A,       // pop rdx
                        0x59,       // pop rcx
                        0x5B,       // pop rbx
                        0x58,       // pop rax

                        0x5F        // pop rdi
                    };

    *(uint32_t *)(opcode + 10) = (uint64_t )Out - (uint64_t)(x86_buffer + *x86_ip + 10 + sizeof (int));
    // 10 - offset of call argument relatively to the beginning of opcode

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
    
    char math_instruction[] = {0xF2, 0x0F, 0x00, 0xCA}; // "instruction" xmm1, xmm2
    //                                       |
    //           this byte will be changed --+

    switch (instruction)
    {
        case ADD:
            math_instruction[2] = 0x58;     // addsd   xmm1, xmm2
            break;
        case SUB:
            math_instruction[2] = 0x5C;     // subsd   xmm1, xmm2
            break;
        case MUL:
            math_instruction[2] = 0x59;     // mulsd   xmm1, xmm2
            break;
        case DVD:
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

static int Handle_Push_Pop_Second (const char *const proc_buff, int *const ip, char *const x86_buffer, int *const x86_ip)
{
    MY_ASSERT (proc_buff,  "const char *const proc_buff", NULL_PTR, ERROR);
    MY_ASSERT (ip,         "int *const ip",               NULL_PTR, ERROR);
    MY_ASSERT (x86_buffer, "char *const x86_buffer",      NULL_PTR, ERROR);
    MY_ASSERT (x86_ip,     "int *const x86_ip",           NULL_PTR, ERROR);
    
    const char instr_name = proc_buff[*ip];

    const int checksum = proc_buff[*ip + 1] + 10 * proc_buff[*ip + 2] + 100 * proc_buff[*ip + 3];
    //                         |                        |                         |
    //                       if RAM                   if reg                    if num
    
    switch (checksum)
    {
        case EMPTY:
            Translate_Pop (x86_buffer, x86_ip);
            *ip += ISA_Consts[pop].proc_sz;
            break;
        
        case NUM:
        {
            const double num = *(double *)(proc_buff + *ip + 1 + 3);

            Translate_Push_Num (x86_buffer, x86_ip, num);
            *ip += ISA_Consts[push_num].proc_sz;

            break;
        }

        case RAM_NUM:
        {
            const int num = *(int *)(proc_buff + *ip + 1 + 3);
            
            if (instr_name == PUSH)
                Translate_Push_RAM_Num (x86_buffer, x86_ip, num);
            else
                Translate_Pop_RAM_Num  (x86_buffer, x86_ip, num);

            *ip += ISA_Consts[push_ram_num].proc_sz;

            break;
        }

        case AX:
        case BX:
        case CX:
        case DX:
        {
            const char reg = proc_buff[*ip + 2];

            if (instr_name == PUSH)
                Translate_Push_Reg (x86_buffer, x86_ip, (int)reg);
            else
                Translate_Pop_Reg  (x86_buffer, x86_ip, (int)reg);
            
            *ip += ISA_Consts[push_reg].proc_sz;

            break;
        }

        case RAM_AX:
        case RAM_BX:
        case RAM_CX:
        case RAM_DX:
        {
            const char reg = proc_buff[*ip + 2];

            if (instr_name == PUSH)
                Translate_Push_RAM_Reg (x86_buffer, x86_ip, (int)reg);
            else
                Translate_Pop_RAM_Reg  (x86_buffer, x86_ip, (int)reg);
            
            *ip += ISA_Consts[push_ram_reg].proc_sz;
            
            break;
        }

        case RAM_AX_NUM:
        case RAM_BX_NUM:
        case RAM_CX_NUM:
        case RAM_DX_NUM:
        {
            const char reg = proc_buff[*ip + 2];
            const int num  = *(int *)(proc_buff + *ip + 1 + 3);

            if (instr_name == PUSH)
                Translate_Push_RAM_Reg_Num (x86_buffer, x86_ip, (int)reg, num);
            else
                Translate_Pop_RAM_Reg_Num  (x86_buffer, x86_ip, (int)reg, num);

            *ip += ISA_Consts[push_ram_reg_num].proc_sz;

            break;
        }

        default:
            MY_ASSERT (false, "int checksum", UNEXP_VAL, ERROR);
            break;
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

static int Fill_Jumps_Args (char *const x86_buff, const int x86_ip, const int ip, struct Jump *const jumps_arr, const int n_jumps, int *jump_i)
{
    MY_ASSERT (x86_buff,  "char *const x86_buff",                NULL_PTR, ERROR);
    MY_ASSERT (jumps_arr, "const struct Jumps *const jumps_arr", NULL_PTR, ERROR);
    
    while (*jump_i < n_jumps && ip == jumps_arr[*jump_i].to)
    {
        int x86_from = jumps_arr[*jump_i].x86_from;

        switch (jumps_arr[*jump_i].type)
        {
            case CALL:
            case JMP:
                *(int *)(x86_buff + x86_from + 1) = x86_ip - (x86_from + 1 + sizeof (int));

                break;

            case JAE:
            case JA:
            case JBE:
            case JB:
            case JE:
            case JNE:
                *(int *)(x86_buff + x86_from + 2) = x86_ip - (x86_from + 2 + sizeof (int));

                break;

            default:
                MY_ASSERT (false, "jumps_arr[jump_i].type", UNEXP_VAL, ERROR);
                break;
        }
        
        (*jump_i)++;
    }

    return NO_ERRORS;
}

static int Second_Passing (struct Bin_Tr *const bin_tr, struct Jump *const jumps_arr, const int n_jumps)
{
    MY_ASSERT (bin_tr,    "struct Bin_Tr *const bin_tr",         NULL_PTR, ERROR);
    
    bin_tr->x86_buff = (char *)aligned_calloc (bin_tr->x86_max_ip, sizeof (char *));
    MY_ASSERT (bin_tr->x86_buff, "bin_tr->x86_buffer", NE_MEM, ERROR);

    const char *proc_buff  = bin_tr->input_buff;
    char       *x86_buffer = bin_tr->x86_buff;
    const int  max_ip      = bin_tr->max_ip;

    if (n_jumps > 0)
        Sort_Jumps (jumps_arr, n_jumps);

    for (int ip = 0, x86_ip = 0, jump_i = 0; ip < max_ip; )
    {
        if (n_jumps > 0)
            Fill_Jumps_Args (x86_buffer, x86_ip, ip, jumps_arr, n_jumps, &jump_i);

        switch (proc_buff[ip])
        {
            case HLT:
                Translate_Ret (x86_buffer, &x86_ip);

                ip  += ISA_Consts[hlt].proc_sz;
                break;
            
            case CALL:
                Translate_Call (x86_buffer, &x86_ip);

                ip  += ISA_Consts[call].proc_sz;
                break;

            case JMP:
                Translate_Jmp (x86_buffer, &x86_ip);

                ip  += ISA_Consts[jmp].proc_sz;
                break;
            
            case JAE:
            case JA:
            case JBE:
            case JB:
            case JNE:
            case JE:
            {
                int jcc = (int)proc_buff[ip];
                
                Translate_Conditional_Jmp (x86_buffer, &x86_ip, jcc);

                ip  += ISA_Consts[jae].proc_sz;
                break;
            }

            case RET:
                Translate_Ret (x86_buffer, &x86_ip);

                ip  += ISA_Consts[ret].proc_sz;
                break;
            
            case IN:
                Translate_In_Align_16 (x86_buffer, &x86_ip);
                ip += ISA_Consts[in].proc_sz;

                break;

            case OUT:
                Translate_Out_Align_8 (x86_buffer, &x86_ip);
                ip += ISA_Consts[out].proc_sz;

                break;

            case PUSH:
            case POP:
                Handle_Push_Pop_Second (proc_buff, &ip, x86_buffer, &x86_ip);
                break;    

            case ADD:
            case SUB:
            case MUL:
            case DVD:
                Translate_Arithmetics (x86_buffer, &x86_ip, proc_buff[ip]);

                ip  += ISA_Consts[add].proc_sz;
                break;

            case SQRT:
                Translate_Sqrt (x86_buffer, &x86_ip);

                ip  += ISA_Consts[Sqrt].proc_sz;
                break;

            default: MY_ASSERT (false, "proc_buff[ip]", UNEXP_VAL, ERROR);
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

    #ifdef DEBUG
    int SP_status = Second_Passing (bin_tr, jumps_arr, n_jumps);
    #else
    Second_Passing (bin_tr, jumps_arr, n_jumps);
    #endif

    MY_ASSERT (SP_status != ERROR, "Second_Passing ()", FUNC_ERROR, ERROR);

    if (n_jumps > 0)
        free (jumps_arr);

    return NO_ERRORS;
}

#ifdef STRESS_TEST
const long long n_tests = 100000000;
#endif

static int JIT (struct Bin_Tr *bin_tr)
{
    #ifdef DEBUG
    int mprotect_res = mprotect (bin_tr->x86_buff, bin_tr->x86_max_ip, PROT_EXEC);
    #else
    mprotect (bin_tr->x86_buff, bin_tr->x86_max_ip, PROT_EXEC);
    #endif

    MY_ASSERT (mprotect_res == 0, "mprotect ()", FUNC_ERROR, ERROR);

    void (* executor)(void) = (void (*)(void))(bin_tr->x86_buff);

    #ifdef STRESS_TEST
    for (long long i = 0; i < n_tests; i++)
        executor ();
    #else
    executor ();
    #endif

    return NO_ERRORS;
}

int Binary_Translator (const char *const input_name)
{
    MY_ASSERT (input_name, "const char *const input_name", NULL_PTR, ERROR);

    struct Bin_Tr bin_tr = {};
    
    bin_tr.input_buff = Make_File_Buffer (input_name, &bin_tr.max_ip);
    MY_ASSERT (bin_tr.input_buff, "Make_File_Buffer ()", FUNC_ERROR, ERROR);

    #ifdef DEBUG
    int Tr_status = Translate (&bin_tr);
    #else
    Translate (&bin_tr);
    #endif

    free (bin_tr.input_buff);
    MY_ASSERT (Tr_status != ERROR, "Translate ()", FUNC_ERROR, ERROR);

    #if 0
    FILE *output = Open_File ("debug.bin", "wb");
    fwrite (bin_tr.x86_buff, sizeof (char), bin_tr.x86_max_ip, output);
    Close_File (output, "debug.bin");
    #endif

    JIT (&bin_tr);

    free (bin_tr.x86_buff);

    printf ("Thanks for choosing Ketchupp_JIT!\n");

    return NO_ERRORS;
}
