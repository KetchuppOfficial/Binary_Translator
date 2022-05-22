#ifndef BINARY_TRANSLATOR_INSLUDED
#define BINARY_TRANSLATOR_INSLUDED

#include "My_Lib.h"
#include <inttypes.h>
#include <string.h>
#include <sys/mman.h>   // for mprotect ()

#ifndef DEBUG
#undef MY_ASSERT
#define MY_ASSERT(condition, var, err_num, error) ;
#endif

#define DEFCMD_(num, name, n_args) name = num,

#define N_INSTRUCTIONS 28

enum Instructions
{
    #include "../include/Commands_List.h"
};

#undef DEFCMD_

enum ISA
{
    hlt,
    call,
    jmp,
    jae,
    ja,
    jbe,
    jb,
    je,
    jne,
    ret,
    in,
    out,
    push_num,
    push_ram_num,
    push_reg,
    push_ram_reg,
    push_ram_reg_num,
    pop,
    pop_ram_num,
    pop_reg,
    pop_ram_reg,
    pop_ram_reg_num,
    add,
    sub,
    mul,
    dvd,
    Sqrt
};

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

int Binary_Translator (const char *const input);

#endif
