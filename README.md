## Project Overview

This project is a basic HTTP server, designed to handle requests by either serving local files or proxying remote files. The server implements logic to serve requests based on their content and the presence of files locally.

## File Structure

- **server.c**: The main server code containing the logic for handling HTTP requests, parsing arguments, and serving/proxying content.
- **makefile**: The build system for compiling and cleaning the project.
- **index.html**: A video player page used for testing the serving of local HTML content. It instructs the browser to request the output.m3u8 video manifest.
- **output.m3u8**: A video manifest used for testing the serving of local content. It contains information about video chunks and instructs the browser to request `.ts` video chunks, which are hosted on our remote video server.

## Build Instructions

To build the project, ensure you have `gcc` installed on your system. Navigate to the directory containing the `server.c` and `makefile`, then use the following commands:

- To compile the project:
    ```bash
    make
    ```
  
- To clean the build (remove the compiled output):
    ```bash
    make clean
    ```

## Running the Server

After compiling, you can run the server using the following command:

```bash
./server [-b local_port] [-r remote_host] [-p remote_port]
```

### Command-line Arguments:

- `-b local_port`: Specify the local port on which the HTTP server will run. Defaults to `8081`.
- `-r remote_host`: Specify the remote host's address for proxying. Defaults to `131.179.176.34`.
- `-p remote_port`: Specify the remote port for proxying. Defaults to `5001`.

## Project Features

Features implemented in this project:

1. **Request Parsing**: Parse the header of the incoming HTTP request to extract essential fields like the requested file name.
2. **Local File Serving**: Implement the logic for serving files that exist locally.
3. **Proxying Remote Files**: If a file does not exist locally, proxy the request to another server.
4. **Handling Errors**: Implement proper error responses.

## Notes

This is a basic server that assumes the HTTP request header is small enough to be read once as a whole, which may not be ideal in a real-world scenario but suffices for this project's scope.
