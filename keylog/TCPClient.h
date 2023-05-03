#pragma once

#include <Windows.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class TCPClient {
private:
    WSADATA wsaData;
    SOCKET clientSocket;

public:
    TCPClient(const std::string& serverIP, int serverPort);
    int sendWideStringBuffer(const std::wstring& buffer);
    ~TCPClient();
};