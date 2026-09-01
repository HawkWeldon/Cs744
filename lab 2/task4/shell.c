#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

int background_pid;

typedef struct
{
    int flag_val;
    int flag_index;    
} 
flag;

void set_default(flag* target)
{
    target->flag_val = 0;
    target->flag_index = 0;
}

/* Splits the input string into space-separated tokens */
char **tokenize(char *line)
{
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));

    int i, tokenIndex = 0, tokenNo = 0;

    for (i = 0; i < strlen(line); i++)
    {
        char ch = line[i];

        if (ch == ' ' || ch == '\t' || ch == '\n')
        {
            token[tokenIndex] = '\0';

            if (tokenIndex != 0)
            {
                tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE);
                strcpy(tokens[tokenNo], token);
                tokenNo++;
                tokenIndex = 0;
            }
        }
        else
        {
            token[tokenIndex++] = ch;
        }
    }

    free(token);
    tokens[tokenNo] = NULL;

    return tokens;
}

void handler(int sig)
{
    int status;
    int pid;
    
    while((pid = waitpid(-1,&status,WNOHANG)) > 0) printf("\n[background] pid %d done [exit status %d] \n", pid, status);
}

int main()
{

    signal(SIGCHLD, handler); // I was researching for how to do task D and this is what I found, I don't fully understand how this work but it does the job.
    char line[MAX_INPUT_SIZE];
    char **tokens;
    int i;

    int fd_in_1;
    int fd_in_2 = dup(STDOUT_FILENO);
    int fd_out_1;
    int fd_out_2 = dup(STDOUT_FILENO);

    while (1)
    {   
        printf("shell> ");

        if (fgets(line, sizeof(line), stdin) == NULL)
            break;

        tokens = tokenize(line);

        
        if (tokens[0] == NULL)
        {
            continue;
        }


        if (strcmp(tokens[0], "exit") == 0)
        {
            printf("Exiting \n");
            return 0;
        }

        i = 0;
        flag and;
        set_default(&and);
        while (tokens[i] != NULL)
        {
            if(strcmp(tokens[i], "&&") == 0)
            {
                and.flag_val = 1;
                and.flag_index = i;
                tokens[i] = NULL;
            }
            i++;
        }

        int count = i;

        flag in;
        set_default(&in);
        flag out;
        set_default(&out);
        int amp_flag = 0;
        
        int pid;
        if (!and.flag_val)
        {
               
            i = 0;
            while (tokens[i] != NULL)
            {
                if(strcmp(tokens[i], "<") == 0)
                {
                    in.flag_val = 1;
                    in.flag_index = i;
                    tokens[i] = NULL;
                }
                i++;
            }

            i = 0;
            while (tokens[i] != NULL)
            {
                if(strcmp(tokens[i], ">") == 0)
                {
                    out.flag_val = 1;
                    out.flag_index = i;
                    tokens[i] = NULL;
                }
                i++;
            }

            if(in.flag_val == 1)
            {
                fd_in_1 = open(tokens[in.flag_index + 1], O_RDONLY);
                int n = dup2(fd_in_1, STDIN_FILENO);
            }

            if(out.flag_val == 1)
            {
                fd_out_1 = open(tokens[out.flag_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                int n = dup2(fd_out_1, STDOUT_FILENO);
            }
            

            if (strcmp(tokens[count - 1], "&") == 0)
            {
                amp_flag = 1;
                tokens[count - 1] = NULL;
            }

            pid = fork();
            if (pid)
            {
                int status;
                i = 0;
                close(fd_in_1);
                close(fd_out_1);
                dup2(fd_in_2,STDIN_FILENO);
                dup2(fd_out_2,STDOUT_FILENO);
                if(amp_flag)
                {
                    printf("[background] started pid : %d \n", pid);
                }
                else
                {
                    waitpid(pid, &status, 0);
                    printf("[exit status: %d]\n", WEXITSTATUS(status));
                }
            }
            else
            {
                execvp(tokens[0],&tokens[0]);
                exit(1);
            }
        }
        else 
        {
            i = 0;
            while (tokens[i] != NULL)
            {
                if(strcmp(tokens[i], "<") == 0)
                {
                    in.flag_val = 1;
                    in.flag_index = i;
                    tokens[i] = NULL;
                }
                i++;
            }

            i = 0;
            while (tokens[i] != NULL)
            {
                if(strcmp(tokens[i], "<") == 0)
                {
                    out.flag_val = 1;
                    out.flag_index = i;
                    tokens[i] = NULL;
                }
                i++;
            }
            count = i;

            if(in.flag_val == 1)
            {
                fd_in_1 = open(tokens[in.flag_index + 1], O_RDONLY);
                int n = dup2(fd_in_1, STDIN_FILENO);
            }

            if(out.flag_val == 1)
            {
                fd_out_1 = open(tokens[out.flag_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                int n = dup2(fd_out_1, STDOUT_FILENO);
            }

            if (strcmp(tokens[count - 1], "&") == 0)
            {
                printf("Warning '&' is not supported for double commands. \n");
                tokens[count - 1] == NULL;
            }

            pid = fork();
            if (pid)
            {
                int status;
                i = 0;
                close(fd_in_1);
                close(fd_out_1);
                dup2(fd_in_2,STDIN_FILENO);
                dup2(fd_out_2,STDOUT_FILENO);
                waitpid(pid, &status, 0);
                printf("[exit status: %d]\n", WEXITSTATUS(status));
                if (WEXITSTATUS(status)) continue;
            }
            else
            {
                execvp(tokens[0],&tokens[0]);
                exit(1);
            }

            set_default(&in);
            set_default(&out);

            i = and.flag_index + 1;
            while (tokens[i] != NULL)
            {
                if(strcmp(tokens[i], "<") == 0)
                {
                    in.flag_val = 1;
                    in.flag_index = i;
                    tokens[i] = NULL;
                }
                i++;
            }

            i = and.flag_index + 1;
            while (tokens[i] != NULL)
            {
                if(strcmp(tokens[i], ">") == 0)
                {
                    out.flag_val = 1;
                    out.flag_index = i;
                    tokens[i] = NULL;
                }
                i++;
            }

            count = count + i;

            if (strcmp(tokens[count - 1], "&") == 0)
            {
                printf("Warning '&' is not supported for double commands. \n");   
                tokens[count - 1] == NULL;
            }

            if(in.flag_val == 1)
            {
                fd_in_1 = open(tokens[in.flag_index + 1], O_RDONLY);
                int n = dup2(fd_in_1, STDIN_FILENO);
            }

            if(out.flag_val == 1)
            {
                fd_out_1 = open(tokens[out.flag_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                int n = dup2(fd_out_1, STDOUT_FILENO);
            }



            pid = fork();
            if (pid)
            {
                int status;
                i = 0;
                close(fd_in_1);
                close(fd_out_1);
                dup2(fd_in_2,STDIN_FILENO);
                dup2(fd_out_2,STDOUT_FILENO); 
                waitpid(pid, &status, 0);         
                printf("[exit status: %d]\n", WEXITSTATUS(status));
            }
            else
            {
                execvp(tokens[and.flag_index + 1],&tokens[and.flag_index + 1]);
                exit(1);
            }
        }
        /*
         * 1. Create a child process using fork().
         * 2. In the child process, execute the command using execvp().
         * 3. If execvp() fails, print an error message and terminate the child.
         * 4. In the parent process, wait for the child to complete.
         */

        /* Free allocated memory */
        for (i = 0; tokens[i] != NULL; i++)
            free(tokens[i]);

        free(tokens);

    }
    
}