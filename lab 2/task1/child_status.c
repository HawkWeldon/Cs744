#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
int main()
{
    int pid = fork();
    
    if (pid < 0) 
    {
        printf("Fork failed to exicute.    \n"); 
        exit(1);
    }
    if (pid)
    {
        printf("Parent process: %d   Child pid: %d \n", getpid(), pid);
        int status;
        wait(&status);
        int exit_code = WEXITSTATUS(status); // status has a lot more info than just the exit code so we use WEXITSTATUS.
        printf("\n");
        printf("Child process %d has terminated \n", pid);
        printf("Child process terminated with the exit code: %d \n", exit_code);
        exit(0);
    }
    else
    {
        printf("Child process: %d   Parent pid: %d \n", getpid(), getppid());
        exit(42);
    }
}