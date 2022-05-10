#ifndef BINARY_TRANSLATOR_INSLUDED
#define BINARY_TRANSLATOR_INSLUDED

#include "../../../My_Lib/My_Lib.h"
#include <inttypes.h>

#define DEBUG 1

#if DEBUG == 0
#undef MY_ASSERT
#define MY_ASSERT(condition, var, err_num, error) ;
#endif

int Translate (const char *const input, const char *const output);

#endif