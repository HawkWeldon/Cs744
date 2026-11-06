#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include<stdatomic.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "http-parser.h"
#include "functions.h"
#include "functions_list.h"

#define BUFFER_SIZE 8192
#define RESPONSE_SIZE 1024
#define QUEUE_SIZE 1024

// Thread pool queue

int client_queue[QUEUE_SIZE];

int queue_front = 0;
int queue_rear = 0;
int queue_count = 0;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_not_full = PTHREAD_COND_INITIALIZER;


atomic_int server_running = 1;
int server_fd = -1;

void shutdown_server(int signal_number)
{
    (void) signal_number;
    server_running = 0;
}

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


// HTTP response functions

int send_all(int client_fd, const char *data, size_t length)
{
    size_t sent = 0;

    while (sent < length)
    {
        ssize_t n = send(client_fd, data + sent, length - sent, MSG_NOSIGNAL);

        if (n < 0 && errno == EINTR)
        {
            continue;
        }

        if (n <= 0)
        {
            return -1;
        }

        sent += (size_t)n;
    }

    return 0;
}


void send_response(int client_fd, const char *status, const char *body)
{
    size_t response_size = 0;

    char *http_response = create_http_response(
        status,
        "text/plain",
        body,
        strlen(body),
        &response_size
    );

    if (http_response == NULL)
    {
        fprintf(stderr, "Error: Could not create HTTP response\n");
        return;
    }

    if (send_all(client_fd, http_response, response_size) == -1)
    {
        fprintf(stderr, "Error: Could not send HTTP response\n");
    }

    free(http_response);
}


void send_error(int client_fd, const char *status, const char *message)
{
    send_response(client_fd, status, message);
}


// URL decoding

int hex_value(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    else if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    else if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }

    return -1;
}


int url_decode(char *str)
{
    char *src = str;
    char *dst = str;

    while (*src != '\0')
    {
        if (*src == '%')
        {
            if (src[1] == '\0' || src[2] == '\0')
            {
                return -1;
            }

            int hi = hex_value(src[1]);
            int lo = hex_value(src[2]);

            if (hi == -1 || lo == -1)
            {
                return -1;
            }

            char decoded = (char)((hi << 4) | lo);

            if (decoded == '\0')
            {
                return -1;
            }

            *dst = decoded;
            dst++;
            src += 3;
        }
        else if (*src == '+')
        {
            *dst = ' ';
            dst++;
            src++;
        }
        else
        {
            *dst = *src;
            dst++;
            src++;
        }
    }

    *dst = '\0';

    return 0;
}


// Query parameter parsing

int get_query_param(const char *query, const char *key, char **result)
{
    *result = NULL;

    if (query == NULL || key == NULL)
    {
        return 0;
    }

    size_t key_len = strlen(key);

    while (*query != '\0')
    {
        const char *end = strchr(query, '&');

        if (end == NULL)
        {
            end = query + strlen(query);
        }

        const char *equal_sign = memchr(query, '=', end - query);

        if (equal_sign != NULL)
        {
            size_t current_key_len = equal_sign - query;

            if (current_key_len == key_len && strncmp(query, key, key_len) == 0)
            {
                const char *value = equal_sign + 1;
                size_t value_len = end - value;

                char *copy = malloc(value_len + 1);

                if (copy == NULL)
                {
                    return -1;
                }

                memcpy(copy, value, value_len);
                copy[value_len] = '\0';

                if (url_decode(copy) == -1)
                {
                    free(copy);
                    return -1;
                }

                *result = copy;

                return 1;
            }
        }

        if (*end == '\0')
        {
            break;
        }

        query = end + 1;
    }

    return 0;
}


// Read HTTP request

