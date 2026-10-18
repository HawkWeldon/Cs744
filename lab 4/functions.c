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

int prime(int key)
{
    if (key < 2) return 0;

    if (key < 4) return 1;

    if (key % 2 == 0) return 0;

    for (int i = 5; i*i <= key; i+=2)
    {
        if (key % i == 0) return 0;
    }

    return 1;
}

int fib
int main()
{
    return 0;
}