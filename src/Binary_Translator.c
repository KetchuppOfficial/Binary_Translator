#include "../include/Binary_Translator.h"

static char *Extract_Code (const char *const input, long *const max_ip)
{
    MY_ASSERT (input,  "const char *const input",  NULL_PTR, NULL);
    MY_ASSERT (max_ip, "int *max_ip",              NULL_PTR, NULL);
    
    FILE *file_ptr = Open_File (input, "rb");

    long n_symbs = Define_File_Size (file_ptr);
    MY_ASSERT (n_symbs > 0, "Define_File_Size ()", FUNC_ERROR, NULL);

    char *code = Make_Buffer (file_ptr, n_symbs);

    Close_File (file_ptr, input);

    *max_ip = n_symbs;

    return code;
}

int Binary_Translator (const char *const input, const char *const output)
{
    MY_ASSERT (input,  "const char *const input",  NULL_PTR, ERROR);
    MY_ASSERT (output, "const char *const output", NULL_PTR, ERROR);
    
    long max_ip = 0L;
    char *input_buff = Extract_Code (input, &max_ip);

    free (input_buff);

    return NO_ERRORS;
}

//***************************************************************
#define DEFCMD_(num, name, n_args, code)    \
case num:                                   \
    return n_args;

static int Check_N_Args (const int cmd_num)
{           
    switch (cmd_num)
    {
        #include "../include/Commands_List.h"

        default:
            MY_ASSERT (false, "const int cmd_num", UNDEF_CMD, ERROR);
            break;
    }

    return 0;
}

#undef DEFCMD_
//***************************************************************

