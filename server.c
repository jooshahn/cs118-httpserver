#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <time.h>
#include <inttypes.h>

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app
{
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

void parse_args(int argc, char *argv[], struct server_app *app);
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *method, const char *path, const char *http_type, const char *content_type);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path, const char *http_type, const char *content_type, const char *request);

int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1)
        {
            perror("accept failed");
            continue;
        }

        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1)
    {
        switch (opt)
        {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL)
    {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0)
    {
        return; // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    if (request == NULL)
    {
        // ERROR - malloc failed
        fprintf(stderr, "MEMORY ALLOCATION ERROR");
        exit(1);
    }
    strcpy(request, buffer);

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html

    printf("%s\n", request); // print out of HTTP request message from server

    char *component = malloc(BUFFER_SIZE);
    if (component == NULL)
    {
        // ERROR - malloc failed
        fprintf(stderr, "MEMORY ALLOCATION ERROR");
        exit(1);
    }

    // get http method
    char http_method[BUFFER_SIZE];
    component = strtok(request, " \r\n");
    strcpy(http_method, component);

    // get file path name
    char fnbuf[BUFFER_SIZE];
    char file_name[BUFFER_SIZE];
    component = strtok(NULL, " \r\n");
    sprintf(fnbuf, ".");
    strcat(fnbuf, component);
    if (strcmp(fnbuf, "./") == 0)
    {
        sprintf(file_name, "./index.html");
    }
    else
    {
        // replace % encodings in file name
        int j = 0;
        int fnbuf_len = strlen(fnbuf);
        for (int i = 0; i < fnbuf_len; i++, j++)
        {
            if (fnbuf[i] == '%' && fnbuf[i + 1] == '2' && fnbuf[i + 2] == '0')
            {
                file_name[j] = ' ';
                i += 2;
            }
            else if (fnbuf[i] == '%' && fnbuf[i + 1] == '2' && fnbuf[i + 2] == '5')
            {
                file_name[j] = '%';
                i += 2;
            }
            else
            {
                file_name[j] = fnbuf[i];
            }
        }
        file_name[j] = '\0';
    }

    // get http type
    char http_type[BUFFER_SIZE];
    component = strtok(NULL, " \r\n");
    strcpy(http_type, component);

    // get file type
    char content_type[BUFFER_SIZE];
    if (strstr(file_name, ".txt") != NULL)
    {
        sprintf(content_type, "Content-Type: text/plain; charset=UTF-8\r\n");
    }
    else if (strstr(file_name, ".html") != NULL || strstr(file_name, ".htm") != NULL)
    {
        sprintf(content_type, "Content-Type: text/html; charset=UTF-8\r\n");
    }
    else if (strstr(file_name, ".jpg") != NULL || strstr(file_name, ".jpeg") != NULL)
    {
        sprintf(content_type, "Content-Type: image/jpeg\r\n");
    }
    else
    {
        sprintf(content_type, "Content-Type: application/octet-stream\r\n");
    }

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    if (strstr(file_name, ".ts"))
    {
        proxy_remote_file(app, client_socket, file_name, http_type, content_type, buffer);
    }
    else
    {
        serve_local_file(client_socket, http_method, file_name, http_type, content_type);
    }
}

void serve_local_file(int client_socket, const char *method, const char *path, const char *http_type, const char *content_type)
{
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response

    // open file
    int fd;
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        // ERROR: file doesn't exist
        char fourzerofour[BUFFER_SIZE];
        char fourofour_response[BUFFER_SIZE];
        sprintf(fourofour_response, "<html><body>404 Not Found</body></html>");
        sprintf(fourzerofour, "%s 404 Not Found\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n",
                http_type, strlen(fourofour_response));
        send(client_socket, fourzerofour, strlen(fourzerofour), 0);
        send(client_socket, fourofour_response, strlen(fourofour_response), 0);
        return;
    }

    // get file info
    struct stat st_str;
    fstat(fd, &st_str);

    // get file length
    int flen = st_str.st_size;

    // get data from file
    char *data_buffer;
    if ((data_buffer = malloc(flen * sizeof(char) + 1)) == NULL)
    {
        // ERROR - malloc failed
        fprintf(stderr, "MEMORY ALLOCATION ERROR");
        exit(1);
    }
    memset(data_buffer, 0, flen + 1);

    int i = 0;
    if (read(fd, data_buffer, flen) < 0)
    {
        // ERROR - read failed
        fprintf(stderr, "READ ERROR");
        exit(1);
    }

    if (close(fd) < 0)
    {
        // ERROR - can't close file
        fprintf(stderr, "FILE CLOSING ERROR");
        exit(1);
    }

    char header_lines[BUFFER_SIZE];
    sprintf(header_lines, "%s 200 OK\r\n%sContent-Length: %d\r\nConnection: keep-alive\r\n\r\n",
            http_type, content_type, flen);

    // printf("%s\n", response); // print HTTP response

    send(client_socket, header_lines, strlen(header_lines), 0);
    send(client_socket, data_buffer, flen, 0);

    free(data_buffer);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *path, const char *http_type, const char *content_type, const char *request)
{
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    int new_socket;
    struct sockaddr_in server_addr;

    // creating socket
    new_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (new_socket < 0)
    {
        perror("SOCKET CREATION FAILED");
        exit(EXIT_FAILURE);
    }

    // setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(app->remote_port);
    server_addr.sin_addr.s_addr = inet_addr(app->remote_host);

    if (connect(new_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        // ERROR: connection to the remote server failed
        char bad_gateway[BUFFER_SIZE];
        char bad_gateway_response[BUFFER_SIZE];
        sprintf(bad_gateway_response, "<html><body>502 Bad Gateway</body></html>");
        sprintf(bad_gateway, "%s 502 Bad Gateway\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n",
                http_type, strlen(bad_gateway_response));
        send(client_socket, bad_gateway, strlen(bad_gateway), 0);
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        return;
    }

    if (send(new_socket, request, strlen(request), 0) < 0)
    {
        perror("SEND FAILED");
        exit(EXIT_FAILURE);
    }

    // forward binary data from server to client
    // can't process data as text for a video or any binary data
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = recv(new_socket, buffer, sizeof(buffer), 0)) > 0)
    {
        if (bytes_read <= 0)
        {
            // Connection closed or error
            printf("CONNECTION CLOSED OR ERROR");
            return;
        }

        send(client_socket, buffer, bytes_read, 0);
    }

    printf("all sent\n");
    // close(client_socket);
    // close(new_socket);
}