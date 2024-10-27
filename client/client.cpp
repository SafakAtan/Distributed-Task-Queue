#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

void fetch_tasks(int client_socket) {
    while (true) {
        // Send a request to get a task
        const char* request = "GET_TASK";
        send(client_socket, request, strlen(request), 0);
        
        char buffer[1024] = {0};
        int valread = read(client_socket, buffer, 1024);

        if (valread > 0) {
            std::string response(buffer);

            if (response == "NO_TASK") {
                std::cout << "No tasks available. Waiting..." << std::endl;
            } else {
                std::cout << "Received task: " << response << std::endl;

                // Simulate task processing
                std::this_thread::sleep_for(std::chrono::seconds(3));

                // Notify server that task is complete
                const char* complete = "COMPLETE_TASK";
                send(client_socket, complete, strlen(complete), 0);
                std::cout << "Completed task: " << response << std::endl;
            }
        } else {
            std::cerr << "Failed to receive a response from the server." << std::endl;
            break;
        }

        // Wait a moment before checking for a new task
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main() {
    int client_socket;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed." << std::endl;
        return -1;
    }

    server = gethostbyname("server");  // Resolve server address
    if (!server) {
        std::cerr << "No such host." << std::endl;
        return -1;
    }

    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed." << std::endl;
        return -1;
    }

    std::cout << "Connected to the server." << std::endl;

    // Start a thread to continuously fetch tasks from the server
    std::thread taskThread(fetch_tasks, client_socket);
    taskThread.join();

    close(client_socket);
    return 0;
}
