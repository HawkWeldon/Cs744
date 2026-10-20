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

void helloworld(char *args, char *response, size_t size)
{
    if (args) snprintf(response, size, "Hello %s", args);
    else snprintf(response, size, "Hello");
}

void pingpong(char *args, char *response, size_t size)
{
    if (args) snprintf(response, size, "%s", args);
    else snprintf(response, size, "PingPong");
}

void prime(char *args, char *response, size_t size)
{
    long long num = args ? atoll(args) : 0;
    bool is_prime =  num >= 2;

    for (long long i = 2; i * i <= num; i++)
    {
        if (num % i == 0) 
        {
            is_prime = false;
            break;
        }
    }

    snprintf(response, size, "%s", is_prime ? "True" : "False");
}

void fib(char *args, char *response, size_t size)
{
    int num = args ? atoll(args) : 0;

    if (num < 0 || num > 93) 
    {
        snprintf(response, size, "Invalid input");
        return;
    }

    long long a = 0, b = 1;

    for (int i = 0; i < num; i++)
    {
        long long temp = a + b;
        a = b;
        b = temp;
    }

    snprintf(response, size, "%lld", a);
}