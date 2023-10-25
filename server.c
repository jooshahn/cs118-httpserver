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

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

#define BINARY_FILE 0
#define TXT_FILE 1
#define HTML_FILE 2
#define JPG_FILE 3

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

void parse_args(int argc, char *argv[], struct server_app *app);
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
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

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
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

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
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

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    printf("%s", request); // print out of HTTP request message from server

    // get request method
    char *curr = request;
    char method[5];
    int i;
    for (i = 0; curr[i] != ' '; i++) {
        method[i] = curr[i];
    }
    method[i] = '\0';
    i++;

    // get file path
    char file_name[] = "./index.html";
    if (curr[i + 1] != ' ') {
        int j;
        for (j = 1; curr[i] != ' '; i++, j++) {
            file_name[j] = curr[i];
        }
        file_name[j] = '\0';
    }
    i++;

    // // get HTTP protocol
    // while (curr[i] != '/') {
    //     i++;
    // }
    // i++;
    // char http_prot[4];
    // for (int k = 0; k < 3; i++, k++) {
    //     http_prot[k] = curr[i];
    // }
    // http_prot[3] = '\0';

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    // if (need_proxy(...)) {
    //    proxy_remote_file(app, client_socket, file_name);
    // } else {
    serve_local_file(client_socket, file_name);
    //}
}

void serve_local_file(int client_socket, const char *path) {
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

    // get file type DOESNT WORK RN
    // const char *ftype = path;
    // int i = 0;
    // while (ftype[i] != '.' && ftype[i] != 0) {
    //     i++;
    // }
    // int file_type = BINARY_FILE;
    // if (ftype[i] == '.') {
    //     i++;
    //     char* fexten;
    //     strcpy(fexten, &ftype[i]);
    //     if (strcmp(fexten, "txt") == 0) {
    //         file_type = TXT_FILE;
    //     }
    //     else if (strcmp(fexten, "html") == 0) {
    //         file_type = HTML_FILE;
    //     }
    //     else if (strcmp(fexten, "jpg") == 0) {
    //         file_type = JPG_FILE;
    //     }
    // }
    // printf("FILE TYPE EXTENSTION: %d\n", file_type);

    // open file
    FILE* fptr;
    if ((fptr = fopen(path, "rb")) == NULL) {
        // ERROR - can't open file
        printf("FILE OPENING ERROR");
        exit(1);
    }

    // get file length
    fseek(fptr, 0, SEEK_END);
    long flen = ftell(fptr);
    rewind(fptr);
    
    // get data from file
    char* data_buffer;
    if ((data_buffer = malloc((flen) * sizeof(char))) == NULL) {
        // ERROR - malloc failed
        printf("MEMORY ALLOCATION ERROR");
        exit(1);
    }
    // while (fread(data_buffer, flen, sizeof(char), fptr) != 0) {
    //     // keep reading
    // }
    unsigned int i = 0;
    while (fread(&data_buffer[i], 1, 1, fptr) == 1) {
        i++;
    }
    printf("file data: %s\n", data_buffer);
    fclose(fptr);

    char *filelen;
    sprintf(filelen, "%ld", flen);

    char* response;
    if ((response = malloc(BUFFER_SIZE)) == NULL) {
        // ERROR - malloc failed
        printf("MEMORY ALLOCATION ERROR");
        exit(1);
    }

    char header_lines[] = 
                    "HTTP/1.1 200 OK\r\n"
                    // "Content-Type: application/octet-stream\r\n"
                    "Content-Type: text/plain; charset=UTF-8\r\n"
                    "Content-Type: text/html; charset=UTF-8\r\n"
                    "Content-Type: image/jpeg\r\n"
                    "Content-Type: multipart/form-data\r\n"
                    "Content-Length: ";
    char temp[] = "\r\n\r\n";

    strcat(response, header_lines);
    strcat(response, filelen);
    strcat(response, temp);
    strcat(response, data_buffer);
    // strcat(response, "this is a sample test file");

    printf("%s", response);  // print HTTP response

    send(client_socket, response, strlen(response), 0);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    send(client_socket, response, strlen(response), 0);
}