//***************************************************************
#define DEFCMD_(num, name, n_args, code)    \
case num:                                   \
    if (strcmp (#name, "push") == 0 ||      \
        strcmp (#name, "pop")  == 0)        \
        return 1;                           \
    else                                    \
        return 0;


int Check_If_Push_Pop (const int cmd_num)
{
    switch (cmd_num)
    {
        #include "../include/Commands_List.h"

        default:
            MY_ASSERT (false, "const int cmd_num", UNDEF_CMD, ERROR);
            break;
    }

    return 0;
}

#undef DEFCMD_
//***************************************************************

//***************************************************************
#define DEFCMD_(num, name, n_args, code)    \
case num:                                   \
    if (strcmp ("jae",  #name)  == 0 ||     \
        strcmp ("jbe",  #name)  == 0 ||     \
        strcmp ("ja",   #name)  == 0 ||     \
        strcmp ("jb",   #name)  == 0 ||     \
        strcmp ("ja",   #name)  == 0 ||     \
        strcmp ("je",   #name)  == 0 ||     \
        strcmp ("call", #name)  == 0)       \
        return 1;                           \
    else                                    \
        return 0;

static int Check_If_Jump (const int cmd_num)
{
    switch (cmd_num)
    {
        #include "../include/Commands_List.h"

        default:
            MY_ASSERT (false, "const int cmd_num", UNDEF_CMD, ERROR);
            break;
    }

    return 0;
}

#undef DEFCMD_
//***************************************************************

#define DEFCMD_(num, name, n_args, code)                                                \
case num:                                                                               \
    if (Check_N_Args (num) == 0)                                                        \
        ip++;                                                                           \
    else if (Check_If_Push_Pop (num) == 1)                                              \
    {                                                                                   \
        ip++;                                                                           \
        int checksum = code_arr[ip] + 10 * code_arr[ip + 1] + 100 * code_arr[ip + 2];   \
                                                                                        \
        switch (checksum)                                                               \
        {                                                                               \
            case 101:                                                                   \
            case 111:                                                                   \
            case 121:                                                                   \
            case 131:                                                                   \
            case 141:                                                                   \
                ip += (3 + sizeof (int));                                               \
                break;                                                                  \
            case   0:                                                                   \
            case  10:                                                                   \
            case  20:                                                                   \
            case  30:                                                                   \
            case  40:                                                                   \
            case 110:                                                                   \
            case 120:                                                                   \
            case 130:                                                                   \
            case 140:                                                                   \
                ip += 3;                                                                \
                break;                                                                  \
                                                                                        \
            case   1:                                                                   \
                ip += (3 + sizeof (double));                                            \
                break;                                                                  \
                                                                                        \
            default:                                                                    \
                MY_ASSERT (false, "int checksum", UNEXP_VAL, NULL);                     \
                break;                                                                  \
        }                                                                               \
    }                                                                                   \
    else if (Check_If_Jump (num) == 1)                                                  \
    {                                                                                   \
        ip++;                                                                           \
        label_arr[label_i++] = *(int *)(code_arr + ip);                                 \
        ip += sizeof (int);                                                             \
    }                                                                                   \
    else                                                                                \
        ip += 1 + sizeof (double);                                                      \
                                                                                        \
    break;                                                              

static char *First_Passing (const char *code_arr, const int max_ip, int *n_labels)
{
    MY_ASSERT (code_arr, "const char *code_arr", NULL_PTR, NULL);

    char *label_arr = (char *)calloc (max_ip, sizeof (char));
    MY_ASSERT (label_arr, "char *label_arr", NE_MEM, NULL);
    int label_i = 0;

    for (int ip = 0; ip < max_ip; )
    {
        switch (code_arr[ip])
        {
            #include "../include/Commands_List.h"

            default: 
            MY_ASSERT (false, "code_arr[ip]", UNDEF_CMD, NULL);
        }
    }

    *n_labels = label_i;

    return label_arr;
}
#undef DEFCMD_

enum MATH
{
    add,
    sub,
    mul,
    dvd
};

static int Translate_Math (char *x86_buffer, int *ip, enum MATH instruction)
{
    const char first_part[] = {
                                0xF2, 0x0F, 0x10, 0x4C, 0x24, 0x08,     // movsd   xmm1, qword [rsp + 8]
                                0xF2, 0x0F, 0x10, 0x14, 0x24,           // movsd   xmm2, qword [rsp]
                                0x48, 0x83, 0xC4, 0x08                  // add     rsp, 8
                              };

    memcpy (x86_buffer, first_part, sizeof first_part);
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
            math_instruction[2] = 0x59;     // subsd   xmm1, xmm2
            break;
        case dvd:
            math_instruction[2] = 0x5E;     // subsd   xmm1, xmm2
            break;

        default:
            MY_ASSERT (false, "enum MATH instruction", UNEXP_VAL, ERROR);
    }

    memcpy (x86_buffer, math_instruction, sizeof math_instruction);
    *ip += sizeof math_instruction;

    const char last_part[] = {
                                0xF2, 0x0F, 0x11, 0x0C, 0x24    // movsd   qword [rsp], xmm1
                             };

    memcpy (x86_buffer, last_part, sizeof last_part);
    *ip += sizeof last_part;

    return NO_ERRORS;
}

char *Translate (const char *const input_buff, const long max_ip, const char *const output_name)
{
    MY_ASSERT (input_buff,  "const char *const input buff",  NULL_PTR, NULL);
    MY_ASSERT (output_name, "const char *const output_name", NULL_PTR, NULL);

    FILE *file_ptr = Open_File (output_name, "wb");

    int n_labels = 0;
    char *label_arr = First_Passing (input_buff, max_ip, &n_labels);
    MY_ASSERT (label_arr, "First_Passing ()", FUNC_ERROR, NULL);

    for (int i = 0; i < n_labels - 1; i++)
        for (int j = i + 1; j < n_labels; j++)
            if (label_arr[i] == label_arr[j])
                label_arr[i] = 0;

    char *x86_buffer = (char *)calloc (max_ip, sizeof (char));
    MY_ASSERT (x86_buffer, "char *x86_buffer", NE_MEM, NULL);

#if 0
    int ret_val = Second_Passing (input_buff, max_ip, label_arr, n_labels);
    MY_ASSERT (ret_val != ERROR, "Second_Passing ()", FUNC_ERROR, NULL);
#endif

    free (label_arr);

    Close_File (file_ptr, output_name);

    return x86_buffer;
}

#if 0

#define DEFCMD_(num, name, n_args, code)                                                    \
case num:                                                                                   \
    if (Check_N_Args (num) == 0)                                                            \
    {                                                                                       \
        fprintf (file_ptr, "%s\n", Show_CMD_Name (num));                                    \
        ip++;                                                                               \
    }                                                                                       \
    else if (Check_If_Push_Pop (num) == 1)                                                  \
    {                                                                                       \
        ip++;                                                                               \
        if (code_arr[ip] == 1 && code_arr[ip + 1] != 0 && code_arr[ip + 2] == 1)            \
        {                                                                                   \
            fprintf (file_ptr, "%s [%cx + %d]\n",                                           \
                               Show_CMD_Name (num),                                         \
                               code_arr[ip + 1] + 'a',                                      \
                               *(int *)(code_arr + ip + 3));                                \
            ip += 3 + sizeof (int);                                                         \
        }                                                                                   \
        if (code_arr[ip] == 1 && code_arr[ip + 1] != 0 && code_arr[ip + 2] == 0)            \
        {                                                                                   \
            fprintf (file_ptr, "%s [%cx]\n",                                                \
                               Show_CMD_Name (num),                                         \
                               code_arr[ip + 1] + 'a');                                     \
            ip += 3;                                                                        \
        }                                                                                   \
        if (code_arr[ip] == 1 && code_arr[ip + 1] == 0 && code_arr[ip + 2] == 1)            \
        {                                                                                   \
            fprintf (file_ptr, "%s [%d]\n",                                                 \
                               Show_CMD_Name (num),                                         \
                               *(int *)(code_arr + ip + 3));                                \
            ip += 3 + sizeof (int);                                                         \
        }                                                                                   \
        if (code_arr[ip] == 0 && code_arr[ip + 1] != 0 && code_arr[ip + 2] == 0)            \
        {                                                                                   \
            fprintf (file_ptr, "%s %cx\n",                                                  \
                               Show_CMD_Name (num),                                         \
                               code_arr[ip + 1] + 'a' - 1);                                 \
            ip += 3;                                                                        \
        }                                                                                   \
        if (code_arr[ip] == 0 && code_arr[ip + 1] == 0 && code_arr[ip + 2] == 1)            \
        {                                                                                   \
            fprintf (file_ptr, "push %d\n",                                                 \
                               *(int *)(code_arr + ip + 3));                                \
            ip += 3 + sizeof (double);                                                      \
        }                                                                                   \
        if (code_arr[ip] == 0 && code_arr[ip + 1] == 0 && code_arr[ip + 2] == 0)            \
        {                                                                                   \
            fprintf (file_ptr, "pop\n");                                                    \
            ip += 3;                                                                        \
        }                                                                                   \
    }                                                                                       \
    else if (Check_If_Jump (num) == 1)                                                      \
    {                                                                                       \
        fprintf (file_ptr, "%s %d\n", Show_CMD_Name (num), *(int *)(code_arr + ip + 1));    \
        ip += 1 + sizeof (int);                                                             \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        fprintf (file_ptr, "%s %lf\n",                                                      \
                           Show_CMD_Name (num),                                             \
                           *(double *)(code_arr + ip + 1));                                 \
        ip += 1 + sizeof (double);                                                          \
    }                                                                                       \
                                                                                            \
    break;                                                              

int Second_Passing (const char *code_arr, const int max_ip, FILE *file_ptr, const char *label_arr, const int n_labels)
{
    MY_ASSERT (code_arr,  "const char *code_arr",  NULL_PTR, ERROR);
    MY_ASSERT (label_arr, "const char *label_arr", NULL_PTR, ERROR);

    for (int ip = 0; ip < max_ip; )
    {
        for (int i = 0; i <= n_labels; i++)
        {
            if (label_arr[i] == ip && label_arr[i] != 0)
                fprintf (file_ptr, "%d:\n", label_arr[i]);
        }

        switch (code_arr[ip])
        {
            #include "../Commands_List.h"

            default: MY_ASSERT (false, "code_arr[ip]", UNDEF_CMD, ERROR);
        }
    }

    return NO_ERRORS;
}
#undef DEFCMD_

#define DEFCMD_(num, name, n_args, code)    \
do                                          \
{                                           \
    if (cmd_n == num)                       \
        return #name;                       \
}                                           \
while (0)

const char *Show_CMD_Name (const int cmd_n)
{
    #include "../Commands_List.h"

    return NULL;
}
#undef DEFCMD_

#endif