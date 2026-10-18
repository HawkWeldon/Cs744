#include <stdio.h>
#include <math.h>
#include <string.h>
#include "functions.h"

int root(int key)
{
    return sqrt(key);
}

int square(int key)
{
    return key*key;
}

int cube(int key)
{
    return key*key*key;
}

char* helloworld()
{
    return "Hello world!!!"
}

char* pingPong(char* input)
{
    if (strcmp(input, "ping")) return "pong";
    else if (strcmp(input, "pong")) return "ping";
    else return "invalid"
}

int prime ()

int main()
{
    return 0;
}