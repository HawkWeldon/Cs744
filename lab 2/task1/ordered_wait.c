#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
int main()
{
    int child_1 = fork();
    int child_2;
    if (child_1) child_2 = fork();

    if (child_1 < 0 || child_2 < 0) 
    {
        printf("Fork failed to exicute.    \n"); 
        exit(1);
    }
    if (child_1 && child_2)
    {
        int status;
        int exit_code;
        
        waitpid(child_1, &status, 0);
        exit_code = WEXITSTATUS(status);
        printf("First child has terminated \n");
        printf("First child exit status = %d    \n", exit_code);

        waitpid(child_2, &status, 0);
        exit_code = WEXITSTATUS(status);
        printf("Second child has terminated \n");
        printf("Second child exit status = %d   \n", exit_code);

        exit(0);
    }
    else if (!child_1)
    {
        printf("First child process %d, parent PID %d   \n", getpid(), getppid());
        sleep(5);
        exit(255);
    }
    else //We have the 00 case 1X case and X1
    {
        printf("Second child process %d, prarent PID %d    \n", getpid(), getppid());
        sleep(2);
        exit(64);
    }
}