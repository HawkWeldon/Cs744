#include <stddef.h>
#include "functions.h"

typedef void (*FuncPtr)(char*, char*, size_t);

void root(char*, char*, size_t);
void square(char*, char*, size_t);
void cube(char*, char*, size_t);
void  helloworld(char*, char*, size_t);
void pingpong(char*, char*, size_t);
void prime(char*, char*, size_t);
void fib(char*, char*, size_t);

FuncPtr funcs[NUM_FUNCS] = {    [ROOT] = root,
                                [SQUARE] = square,
                                [CUBE] = cube,
                                [HELLOWORLD] = helloworld,
                                [PINGPONG] = pingpong,
                                [PRIME] = prime,
                                [FIB] = fib                 };