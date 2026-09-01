#include <stdio.h>
#include <unistd.h>
int main()
{
    int pid = getpid();
    printf("cmd_exec PID: %d    \n", pid);
    execlp("ls", "ls", "-l", NULL);
    // execlp("NULL", "NULL", NULL); //To test the fail conditions.
    //Now if execlp() fails.
    
    //Only reached if execlp() fails.
    //perror("execlp");
    printf("Execution failed.   \n");// perror is better but the requirements make this better.
    return 1;
}
/*
Now I know that we can make a child process and involve those commands into the program as well but this is simpler.
*/