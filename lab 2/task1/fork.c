#include <stdio.h>
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
        exit(0);
    }
    else
    {
        printf("Child process: %d   Parent pid: %d \n", getpid(), getppid());
        exit(0);
    }
}
// I am using exit(0) because it's the fancy new thing I just learned, I know I can just return 0 instead here.