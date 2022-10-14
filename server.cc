#include <iostream>
#include <fstream>
#include <memory>
#include <chrono>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MB_IN_BYTES 1048576

void WriteToSocket(int sock_fd, char* buffer, int count) {
    int bytes_written = 0;

    while ((bytes_written = write(sock_fd, buffer, count)) != count) {
        buffer += bytes_written;
        count -= bytes_written;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " [PORT] [FILE_PATH] [BUFFER_SIZE=1024] [PROGRESS_DISPLAY_INTERVAL=10]" << std::endl;
        exit(1);
    }
 
    int listen_port = 1234;
    if (argc > 1)
        listen_port = atoi(argv[1]);
 
    int sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_server < 0) {
        std::cerr << "Error creating listen socket: " << strerror(errno) << std::endl;
        exit(1);
    }

    int sock_option = 1;
    setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, &sock_option, sizeof(sock_option));
 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(listen_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
 
    int res = bind(sock_server, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if (res < 0) {
        std::cerr << "Error binding listen socket: " << strerror(errno) << std::endl;
        exit(1);
    }
 
    struct linger linger_opt = { 1, 0 };
    setsockopt(sock_server, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));
 
    res = listen(sock_server, 1);
    if (res < 0) {
        std::cerr << "Error listening for a connection: " << strerror(errno) << std::endl;
        exit(1);
    }
 
    struct sockaddr_in client_addr;
    socklen_t peeraddr_len;
    int sock_client = accept(sock_server, (struct sockaddr*) &client_addr, &peeraddr_len);
    if (sock_client < 0) {
        std::cerr << "Failed to accept client connection: " << strerror(errno) << std::endl;
        exit(1);
    }

    res = close(sock_server);
    if (res < 0) {
        std::cerr << "Error closing listen socket: " << strerror(errno) << std::endl;

        res = close(sock_client);
        if (res < 0)
            std::cerr << "Error closing client socket: " << strerror(errno) << std::endl;

        exit(1);
    }

    const unsigned short kBufferSize = (argc >= 4 ? atoi(argv[3]) : 1024);

    std::ifstream input_stream(argv[2], std::ifstream::binary);
    auto read_buffer = std::make_unique<char[]>(kBufferSize);

    unsigned long long total_sent = 0;
    short show_progress = 0;
    int progress_interval = MB_IN_BYTES * (argc >= 5 ? atoi(argv[4]) : 10) / kBufferSize;

    auto start = std::chrono::high_resolution_clock::now();

    while (input_stream.peek() != EOF) {
        input_stream.read(read_buffer.get(), kBufferSize);
        int bytes_read = input_stream.gcount();
        WriteToSocket(sock_client, read_buffer.get(), bytes_read);

        total_sent += bytes_read;
        if (show_progress == progress_interval - 1)
            std::cout << "Sent: " << total_sent / MB_IN_BYTES << " (MB)" << std::endl;
        show_progress = ++show_progress % progress_interval;
    }

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    std::cout << "Execution time: " << duration.count() << " (μs), " << "Total sent: " 
    << total_sent << " (bytes), Buffer size: " << kBufferSize << " (bytes)" << std::endl;

    input_stream.close();

    shutdown(sock_client, SHUT_WR);
    read(sock_client, read_buffer.get(), 1);
    close(sock_client);

    return 0;
}