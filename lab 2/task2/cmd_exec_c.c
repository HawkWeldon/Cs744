#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Give some input !!! \n");
        return 1;
    }

    int pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if(!pid)
    {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(1);
    }
    else 
    {
        int ppid = getpid();
        printf("Parent PID: %d  \n", ppid);
        printf("Child PID: %d   \n", pid);
        
        int status;
        wait(&status);
        status = WEXITSTATUS(status);
        if(status == 0) printf("Child Process completed successfully.   \n");
        else printf("Child process terminated.  \n");
    }
    return 0;
}