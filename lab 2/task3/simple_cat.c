#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
int main(int argc, char* argv[])
{
    int args_bool = 1;
    char buffer[100];
    char completetion_message[] = "Child process has successfully terminated.\n";
    char abnormal_exit_message[] = "Child exited abnormally.\n";
    char error_message_overflow[] = "Give a file name and nothing else.\n";
    char error_message_open_fail[]  = "open() has failed.\n";
    char error_message_read_fail[] = "read() has failed.\n";
    char error_message_fork_fail[] = "fork() has failed.\n";

    if (argc < 2) args_bool = 0;

    if (args_bool)
    {
        if (argc > 2)
        {
            write(STDOUT_FILENO, error_message_overflow, sizeof(error_message_overflow));
            return 1;
        }

        int fd = open(argv[1], O_RDONLY);
        if (fd == -1)
        {
            write(STDOUT_FILENO, error_message_open_fail, sizeof(error_message_open_fail));
            return 1;
        }

        int n = read(fd, buffer, 100);
        if (n == -1)
        {
            write(STDOUT_FILENO, error_message_read_fail, sizeof(error_message_read_fail));
            return 1;
        }

        write(STDOUT_FILENO, buffer, n);
        return 0;
    }
    else
    {
        int n = fork();

        if (n == -1)
        {
            write(STDOUT_FILENO, error_message_fork_fail, sizeof(error_message_fork_fail));
            return 1;
        }

        int fd = dup(STDIN_FILENO);
        int fd1 = dup(STDOUT_FILENO);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);

        if (n > 0)
        {
            wait(NULL);
            write(fd1, completetion_message, sizeof(completetion_message));
            close(fd);
            close(fd1);
            return 0;
        }
        else
        {
            int m = read(fd, buffer, 100);
            if (m == -1) 
            {
                write(fd1, error_message_read_fail, sizeof(error_message_read_fail));
                exit(1);   
            }
            write(fd1, buffer, m);
            exit(0);
        }
    }
}