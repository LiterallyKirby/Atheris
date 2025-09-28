#include "Network.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <cstring>
#include <iostream>

NetworkClient::NetworkClient() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    socketHandle = nullptr;
}

NetworkClient::~NetworkClient() {
#ifdef _WIN32
    if (socketHandle) closesocket((SOCKET)(uintptr_t)socketHandle);
    WSACleanup();
#else
    if (socketHandle) close((int)(uintptr_t)socketHandle);
#endif
}

bool NetworkClient::connectToServer(const std::string& ip, uint16_t port) {
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
#endif
    if (sock < 0) {
        perror("Socket creation failed");
        return false;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
#ifdef _WIN32
    InetPton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
#else
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
#endif

    socketHandle = reinterpret_cast<void*>(static_cast<uintptr_t>(sock));
    return true;
}

bool NetworkClient::sendData(const void* data, size_t size) {
#ifdef _WIN32
    SOCKET sock = (SOCKET)(uintptr_t)socketHandle;
#else
    int sock = (int)(uintptr_t)socketHandle;
#endif
    return sendto(sock, static_cast<const char*>(data), size, 0,
                  (struct sockaddr*)&serverAddr, sizeof(serverAddr)) >= 0;
}

std::vector<uint8_t> NetworkClient::receiveData() {
    char buffer[65536];
#ifdef _WIN32
    SOCKET sock = (SOCKET)(uintptr_t)socketHandle;
#else
    int sock = (int)(uintptr_t)socketHandle;
#endif
    ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
    if (len <= 0) return {};
    return std::vector<uint8_t>(buffer, buffer + len);
}

NetworkServer::NetworkServer(uint16_t port) : port(port), socketHandle(nullptr) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

NetworkServer::~NetworkServer() {
#ifdef _WIN32
    if (socketHandle) closesocket((SOCKET)(uintptr_t)socketHandle);
    WSACleanup();
#else
    if (socketHandle) close((int)(uintptr_t)socketHandle);
#endif
}

bool NetworkServer::start() {
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
#endif
    if (sock < 0) {
        perror("Socket creation failed");
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        return false;
    }

    socketHandle = reinterpret_cast<void*>(static_cast<uintptr_t>(sock));
    return true;
}

std::vector<uint8_t> NetworkServer::receiveFromClient(sockaddr_in& clientAddr) {
    char buffer[65536];
    socklen_t addrLen = sizeof(clientAddr);

#ifdef _WIN32
    SOCKET sock = (SOCKET)(uintptr_t)socketHandle;
#else
    int sock = (int)(uintptr_t)socketHandle;
#endif

    ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0,
                           (struct sockaddr*)&clientAddr, &addrLen);
    if (len <= 0) return {};
    return std::vector<uint8_t>(buffer, buffer + len);
}

bool NetworkServer::sendToClient(const void* data, size_t size, const sockaddr_in& clientAddr) {
#ifdef _WIN32
    SOCKET sock = (SOCKET)(uintptr_t)socketHandle;
#else
    int sock = (int)(uintptr_t)socketHandle;
#endif
    return sendto(sock, static_cast<const char*>(data), size, 0,
                  (struct sockaddr*)&clientAddr, sizeof(clientAddr)) >= 0;
}
