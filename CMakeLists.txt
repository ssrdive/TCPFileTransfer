cmake_minimum_required (VERSION 2.8.11)
project (CLIENT_SERVER_FILE_TRANSFER)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable (Server server.cc)
add_executable (Client client.cc)