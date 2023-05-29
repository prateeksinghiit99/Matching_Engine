#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

constexpr int MAX_BUFFER_SIZE = 1024;


using namespace std;
int main() {
    // Create a TCP socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Failed to create socket\n";
        return 1;
    }

    // Set up server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080); // Server port
    if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) <= 0) {
        cerr << "Invalid address\n";
        close(clientSocket);
        return 1;
    }

    // Connect to the server
    if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Failed to connect to server\n";
        close(clientSocket);
        return 1;
    }

    cout << "Connected to server\n";

    // Send messages to the server
    string message;
    while (getline(cin, message)) {
        if (message.empty()) {
            continue;
        }

        // Send message to the server
        send(clientSocket, message.c_str(), message.size(), 0);

        // Receive response from the server
        char buffer[MAX_BUFFER_SIZE];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            cerr << "Server disconnected\n";
            break;
        }

        // Print server response
        cout << "Server response: " << string(buffer, bytesRead) << endl;
    }

    // Close the socket
    close(clientSocket);

    return 0;
}

