#include "../include/Binary_Translator.h"

int Check_Argc (const int argc, const int expected)
{
    return (argc == expected) ? 0 : 1;
}

int main (int argc, char *argv[])
{
    #ifdef DEBUG
    Open_Log_File ("Binary_Translator");
    #endif
    
    MY_ASSERT (Check_Argc (argc, 2) == 0, "int argc", NE_MAIN_ARGS, ERROR);

    #ifdef DEBUG
    int ret_val = Binary_Translator (argv[1]);
    #else
    Binary_Translator (argv[1]);
    #endif

    MY_ASSERT (ret_val != ERROR, "Translate ()", FUNC_ERROR, ERROR);
    
    return 0;
}
