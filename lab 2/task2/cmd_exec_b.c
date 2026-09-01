#include <stdio.h>
#include <unistd.h>
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Give some input !!! \n");
        return 1;
    }
    //argc is the number of things in the comandline which includes ./cmd_exec_b and then the arguments if its 1 then we know that there was no arguments.

    int pid = getpid();
    printf("cmd_exec PID: %d    \n", pid);

    execvp(argv[1], &argv[1]);//argv[0] is the ./cmd_exec_b so we want from 1.

    printf("Execution failed.   \n");
    return 1;
}