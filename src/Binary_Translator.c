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
};

struct Bin_Tr
{
    char *input_buff;
    int ip;
    int max_ip;
    char *x86_buff;
    int x86_ip;
    int x86_max_ip;
};

static struct Jump *First_Passing (struct Bin_Tr *bin_tr, int *const n_jumps)
{
    MY_ASSERT (bin_tr,             "struct Bin_Tr bin_tr",    NULL_PTR, ERROR)
    MY_ASSERT (bin_tr->input_buff, "const char *const input", NULL_PTR, ERROR);
    MY_ASSERT (n_jumps,            "int *const n_jumps",      NULL_PTR, NULL);

    const int max_ip = bin_tr->max_ip;
    const char *code_arr = bin_tr->input_buff;

    struct Jump *jumps_arr = (struct Jump *)calloc (max_ip, sizeof (struct Jump));
    MY_ASSERT (jumps_arr, "struct Jump *jumps_arr", NE_MEM, NULL);
    int jump_i = 0;

    for (int ip = 0; ip < max_ip; )
    {
        switch (code_arr[ip])
        {
            case hlt:
            case ret:
            case in:
            case out:
            case add:
            case sub:
            case mul:
            case dvd:
                ip++;
                break;
            
            case jae:
            case ja:
            case jbe:
            case jb:
            case jne:
            case je:
            case jmp:
            case call:
                jumps_arr[jump_i].to = *(int *)(code_arr + ip + 1);
                jumps_arr[jump_i].from = ip;
                jump_i++;
                ip += 1 + sizeof (int);
                break;

            case push:
            case pop:
                ip++;
                int checksum = code_arr[ip] + 10 * code_arr[ip + 1] + 100 * code_arr[ip + 2];

                switch (checksum)
                {                    
                    case NUM:
                        ip += (3 + sizeof (double));
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                    case EMPTY:
                    case RAM_AX:
                    case RAM_BX:
                    case RAM_CX:
                    case RAM_DX:
                        ip += 3;
                        break;

                    case RAM_NUM:
                    case RAM_AX_NUM:
                    case RAM_BX_NUM:
                    case RAM_CX_NUM:
                    case RAM_DX_NUM:
                        ip += (3 + sizeof (int));
                        break;

                    default:
                        MY_ASSERT (false, "int checksum", UNEXP_VAL, NULL);
                        break;
                }
                break;

            default: MY_ASSERT (false, "code_arr[ip]", UNDEF_CMD, ERROR);
        }
    }

    *n_jumps = jump_i;

    return jumps_arr;
}

//=====================================================================================//

//=====================================================================================//
//                                   SECOND PASSING                                    //
//=====================================================================================//

static inline void Translate_Hlt (char *const x86_buffer, int *const ip)
{
    char opcode[] = {
                        0xB8, 0x3C, 0x00, 0x00, 0x00,   // mov rax, 0x3C
                        0x48, 0x31, 0xFF,               // xor rdi, rdi
                        0x0F, 0x05                      // syscall
                    };

    memcpy (x86_buffer, opcode, sizeof opcode);
    *ip += sizeof opcode;
}

static inline void Translate_Ret (char *const x86_buffer, int *const ip)
{
    x86_buffer[*ip] = 0x3C;
    (*ip)++;
}

static inline void Translate_Push_Num (char *const x86_buffer, int *const ip, const double num)
{    
    char opcode[] = {
                        0x48, 0xBF,                                         // mov rdi, 0
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // (0 is changed below)

                        0x57                                                // push rdi
                    };

    *(double *)(opcode + 2) = num;

    memcpy (x86_buffer + *ip, opcode, sizeof opcode);
    *ip += sizeof opcode;
}

static inline void Translate_Push_RAM_Num (char *const x86_buffer, int *const ip, const int num)
{
    char opcode[] = {
                        0x48, 0x8B, 0x3C, 0x25,     // mov rdi, qword [0]
                        0x00, 0x00, 0x00, 0x00,     // (0 is changed below)

                        0x57                        // push rdi
                    };

    *(int *)(opcode + 4) = num;

    memcpy (x86_buffer + *ip, opcode, sizeof opcode);
    *ip += sizeof opcode;
}

enum Registers
{
    ax = 1,
    bx = 2,
    cx = 3,
    dx = 4
};

static inline int Translate_Push_Reg (char *x86_buffer, int *ip, enum Registers reg)
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
            MY_ASSERT (false, "enum Registers reg", UNEXP_VAL, ERROR);
            break;
    }

    x86_buffer[(*ip)++] = opcode;

    return NO_ERRORS;
}