ssize_t read_http_request(int client_fd, char *buffer, size_t size)
{
    size_t total = 0;
    size_t expected = 0;
    int headers_complete = 0;

    while (total < size - 1)
    {
        ssize_t bytes = recv(client_fd, buffer + total, size - total - 1, 0);

        if (bytes < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return -3;
        }

        if (bytes == 0)
        {
            return -1;
        }

        total += (size_t)bytes;
        buffer[total] = '\0';

        if (headers_complete == 0)
        {
            char *header_end = strstr(buffer, "\r\n\r\n");

            if (header_end != NULL)
            {
                headers_complete = 1;

                size_t header_size = header_end + 4 - buffer;
                size_t content_length = 0;
                int found_length = 0;

                char *line = strstr(buffer, "\r\n");

                if (line == NULL)
                {
                    return -1;
                }

                line += 2;

                while (line < header_end)
                {
                    char *next = strstr(line, "\r\n");

                    if (next == NULL || next > header_end)
                    {
                        return -1;
                    }

                    if (strncasecmp(line, "Content-Length:", 15) == 0)
                    {
                        if (found_length)
                        {
                            return -1;
                        }

                        found_length = 1;

                        char *start = line + 15;

                        while (start < next && (*start == ' ' || *start == '\t'))
                        {
                            start++;
                        }

                        if (start == next || !isdigit((unsigned char)*start))
                        {
                            return -1;
                        }

                        errno = 0;
                        char *endptr;

                        unsigned long long length = strtoull(start, &endptr, 10);

                        while (endptr < next && (*endptr == ' ' || *endptr == '\t'))
                        {
                            endptr++;
                        }

                        if (errno != 0 || endptr != next)
                        {
                            return -1;
                        }

                        if (length > size - 1 - header_size)
                        {
                            return -2;
                        }

                        content_length = (size_t)length;
                    }

                    if (strncasecmp(line, "Transfer-Encoding:", 18) == 0)
                    {
                        return -1;
                    }

                    line = next + 2;
                }

                expected = header_size + content_length;
            }
        }

        if (headers_complete && total >= expected)
        {
            buffer[expected] = '\0';
            return expected;
        }
    }

    return -2;
}


// Client handler

void *handle_client(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};
    char response_body[RESPONSE_SIZE] = {0};

    ssize_t bytes = read_http_request(client_fd, buffer, sizeof(buffer));

    if (bytes <= 0)
    {
        if (bytes == -2)
        {
            send_error(client_fd, "413 Content Too Large", "Request too large");
        }
        else
        {
            send_error(client_fd, "400 Bad Request", "Invalid HTTP request");
        }

        close(client_fd);
        return NULL;
    }

    ParsedRequest req = {0};
    parse_http_request(buffer, &req);

    if (req.request_line.method == NULL || req.request_line.uri == NULL)
    {
        send_error(client_fd, "400 Bad Request", "Invalid request line");

        close(client_fd);
        return NULL;
    }

    if (strcmp(req.request_line.method, "GET") != 0 &&
        strcmp(req.request_line.method, "POST") != 0)
    {
        send_error(client_fd, "405 Method Not Allowed", "Only GET and POST supported");

        close(client_fd);
        return NULL;
    }

    char *path = req.request_line.uri;
    char *args = NULL;

    char *question_mark = strchr(path, '?');

    if (question_mark != NULL)
    {
        *question_mark = '\0';
        args = question_mark + 1;
    }

    if (path[0] != '/')
    {
        send_error(client_fd, "400 Bad Request", "Invalid request path");

        close(client_fd);
        return NULL;
    }

    printf("Method: %s\n", req.request_line.method);
    printf("URI: %s\n", path);

    int function_id = -1;

    for (int i = 0; i < NUM_FUNCS; i++)
    {
        if (strcmp(path, routes[i].url) == 0)
        {
            function_id = routes[i].id;
            break;
        }
    }

    if (function_id < 0 || function_id >= NUM_FUNCS)
    {
        send_error(client_fd, "404 Not Found", "404 NOT FOUND");

        close(client_fd);
        return NULL;
    }

    const char *key = "num";

    if (function_id == HELLOWORLD)
    {
        key = "str";
    }
    else if (function_id == PINGPONG)
    {
        key = "Val";
    }

    const char *params = args;

    if (strcmp(req.request_line.method, "POST") == 0)
    {
        params = req.body;

        int form_content_type = 0;

        for (int i = 0; i < req.header_count; i++)
        {
            if (strcasecmp(req.headers[i].key, "Content-Type") == 0 &&
                strncasecmp(req.headers[i].value,
                            "application/x-www-form-urlencoded", 33) == 0)
            {
                form_content_type = 1;
                break;
            }
        }

        if (form_content_type == 0)
        {
            send_error(client_fd, "415 Unsupported Media Type",
                       "Unsupported POST content type");

            close(client_fd);
            return NULL;
        }
    }

    char *value = NULL;

    int param_result = get_query_param(params, key, &value);

    if (param_result == -1)
    {
        send_error(client_fd, "400 Bad Request", "Invalid query parameter");

        close(client_fd);
        return NULL;
    }

    funcs[function_id](value, response_body, sizeof(response_body));

    free(value);

    send_response(client_fd, "200 OK", response_body);

    close(client_fd);
    return NULL;
}


