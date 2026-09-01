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
        wait(NULL);//NULL goes in here when we don't care about the termination status of the child.
        printf("Child process %d has terminated", pid);
        exit(0);
    }
    else
    {
        printf("Child process: %d   Parent pid: %d \n", getpid(), getppid());
        exit(0);
    }
}