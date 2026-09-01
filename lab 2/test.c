#include <fcntl.h>
#include <unistd.h>

int main()
{
    char buffer[100];

    // Open the file for reading
    int fd = open("file.txt", O_RDONLY);

    // Read up to 100 bytes from the file
    int n = read(fd, buffer, 100);

    // Close the file
    close(fd);

    // Write those bytes to the terminal
    write(STDOUT_FILENO, buffer, n);

    fd = open("file2.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, buffer, n);
    close(fd);
    
    return 0;
}