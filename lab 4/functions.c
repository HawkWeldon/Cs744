#include <stdio.h>
#include <string.h>
#include "functions.h"

void root(char *args, char *response, size_t size)
{
    snprintf(response, size, "Hello world!!!");
}

void square(char *args, char *response, size_t size)
{
    int num = 1;
    if (args) num = atoi(args);
    snprintf(response, size, "%d", num * num);
}

void cube(char *args, char *response, size_t size)
{
    int num = 1;
    if (args) num = atoi(args);
    snprintf(response, size, "%d", num * num * num);
}

void helloworld(char *args, char *respone, size_t size)
{
    
}

int main()
{
    return 0;
}