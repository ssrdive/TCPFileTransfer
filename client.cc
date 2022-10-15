#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MB_IN_BYTES 1048576

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " [SERVER] [PORT] [FILE_PATH] [BUFFER_SIZE=1024] [PROGRESS_DISPLAY_INTERVAL=10]" << std::endl;
        exit(1);
    }
 
    int sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_server < 0) {
        std::cerr << "Error creating client socket: " << strerror(errno) << std::endl;
        exit(1);
    }
 
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
 
    char* server_host = (char*) "localhost";
    if (argc > 1)
        server_host = argv[1];
 
    struct hostent *host = gethostbyname(server_host);
    if (host == NULL) {
        std::cerr << "Error resolving host: " << strerror(errno) << std::endl;
        exit(1);
    }
 
    server_addr.sin_family = AF_INET;
    short server_port = 1234;
    if (argc >= 3)
        server_port = (short) atoi(argv[2]);
    server_addr.sin_port = htons(server_port);

    memmove(&(server_addr.sin_addr.s_addr), host->h_addr_list[0], 4);

    int res = connect(sock_server, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if (res < 0) {
        std::cerr << "Error connecting to server: " << strerror(errno) << std::endl;
        exit(1);
    }

    const unsigned short kBufferSize = (argc >= 5 ? atoi(argv[4]) : 1024);
 
    std::ofstream output_stream(argv[3], std::ofstream::binary);
    auto read_buffer = std::make_unique<char[]>(kBufferSize);

    unsigned long long total_read = 0;
    short show_progress = 0;
    int progress_interval = MB_IN_BYTES * (argc >= 6 ? atoi(argv[5]) : 10) / kBufferSize;

    auto start = std::chrono::high_resolution_clock::now();

    int bytes_read = 0;
    while ((bytes_read = read(sock_server, read_buffer.get(), kBufferSize)) > 0) {
        output_stream.write(read_buffer.get(), bytes_read);

        total_read += bytes_read;
        if (show_progress == progress_interval - 1)
            std::cout << "Read: " << total_read / MB_IN_BYTES << " (MB)" << std::endl;
        show_progress = ++show_progress % progress_interval;
    }

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    std::cout << "Execution time: " << duration.count() << " (μs), " << "Total received: " 
    << total_read << " (bytes), Buffer size: " << kBufferSize << " (bytes)" << std::endl;

    output_stream.close();

    shutdown(sock_server, SHUT_RDWR);
    close(sock_server);

    return 0;
}
