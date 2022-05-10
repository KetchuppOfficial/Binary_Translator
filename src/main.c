#include "../include/Binary_Translator.h"

int main (int argc, char *argv[])
{
    if (argc < 3)
    {
        MY_ASSERT (false, "int argc", NOT_ENOUGH_ARGS, ERROR);
    }
    else
    {
        #if DEBUG == 1
        int ret_val = Translate (argv[1], argv[2]);
        #elif DEBUG == 0
        Translate (argv[1], argv[2]);
        #endif

        MY_ASSERT (ret_val != ERROR, "Translate ()", FUNC_ERROR, ERROR);
    }
    
    return 0;
}
