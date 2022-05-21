#ifndef BINARY_TRANSLATOR_INSLUDED
#define BINARY_TRANSLATOR_INSLUDED

#include "../../../My_Lib/My_Lib.h"
#include <inttypes.h>
#include <string.h>
#include <sys/mman.h>   // for mprotect ()

#ifndef DEBUG
#undef MY_ASSERT
#define MY_ASSERT(condition, var, err_num, error) ;
#endif

int Binary_Translator (const char *const input);

#endif