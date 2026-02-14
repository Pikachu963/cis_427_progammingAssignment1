#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // 1. Create the socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(4552); // Must match your server's port

    // 2. Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "\nInvalid address/ Address not supported \n";
        return -1;
    }

    // 3. Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\nConnection Failed \n";
        return -1;
    }

    std::cout << "Connected to the Stock Server!" << std::endl;
    std::cout << "Commands: BALANCE, BUY <symbol> <qty>, SELL <symbol> <qty>, LIST, QUIT, SHUTDOWN" << std::endl;

    // 4. Communication Loop
    while (true) {
        std::cout << "> ";
        std::string command;
        std::getline(std::cin, command);

        if (command == "QUIT") break;

        // Send command to server
        send(sock, command.c_str(), command.length(), 0);

        // Receive response from server
        memset(buffer, 0, 1024);
        int valread = read(sock, buffer, 1024);
        
        if (valread > 0) {
            std::cout << buffer << std::endl;
        } else {
            std::cout << "Server disconnected." << std::endl;
            break;
        }
    }

    close(sock);
    return 0;
}