// Worker thread

void *worker(void *arg)
{
    (void)arg;

    while (1)
    {
        int client_fd = dequeue();

        int *client_args = malloc(sizeof(int));

        if (client_args == NULL)
        {
            fprintf(stderr, "Error: malloc failed\n");
            close(client_fd);
            continue;
        }

        *client_args = client_fd;

        handle_client(client_args);
    }

    return NULL;
}


// Main

int main(int argc, char *argv[])
{
    if (argc < 3 ||
        (strcmp(argv[1], "s") != 0 &&
         strcmp(argv[1], "m") != 0 &&
         strcmp(argv[1], "t") != 0))
    {
        fprintf(stderr, "Usage: %s <s|m|t> <port> [num_threads]\n", argv[0]);
        return 1;
    }

    int thread_pool_mode = strcmp(argv[1], "t") == 0;

    if ((thread_pool_mode && argc != 4) ||
        (!thread_pool_mode && argc != 3))
    {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        return 1;
    }

    char *end;
    errno = 0;

    long port = strtol(argv[2], &end, 10);

    if (errno != 0 || *argv[2] == '\0' || *end != '\0' || port < 1 || port > 65535)
    {
        fprintf(stderr, "Error: Invalid port\n");
        return 1;
    }

    int num_threads = 0;

    if (thread_pool_mode)
    {
        char *thread_end;
        errno = 0;

        long count = strtol(argv[3], &thread_end, 10);

        if (errno != 0 || *argv[3] == '\0' || *thread_end != '\0' || count < 1 || count > 256)
        {
            fprintf(stderr, "Error: Invalid thread count\n");
            return 1;
        }

        num_threads = (int)count;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        fprintf(stderr, "Error: socket failed\n");
        return 1;
    }

    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        fprintf(stderr, "Error: setsockopt failed\n");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in address = {0};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        fprintf(stderr, "Error: bind failed\n");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) == -1)
    {
        fprintf(stderr, "Error: listen failed\n");
        close(server_fd);
        return 1;
    }

    printf("===============cflask is in business===============\n");
    printf("Mode: %s\n", argv[1]);
    printf("Port: %ld\n", port);

    if (thread_pool_mode)
    {
        for (int i = 0; i < num_threads; i++)
        {
            pthread_t thread;

            int result = pthread_create(&thread, NULL, worker, NULL);

            if (result != 0)
            {
                fprintf(stderr, "Error: Worker creation failed\n");
                close(server_fd);
                return 1;
            }

            pthread_detach(thread);
        }

        printf("Thread pool initialized with %d workers\n", num_threads);
    }

    struct sigaction action = {0};
    action.sa_handler = shutdown_server;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        fprintf(stderr, "Error: Could not register SIGINT handler\n");
        return 1;
    }

    if (sigaction(SIGTERM, &action, NULL) == -1)
    {
        fprintf(stderr, "Error: Could not register SIGTERM handler\n");
        return 1;
    }

    while (server_running)
    {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            fprintf(stderr, "Error: accept failed\n");
            continue;
        }

        struct timeval timeout;

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
        {
            fprintf(stderr, "Error: Could not set receive timeout\n");
            close(client_fd);
            continue;
        }

        if (setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1)
        {
            fprintf(stderr, "Error: Could not set send timeout\n");
            close(client_fd);
            continue;
        }

        if (thread_pool_mode)
        {
            enqueue(client_fd);
        }
        else
        {
            int *client_args = malloc(sizeof(int));

            if (client_args == NULL)
            {
                fprintf(stderr, "Error: malloc failed\n");
                close(client_fd);
                continue;
            }

            *client_args = client_fd;

            if (strcmp(argv[1], "s") == 0)
            {
                handle_client(client_args);
            }
            else
            {
                pthread_t thread;

                int result = pthread_create(&thread, NULL, handle_client, client_args);

                if (result != 0)
                {
                    fprintf(stderr, "Error: Thread creation failed\n");
                    free(client_args);
                    close(client_fd);
                    continue;
                }

                pthread_detach(thread);
            }
        }
    }

    close(server_fd);
    printf("\ncflask downed \n");
    return 0;
}
