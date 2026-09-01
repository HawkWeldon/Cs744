#include <unistd.h>
int main()
{
    char buffer[1000];
    int n;

    int fd = dup(STDIN_FILENO);
    close(STDIN_FILENO);

    int fd1 = dup(STDOUT_FILENO);
    close(STDOUT_FILENO);

    while ((n = read(fd, buffer, sizeof(buffer))) > 0) write(fd1, buffer, n);

    close(fd);
    close(fd1);

    if (n == 0) return 0;
    else return 1;

    /*
        if(n == 0)
        {
            write(STDOUT_FILENO, "Terminated Normally.\n", 20);
            return 0;
        }
        else 
        {
            write(STDOUT_FILENO, "Termination on fault.\n", 22);
            return 1;
        }
        //If I am supposed to be printing messages as well.
    */
}