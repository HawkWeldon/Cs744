#include <unistd.h>
#include <fcntl.h>
int main()
{
    int fd = open("dup2.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644); //0644 is rw-r--r-- permissions with the first 0 indicating that it's an Octal.
    char message[] = "Hi there, Welcome to cs744.\n";
    char error_message_open[] = "Open failed.\n";
    char error_message_dup2[] = "Dup2 failed. \n";
    
    if (fd == -1) return 1;
    /*
        if (fd == -1)
        {
            write(STDOUT_FILENO, error_message_open, sizeof(error_message_open));
            retunr -1;
        }
    */

    int n = dup2(fd, STDOUT_FILENO);
    close(fd);

    if (n == -1) return 1;
    /*
        if (n == -1)
        {
            write(STDOUT_FILENO, error_message_dup2, sizeof(error_message_dup2));
            retunr -1;
        }        
    */

    write(STDOUT_FILENO, message, sizeof(message) - 1); //It's printing a symbol that isn't displayable on a .txt so I decided to just remove it.
    close(STDOUT_FILENO);
    return 0;
}