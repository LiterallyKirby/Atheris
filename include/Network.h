#pragma once

#include <vector>
#include <string>
#include <cstdint>


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool connectToServer(const std::string& ip, uint16_t port);
    bool sendData(const void* data, size_t size);
    std::vector<uint8_t> receiveData();

private:
    void* socketHandle;  // platform-specific socket
    sockaddr_in serverAddr;
};

class NetworkServer {
public:
    NetworkServer(uint16_t port);
    ~NetworkServer();

    bool start();
    std::vector<uint8_t> receiveFromClient(sockaddr_in& clientAddr);
    bool sendToClient(const void* data, size_t size, const sockaddr_in& clientAddr);

private:
    void* socketHandle;
    uint16_t port;
};
