#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

std::queue<std::string> taskQueue; // Queue to hold tasks
std::vector<int> clients; // List of connected clients
std::mutex queueMutex, clientsMutex;

void handle_client(int client_socket) {
    char buffer[1024];

    while (true) {
        // Wait for request from client
        int valread = read(client_socket, buffer, 1024);
        if (valread <= 0) {
            std::cerr << "Client disconnected." << std::endl;
            close(client_socket);
            break;
        }
        buffer[valread] = '\0';

        // Handle task request
        std::string command(buffer);
        if (command == "GET_TASK") {
            std::lock_guard<std::mutex> lock(queueMutex);

            if (!taskQueue.empty()) {
                std::string task = taskQueue.front();
                taskQueue.pop();
                send(client_socket, task.c_str(), task.size(), 0);
            } else {
                const char* noTask = "NO_TASK";
                send(client_socket, noTask, strlen(noTask), 0);
            }
        } else if (command == "COMPLETE_TASK") {
            // Additional code here to handle task completion (optional)
            std::cout << "Task completed by client." << std::endl;
        }
    }

    // Remove client from the list
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation failed." << std::endl;
        return -1;
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Set socket options failed." << std::endl;
        return -1;
    }

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Binding failed." << std::endl;
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listening failed." << std::endl;
        return -1;
    }

    std::cout << "Server is listening on port 8080..." << std::endl;

    // Accept connections in a loop
    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        std::cout << "New client connected." << std::endl;
        clients.push_back(client_socket);

        // Create a new thread to handle each client
        std::thread(handle_client, client_socket).detach();
    }

    return 0;
}