static inline int Translate_Push_RAM_Reg (char *x86_buffer, int *ip, enum Registers reg)
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
            opcode[2] = 0x38;      // push rax
            break;
        case bx:
            opcode[2] = 0x3B;      // push rbx
            break;
        case cx:
            opcode[2] = 0x39;      // push rcx
            break;
        case dx:
            opcode[2] = 0x3A;      // push rdx
            break;

        default:
            MY_ASSERT (false, "enum Registers reg", UNEXP_VAL, ERROR);
            break;   
    }

    memcpy (x86_buffer + *ip, opcode, sizeof opcode);
    *ip += sizeof opcode;

    return NO_ERRORS;
}

static inline int Translate_Push_RAM_Reg_Num (char *const x86_buffer, int *const ip, enum Registers reg, const int num)
{
    if (-128 <= num && num <= 127)
    {
        char opcode[] = {
                            0x48, 0x8B,     // mov rdi, qword [r?x + num]
                            0x00,           // register
                            0x00,           // num

                            0x57            // push rdi                    
                        };

        switch (reg)
        {
            case ax:
                opcode[2] = 0x78;      // rax
                break;
            case bx:
                opcode[2] = 0x7B;      // rbx
                break;
            case cx:
                opcode[2] = 0x79;      // rcx
                break;
            case dx:
                opcode[2] = 0x7A;      // rdx
                break;

            default:
                MY_ASSERT (false, "enum Registers reg", UNEXP_VAL, ERROR);
                break;
        }

        opcode[3] = (char)num;

        memcpy (x86_buffer + *ip, opcode, sizeof opcode);
        *ip += sizeof opcode;
    }
    else
    {
        char opcode[] = {
                            0x48, 0x8B,                 // mov rdi, qword [r?x + num]
                            0x00,                       // register
                            0x00, 0x00, 0x00, 0x00,     // num

                            0x57                        // push rdi                    
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
                MY_ASSERT (false, "enum Registers reg", UNEXP_VAL, ERROR);  
                break;
        }

        *(int *)(opcode + 3) = num;

        memcpy (x86_buffer + *ip, opcode, sizeof opcode);
        *ip += sizeof opcode;
    }

    return NO_ERRORS;
}

static int Translate_Math (char *x86_buffer, int *ip, enum Instructions instruction)
{
    const char first_part[] = {
                                0xF2, 0x0F, 0x10, 0x4C, 0x24, 0x08,     // movsd   xmm1, qword [rsp + 8]
                                0xF2, 0x0F, 0x10, 0x14, 0x24,           // movsd   xmm2, qword [rsp]
                                0x48, 0x83, 0xC4, 0x08                  // add     rsp, 8
                              };

    memcpy (x86_buffer + *ip, first_part, sizeof first_part);
    *ip += sizeof first_part;
    
    char math_instruction[] = {0xF2, 0x0F, 0x00, 0xCA};
    //                                       |
    //                                       |
    //                                       |
    //           this byte will be changed --+

    switch (instruction)
    {
        case add:
            math_instruction[2] = 0x58;     // addsd   xmm1, xmm2
            break;
        case sub:
            math_instruction[2] = 0xCA;     // subsd   xmm1, xmm2
            break;
        case mul:
            math_instruction[2] = 0x59;     // mulsd   xmm1, xmm2
            break;
        case dvd:
            math_instruction[2] = 0x5E;     // divsd   xmm1, xmm2
            break;

        default:
            MY_ASSERT (false, "enum Instructions instruction", UNEXP_VAL, ERROR);
    }

    memcpy (x86_buffer + *ip, math_instruction, sizeof math_instruction);
    *ip += sizeof math_instruction;

    const char last_part[] = {
                                0xF2, 0x0F, 0x11, 0x0C, 0x24    // movsd   qword [rsp], xmm1
                             };

    memcpy (x86_buffer + *ip, last_part, sizeof last_part);
    *ip += sizeof last_part;

    return NO_ERRORS;
}           

