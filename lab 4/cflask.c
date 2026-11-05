#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "http-parser.h"
#include "functions.h"
#include "functions_list.h"

/* 
* Implement all the cflask logic here.
* 1. web server initialization 
* 2. accept http requests
* 3. parse http requests
* 4. dispatch http requests
* 5. process, generate and send http responses
*/


int main(int argc, char *argv[]) 
{

    if(argc !=3 || strcmp(argv[1], "s") != 0)
    {
        fprintf(stderr, "Error 1");
        return 1;
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


    while(1)
    {
        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1)
        {
            fprintf(stderr, "Error 6");
            continue;
        }

        char buffer[8192];

        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) -1 , 0);

        if (bytes > 0)
        {
            buffer[bytes] = '\0';
            ParsedRequest req = {0};
            parse_http_request(buffer, &req);

            printf("Method: %s\n", req.request_line.method);
            printf("URI: %s\n", req.request_line.uri);
        }

        const char *body = "WHATS UP WORLD";
        size_t response_size = 0;

        char* http_response = create_http_response("200 0K", "text/plain", body, strlen(body), &response_size);
        if (http_response != NULL)
        {
            send(client_fd, http_response, response_size, 0);
            free(http_response);
        }

        close(client_fd);
    }

    close(server_fd);
    
    
    
    // // Example raw HTTP POST request string
    // char raw_request[] = 
    //     "POST /submit-form?name=Buzz HTTP/1.1\r\n"
    //     "Host: localhost:8080\r\n"
    //     "Content-Type: application/json\r\n"
    //     "Content-Length: 18\r\n"
    //     "\r\n"
    //     "{\"username\":\"dev\"}";

    // ParsedRequest req;
    // parse_http_request(raw_request, &req);

    // // Print parsed output to verify
    // printf("--- Request Line ---\n");
    // printf("Method:  %s\n", req.request_line.method);
    // printf("URI:     %s\n", req.request_line.uri);
    // printf("Version: %s\n\n", req.request_line.version);

    // printf("--- Headers parsed: %d ---\n", req.header_count);
    // for (int i = 0; i < req.header_count; i++) {
    //     printf("%s -> %s\n", req.headers[i].key, req.headers[i].value);
    // }

    // if (req.body) {
    //     printf("\n--- Body ---\n%s\n\n", req.body);
    // }

    
    // const char *body = "{\"status\":\"success\",\"message\":\"Hello from C server!\"}";
    // size_t len = strlen(body);
    // size_t response_size = 0;

    // char *http_response = create_http_response("200 OK", "text/html", body, len, &response_size);

    // if (http_response) {
    //     printf("--- Generated HTTP Response (%zu bytes) ---\n", response_size);
    //     printf("%s\n", http_response);
        
    //     // Clean up allocated memory
    //     free(http_response);
    // }

    return 0;
}

