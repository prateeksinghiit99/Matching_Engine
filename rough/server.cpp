#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <vector>

constexpr int MAX_BUFFER_SIZE = 1024;

// Structure to hold client connection information
struct Client {
    int socket;
    sockaddr_in address;
};


using namespace std;
int main() {
    vector<Client> clients;
    vector<string> messages;

    // Create a TCP socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Failed to create socket\n";
        return 1;
    }

    // Set up server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080); // Server port

    // Bind socket to the server address
    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Failed to bind socket\n";
        close(serverSocket);
        return 1;
    }

    // Listen for client connections
    if (listen(serverSocket, SOMAXCONN) == -1) {
        cerr << "Failed to listen\n";
        close(serverSocket);
        return 1;
    }

    cout << "Server listening on port 8080\n";

    while (true) {
        // Accept a new client connection
        sockaddr_in clientAddress{};
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket == -1) {
            cerr << "Failed to accept client connection\n";
            continue;
        }

        // Add the client to the client list
        clients.push_back({clientSocket, clientAddress});

        cout << "New client connected\n";

        // Handle client messages
        char buffer[MAX_BUFFER_SIZE];
        while (true) {
            // Receive message from the client
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) {
                cout << "Client disconnected\n";
                break;
            }

            // Store the message
            string message(buffer, bytesRead);
            messages.push_back(message);

            // Send a response to the client
            string response = "Message received: " + message;
            send(clientSocket, response.c_str(), response.size(), 0);
        }

        // Close the client socket
        close(clientSocket);
    }

    // Close the server socket
    close(serverSocket);

    return 0;
}

