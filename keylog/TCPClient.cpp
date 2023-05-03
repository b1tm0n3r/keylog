#include "TCPClient.h"

TCPClient::TCPClient(const std::string& serverIp, int serverPort) {
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock.\n");
    }

    // Create a socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Failed to create socket.\n");
        WSACleanup();
    }

    // Connect to the server
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = inet_addr(serverIp.c_str());

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Failed to connect to the server.\n");
        closesocket(clientSocket);
        WSACleanup();
    }
}

TCPClient::~TCPClient() {
    closesocket(clientSocket);
    WSACleanup();
}

int TCPClient::sendWideStringBuffer(const std::wstring& buffer) {
    std::string utf8Data;
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, buffer.c_str(), -1, nullptr, 0, nullptr, nullptr);
    int bytesSent = send(clientSocket, utf8Data.c_str(), utf8Length, 0);

    if (bytesSent == SOCKET_ERROR) {
        printf("Failed to send data to the server.\n");
    }
    else {
        printf("Sent %d bytes to the server.\n", bytesSent);
    }

    return bytesSent;
}