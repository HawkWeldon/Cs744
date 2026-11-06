#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
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

static int queue[QUEUE_SIZE];
static int front = 0, rear = 0, count = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

void enqueue(int fd)
{
    pthread_mutex_lock(&mutex);
    while (count == QUEUE_SIZE)
        pthread_cond_wait(&not_full, &mutex);
    queue[rear] = fd;
    rear = (rear + 1) % QUEUE_SIZE;
    count++;
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
}

int dequeue(void)
{
    pthread_mutex_lock(&mutex);
    while (count == 0)
        pthread_cond_wait(&not_empty, &mutex);
    int fd = queue[front];
    front = (front + 1) % QUEUE_SIZE;
    count--;
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
    return fd;
}


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
        return;

    send_all(client_fd, http_response, response_size);
    free(http_response);
}

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

// Each client connection is handled here, in any of the three modes.
void *handle_client(void *arg)
{
    int fd = (int)(intptr_t)arg;
    char buffer[BUFFER_SIZE] = {0};
    char body[RESPONSE_SIZE] = {0};
    const char *status = "200 OK";
    const char *message = NULL;
    char *value = NULL;

    ssize_t n = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0)
    {
        status = "400 Bad Request";
        message = "Invalid HTTP request";
        goto finish;
    }
    buffer[n] = '\0';

    ParsedRequest req = {0};
    parse_http_request(buffer, &req);

    if (req.request_line.method[0] == '\0' || req.request_line.uri[0] == '\0')
    {
        status = "400 Bad Request";
        message = "Invalid request line";
        goto finish;
    }

    int is_post = strcmp(req.request_line.method, "POST") == 0;
    if (!is_post && strcmp(req.request_line.method, "GET") != 0)
    {
        status = "405 Method Not Allowed";
        message = "Only GET and POST supported";
        goto finish;
    }

    char *path = req.request_line.uri;
    char *args = strchr(path, '?');

    printf("Method: %s\n", req.request_line.method);
    printf("URI: %s\n", path);
    
    if (args != NULL)
        *args++ = '\0';

    if (path[0] != '/')
    {
        status = "400 Bad Request";
        message = "Invalid request path";
        goto finish;
    }

    if (strchr(path + 1, '/') != NULL)
    {
        status = "404 Not Found";
        message = "404 NOT FOUND";
        goto finish;
    }

    int id = -1;
    for (int i = 0; i < NUM_FUNCS; i++)
    {
        if (strcmp(path, routes[i].url) == 0)
        {
            id = routes[i].id;
            break;
        }
    }
    if (id < 0 || id >= NUM_FUNCS)
    {
        status = "404 Not Found";
        message = "404 NOT FOUND";
        goto finish;
    }

    const char *key = "num";
    if (id == HELLOWORLD)
        key = "str";
    else if (id == PINGPONG)
        key = "Val";

    if (is_post)
    {
        int valid = 0;
        for (int i = 0; i < req.header_count; i++)
        {
            if (strcasecmp(req.headers[i].key, "Content-Type") == 0 &&
                strncasecmp(req.headers[i].value, "application/x-www-form-urlencoded", 33) == 0)
            {
                valid = 1;
                break;
            }
        }
        if (!valid)
        {
            status = "415 Unsupported Media Type";
            message = "Unsupported POST content type";
            goto finish;
        }
        args = req.body;
    }

    if (get_query_param(args, key, &value) == -1)
    {
        status = "400 Bad Request";
        message = "Invalid query parameter";
        goto finish;
    }

    funcs[id](value, body, sizeof(body));
    message = body;

finish:
    send_response(fd, status, message);
    free(value);
    close(fd);
    return NULL;
}

void *worker(void *arg)
{
    (void)arg;
    while (1)
        handle_client((void *)(intptr_t)dequeue());
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 4 || strlen(argv[1]) != 1 ||
        (argv[1][0] != 's' && argv[1][0] != 'm' && argv[1][0] != 't') ||
        (argv[1][0] == 't' ? argc != 4 : argc != 3))
    {
        fprintf(stderr, "Usage: %s <s|m|t> <port> [num_threads]\n", argv[0]);
        return 1;
    }

    char *end;
    long port = strtol(argv[2], &end, 10);
    if (!*argv[2] || *end || port < 1 || port > 65535)
    {
        return 1;
    }

    int threads = 0;
    if (argv[1][0] == 't')
    {
        long n = strtol(argv[3], &end, 10);
        if (!*argv[3] || *end || n < 1 || n > 256)
        {
            return 1;
        }
        threads = (int)n;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(server_fd, 128) < 0)
    {
        close(server_fd);
        return 1;
    }

    if (threads)
    {
        for (int i = 0; i < threads; i++)
        {
            pthread_t thread;
            if (pthread_create(&thread, NULL, worker, NULL) != 0)
            {
                return 1;
            }
            pthread_detach(thread);
        }
    }

    printf("cflask mode %s listening on port %ld\n", argv[1], port);
    fflush(stdout);

    printf("===============cflask is in business===============\n");
    while (1)
    {
        printf("Mode: %s\n", argv[1]);
        printf("Port: %ld\n", port);
        int fd = accept(server_fd, NULL, NULL);
        if (fd < 0)
        {
            if (errno == EINTR)
                continue;
            continue;
        }

        if (argv[1][0] == 's')
            handle_client((void *)(intptr_t)fd);
        else if (argv[1][0] == 't')
            enqueue(fd);
        else
        {
            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_client, (void *)(intptr_t)fd) != 0)
            {
                close(fd);
                continue;
            }
            pthread_detach(thread);
        }
    }
    return 0;
}