char *Second_Passing (const char *code_arr, const int max_ip, const struct Jump *jumps_arr, const int n_jumps)
{
    MY_ASSERT (code_arr,  "const char *code_arr",         NULL_PTR, ERROR);
    MY_ASSERT (jumps_arr, "const struct Jump *jumps_arr", NULL_PTR, ERROR);

    char *x86_buffer = (char *)calloc (max_ip, sizeof (char));
    MY_ASSERT (x86_buffer, "char *x86_buffer", NE_MEM, NULL);
    
    int x86_ip = 0;
    for (int ip = 0; ip < max_ip; )
    {
        #if 0
        for (int i = 0; i <= n_jumps; i++)
        {
            if (jumps_arr[i] == ip && jumps_arr[i] != 0)
                fprintf (file_ptr, "%d:\n", jumps_arr[i]);
        }
        #endif

        switch (code_arr[ip])
        {
            case hlt:
                Translate_Hlt (x86_buffer, &x86_ip);
                break;
            
            case call:
                break;
            
            case jae:
            case ja:
            case jbe:
            case jb:
            case jne:
            case je:
                break;

            case ret:
                Translate_Ret (x86_buffer, &x86_ip);
                break;
            
            case in:
            case out:
                break;

            case push:
                ip++;
                int checksum = code_arr[ip] + 10 * code_arr[ip + 1] + 100 * code_arr[ip + 2];

                switch (checksum)
                {
                    case NUM:
                        Translate_Push_Num (x86_buffer, &x86_ip, *(double *)(code_arr + ip + 3));
                        ip += (3 + sizeof (double));
                        break;

                    case RAM_NUM:
                        Translate_Push_RAM_Num (x86_buffer, &x86_ip, *(int *)(code_arr + ip + 3));
                        ip += (3 + sizeof (int));
                        break;

                    case AX:
                    case BX:
                    case CX:
                    case DX:
                        Translate_Push_Reg (x86_buffer, &x86_ip, (int)code_arr[ip + 1]);
                        ip += 3;
                        break;

                    case RAM_AX:
                    case RAM_BX:
                    case RAM_CX:
                    case RAM_DX:
                        Translate_Push_RAM_Reg (x86_buffer, &x86_ip, (int)code_arr[ip + 1]);
                        ip += 3;
                        break;

                    case RAM_AX_NUM:
                    case RAM_BX_NUM:
                    case RAM_CX_NUM:
                    case RAM_DX_NUM:
                        Translate_Push_RAM_Reg_Num (x86_buffer, &x86_ip, (int)code_arr[ip + 1], *(int *)(code_arr + ip + 3));
                        ip += (3 + sizeof (int));
                        break;

                    default:
                        MY_ASSERT (false, "int checksum", UNEXP_VAL, NULL);
                        break;
                }
                break;

            case pop:
                break;

            case add:
            case sub:
            case mul:
            case dvd:
                Translate_Math (x86_buffer, &ip, code_arr[ip]);
                ip++;
                break;

            default: MY_ASSERT (false, "code_arr[ip]", UNDEF_CMD, ERROR);
        }
    }

    return x86_buffer;
}
#undef DEFCMD_

static char *Translate (struct Bin_Tr *bin_tr)
{
    MY_ASSERT (bin_tr,             "struct Bin_Tr bin_tr",    NULL_PTR, ERROR)
    MY_ASSERT (bin_tr->input_buff, "const char *const input", NULL_PTR, ERROR);

    int n_jumps = 0;
    struct Jump *jumps_arr = First_Passing (bin_tr, &n_jumps);
    MY_ASSERT (jumps_arr, "First_Passing ()", FUNC_ERROR, NULL);

    char *x86_buffer = Second_Passing (bin_tr->input_buff, bin_tr->max_ip, jumps_arr, n_jumps);
    MY_ASSERT (x86_buffer, "Second_Passing ()", FUNC_ERROR, NULL);

    free (jumps_arr);

    return x86_buffer;
}

static int Extract_Code (struct Bin_Tr *bin_tr, const char *const input_name)
{
    MY_ASSERT (bin_tr,             "struct Bin_Tr bin_tr",    NULL_PTR, ERROR)
    MY_ASSERT (bin_tr->input_buff, "const char *const input", NULL_PTR, ERROR);
    
    FILE *file_ptr = Open_File (input_name, "rb");

    long n_symbs = Define_File_Size (file_ptr);
    MY_ASSERT (n_symbs > 0, "Define_File_Size ()", FUNC_ERROR, NULL);
    bin_tr->max_ip = n_symbs;

    bin_tr->input_buff = Make_Buffer (file_ptr, n_symbs);

    Close_File (file_ptr, input_name);

    return NO_ERRORS;
}

int Binary_Translator (const char *const input_name, const char *const output_name)
{
    MY_ASSERT (input_name,  "const char *const input_name",  NULL_PTR, ERROR);
    MY_ASSERT (output_name, "const char *const output_name", NULL_PTR, ERROR);

    struct Bin_Tr bin_tr = {};
    
    Extract_Code (&bin_tr, input_name);

    Translate (&bin_tr);
    free (bin_tr.input_buff);

    #if 0
    Generate_ELF (output_buff, output);
    #endif

    free (bin_tr.x86_buff);

    return NO_ERRORS;
}
