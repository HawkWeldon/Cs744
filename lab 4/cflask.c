#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "http-parser.h"
#include "functions.h"
#include "functions_list.h"
#include <pthread.h>

/* 
* Implement all the cflask logic here.
* 1. web server initialization 
* 2. accept http requests
* 3. parse http requests
* 4. dispatch http requests
* 5. process, generate and send http responses
*/
#define QUEUE_SIZE 1024

int client_queue[QUEUE_SIZE];
int queue_front = 0;
int queue_rear = 0;
int queue_count = 0;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_not_full = PTHREAD_COND_INITIALIZER;

void enqueue(int client_fd)
{
    pthread_mutex_lock(&queue_mutex);

    while (queue_count == QUEUE_SIZE)
    {
        pthread_cond_wait(&queue_not_full, &queue_mutex);
    }

    client_queue[queue_rear] = client_fd;

    queue_rear = (queue_rear + 1) % QUEUE_SIZE;
    queue_count++;

    pthread_cond_signal(&queue_cond);

    pthread_mutex_unlock(&queue_mutex);
}

int dequeue(void)
{
    pthread_mutex_lock(&queue_mutex);

    while (queue_count == 0)
    {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    int client_fd = client_queue[queue_front];

    queue_front = (queue_front + 1) % QUEUE_SIZE;
    queue_count--;

    pthread_cond_signal(&queue_not_full);

    pthread_mutex_unlock(&queue_mutex);

    return client_fd;
}

void *handle_client(void *args)
{
    int client_fd = *(int*)args;
    free(args);

    char buffer[8192];
    char body [1024];
    char response_body[1024];

    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) -1 , 0);

    if (bytes > 0)
    {
        buffer[bytes] = '\0';
        ParsedRequest req = {0};
        parse_http_request(buffer, &req);

        char* path = req.request_line.uri;
        char* args = NULL;

        char* question_mark = strchr(path, '?');

        if(question_mark)
        {
            *question_mark = '\0';
            args = question_mark + 1;
        }

        printf("Method: %s\n", req.request_line.method);
        printf("URI: %s\n", req.request_line.uri);

        
        int function_id = -1;

        for (int i = 0; i < NUM_FUNCS; i++)
        {
            if(strcmp(path, routes[i].url) == 0)
            {
                function_id = routes[i].id;
                break;
            }
        }

        if (function_id != -1)
        {
            char* value = NULL;

            if (args != NULL)
            {
                char* equal_sign = strchr(args, '=');

                if (equal_sign)
                {
                    value = equal_sign + 1;
                }
            }

            funcs[function_id](value, response_body, sizeof(response_body));
        }
        else 
        {
            snprintf(response_body, sizeof(response_body), "404 NOT FOUND");
        }
        snprintf(body, sizeof(body), "WHATS UP WORLD\n%s ::: %s\n%d \n%s \n", path, args, function_id, response_body);

    }

    size_t response_size = 0;

    char* http_response = create_http_response("200 OK", "text/plain", body, strlen(body), &response_size);
    if (http_response != NULL)
    {
        send(client_fd, http_response, response_size, 0);
        free(http_response);
    }


    close(client_fd);
    return NULL;
}

void *worker(void *arg)
{
    (void)arg;

    while (1)
    {
        int client_fd = dequeue();

        int *client_args = malloc(sizeof(int));

        if (client_args == NULL)
        {
            perror("malloc");
            close(client_fd);
            continue;
        }

        *client_args = client_fd;

        handle_client(client_args);
    }

    return NULL;
}

int main(int argc, char *argv[]) 
{

    if((strcmp(argv[1], "s") != 0 && strcmp(argv[1], "m") != 0 && strcmp(argv[1], "t") != 0))
    {
        fprintf(stderr, "Error 1");
        return 1;
    }

    int thread_pool_mode = strcmp(argv[1], "t") == 0;

    if ((thread_pool_mode && argc != 4) || (!thread_pool_mode && argc != 3))
    {
        fprintf(stderr, "Error 7");
        return 1;
    }

    int num_threads = 0;

    if (thread_pool_mode)
    {
        char *thread_end;
        long count = strtol(argv[3], &thread_end, 10);

        if (*argv[3] == '\0' || *thread_end != '\0' || count < 1 || count > 256)
        {
            fprintf(stderr, "Error 8");
            return 1;
        }

        num_threads = (int)count;
    }

    char *end;
    long port = strtol(argv[2], &end, 10);
   
    if(*argv[2] == '\0' || *end != '\0' || port < 1|| port > 65535)
    {
        fprintf(stderr, "Error 2");
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_fd == -1)
    {
        fprintf(stderr, "Error 3");
        return 1;
    }

    struct sockaddr_in address = {0};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        fprintf(stderr, "Error 4");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) == -1)
    {
        fprintf(stderr, "Error 5");
        close(server_fd);
        return 1;
    }

    printf("===============cflask is in business===============\n");

    if (thread_pool_mode)
    {
        for (int i = 0; i < num_threads; i++)
        {
            pthread_t thread;

            int result = pthread_create(
                &thread,
                NULL,
                worker,
                NULL
            );

            if (result != 0)
            {
                fprintf(stderr, "Error 9");
                exit(EXIT_FAILURE);
            }

            pthread_detach(thread);
        }

        printf("Thread pool initialized with %d workers\n", num_threads);
    }

    while(1)
    {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1)
        {
            fprintf(stderr, "Error 6");
            continue;
        }

        int *client_args = malloc(sizeof(int));
        *client_args = client_fd;
        //handle_client(client_args);

        if (strcmp(argv[1], "s") == 0)
        {
            handle_client(client_args);
        }
        else
        {
            pthread_t thread;
            int result = pthread_create(&thread, NULL, handle_client, client_args);

            if (result)
            {
                fprintf(stderr, "Error 7");
                free(client_args);
                close(client_fd);
                continue;
            }
            pthread_detach(thread);
        }
    }
}

