#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 10

static int SERVER_PORT = 4444;

int main()
{
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("[!] Failed to initialize Winsock.\n");
        return 1;
    }

    // Create a master socket
    SOCKET masterSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (masterSocket == INVALID_SOCKET)
    {
        printf("[!] Failed to create master socket.\n");
        WSACleanup();
        return 1;
    }

    // Bind the master socket to an address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(masterSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("[!] Failed to bind the master socket.\n");
        closesocket(masterSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(masterSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("[!] Failed to listen on the master socket.\n");
        closesocket(masterSocket);
        WSACleanup();
        return 1;
    }

    printf("[+] Server is listening on port %d...\n", SERVER_PORT);

    // Create a set of sockets
    fd_set readSockets;
    SOCKET maxSocket = masterSocket;
    std::vector<SOCKET> clientSockets;

    // Add the master socket to the set
    FD_ZERO(&readSockets);
    FD_SET(masterSocket, &readSockets);

    // Accept connections and handle data
    while (true) {
        // Wait for activity on any of the sockets
        fd_set tempReadSockets = readSockets;
        int activity = select(0, &tempReadSockets, nullptr, nullptr, nullptr);
        if (activity == SOCKET_ERROR) {
            printf("[!] select error.\n");
            closesocket(masterSocket);
            WSACleanup();
            return 1;
        }

        // Handle activity on the master socket (new connection)
        if (FD_ISSET(masterSocket, &tempReadSockets)) {
            struct sockaddr_in clientAddress;
            int clientAddressSize = sizeof(clientAddress);

            SOCKET newClientSocket = accept(masterSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
            if (newClientSocket == INVALID_SOCKET) {
                printf("[!] Failed to accept the client connection.\n");
                closesocket(masterSocket);
                WSACleanup();
                return 1;
            }

            printf("[+] New client connected: %d\n", newClientSocket);

            // Add the new client socket to the set
            FD_SET(newClientSocket, &readSockets);
            if (newClientSocket > maxSocket) {
                maxSocket = newClientSocket;
            }

            // Add the new client
            clientSockets.push_back(newClientSocket);
        }

        // Handle activity on client sockets (received data)
        for (int i = 0; i < clientSockets.size(); ++i) {
            SOCKET clientSocket = clientSockets[i];
            char processedMessageBuffer[1024];
            if (FD_ISSET(clientSocket, &tempReadSockets)) {
                int bytesRead = recv(clientSocket, processedMessageBuffer, sizeof(processedMessageBuffer) - 1, 0);
                if (bytesRead <= 0) {
                    printf("[+] Client disconnected: %d\n", clientSocket);
                    closesocket(clientSocket);
                    FD_CLR(clientSocket, &readSockets);
                    clientSockets.erase(clientSockets.begin() + i);
                    --i;
                }
                else {
                    processedMessageBuffer[bytesRead] = '\0';
                    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)processedMessageBuffer, -1, nullptr, 0, nullptr, nullptr);
                    std::wstring utf8Data(requiredSize, 0);
                    WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)processedMessageBuffer, -1, reinterpret_cast<LPSTR>(&utf8Data[0]), requiredSize, nullptr, nullptr);
                    printf("[+] Received data from client %d: %s\n", clientSocket, utf8Data);
                    utf8Data.clear();
                }
            }

            // zeromemory (winapi) / memset (c) doesn't seem to work here, so doing cleanup manually
            // fix in spare time
            for (size_t x = 0; x < sizeof(processedMessageBuffer); x++) {
                processedMessageBuffer[x] = 0;
            }

            // Also, can consider creating separate recv buffer for each client and using them accordingly to handle communication
        }
    }

    // Cleanup
    closesocket(masterSocket);
    WSACleanup();

    return 0;
}

void printBuffer(char* buffer) {
    printf("Buffer: ");
    for (size_t x = 0; x < 1024; x++) {
        printf("%d ", buffer[x]);
    }
    printf("\n");
}