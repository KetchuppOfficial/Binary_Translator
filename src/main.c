#include "../include/Binary_Translator.h"

int main (int argc, char *argv[])
{
    if (argc < 3)
    {
        MY_ASSERT (false, "int argc", NOT_ENOUGH_ARGS, ERROR);
    }
    else
    {
        #ifdef DEBUG
        int ret_val = Binary_Translator (argv[1], argv[2]);
        #else
        Binary_Translator (argv[1], argv[2]);
        #endif

        MY_ASSERT (ret_val != ERROR, "Translate ()", FUNC_ERROR, ERROR);
    }
    
    return 0;
}
