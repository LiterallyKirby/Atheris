#include "Server.h"
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>

#ifdef _WIN32
// Windows sockets omitted for brevity
#else
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

Server::Server(uint16_t port_) : port(port_), running(false) {
    udpSocket.store(INVALID_SOCKET_VALUE);
    tcpSocket.store(INVALID_SOCKET_VALUE);
    chunkManager = std::make_unique<ChunkManager>(16, 4);
}

Server::~Server() {
    stop();
}

void Server::start() {
    running = true;
    serverThread = std::thread(&Server::run, this);
}

void Server::stop() {
    running = false;
    if (serverThread.joinable()) serverThread.join();

    auto us = udpSocket.load();
    auto ts = tcpSocket.load();
    if (us != INVALID_SOCKET_VALUE) close(us);
    if (ts != INVALID_SOCKET_VALUE) close(ts);
    udpSocket.store(INVALID_SOCKET_VALUE);
    tcpSocket.store(INVALID_SOCKET_VALUE);
}

socket_t Server::createNonBlockingSocket(int type, int protocol) {
    int sock = socket(AF_INET, type, protocol);
    if (sock < 0) {
        perror("socket");
        return INVALID_SOCKET_VALUE;
    }
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return sock;
}

void Server::run() {
    socket_t udp_sock = createNonBlockingSocket(SOCK_DGRAM, 0);
    if (udp_sock == INVALID_SOCKET_VALUE) return;
    udpSocket.store(udp_sock);

    socket_t tcp_sock = createNonBlockingSocket(SOCK_STREAM, 0);
    if (tcp_sock == INVALID_SOCKET_VALUE) {
        close(udp_sock);
        udpSocket.store(INVALID_SOCKET_VALUE);
        return;
    }
    tcpSocket.store(tcp_sock);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind udp");
        return;
    }
    if (bind(tcp_sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind tcp");
        return;
    }
    if (listen(tcp_sock, 8) < 0) {
        perror("listen");
        return;
    }

    std::cout << "Server running on port " << port << " (UDP+TCP)\n";

    std::vector<socket_t> tcpClients;

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(udp_sock, &readfds);
        FD_SET(tcp_sock, &readfds);
        socket_t maxfd = std::max(udp_sock, tcp_sock);
        for (auto c : tcpClients) { FD_SET(c, &readfds); maxfd = std::max(maxfd, c); }

        struct timeval tv{0, 100000}; // 100ms
        int res = select((int)maxfd + 1, &readfds, nullptr, nullptr, &tv);
        if (res < 0) {
            if (errno == EINTR) continue;
            break;
        }

        // update server-side chunk manager around player if available
        if (player) {
            // compute player's chunk coords and ensure nearby chunks exist
            glm::vec3 chunkCoords = player->getChunkCoordinates(chunkManager->getChunkSize());
            int cx = (int)chunkCoords.x;
            int cz = (int)chunkCoords.z;
            int rd = chunkManager->getRenderDistance();
            for (int dx=-rd; dx<=rd; ++dx)
                for (int dz=-rd; dz<=rd; ++dz)
                    chunkManager->loadChunk(cx+dx, cz+dz);
        }

        if (res == 0) continue;

        if (FD_ISSET(tcp_sock, &readfds)) {
            sockaddr_in clientAddr{};
            socklen_t addrLen = sizeof(clientAddr);
            int clientSock = accept(tcp_sock, (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSock >= 0) {
                tcpClients.push_back(clientSock);
                std::cout << "New TCP client\n";
            }
        }

        if (FD_ISSET(udp_sock, &readfds)) {
            handleUdpRequest(udp_sock);
        }

        for (auto it = tcpClients.begin(); it != tcpClients.end();) {
            socket_t s = *it;
            if (FD_ISSET(s, &readfds)) {
                if (!handleTcpRequest(s)) {
                    close(s);
                    it = tcpClients.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    // cleanup
    for (auto c : tcpClients) close(c);
    close(udp_sock);
    close(tcp_sock);
    udpSocket.store(INVALID_SOCKET_VALUE);
    tcpSocket.store(INVALID_SOCKET_VALUE);
    std::cout << "Server stopped\n";
}

void Server::handleUdpRequest(socket_t sock) {
    char buf[1024];
    sockaddr_in caddr{};
    socklen_t len = sizeof(caddr);
    ssize_t r = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&caddr, &len);
    if (r <= 0) return;
    const char* resp = "UDP_OK";
    sendto(sock, resp, (int)strlen(resp), 0, (struct sockaddr*)&caddr, len);
}

bool Server::handleTcpRequest(socket_t clientSock) {
    // Expect 3x uint32_t chunk coords from client
    uint32_t coords[3];
    ssize_t got = recv(clientSock, coords, sizeof(coords), 0);
    if (got != (ssize_t)sizeof(coords)) {
        // may be client closed or sent partial -> treat as disconnect
        return false;
    }
    uint32_t cx = coords[0], cy = coords[1], cz = coords[2];
    std::cout << "Server: chunk request " << cx << "," << cy << "," << cz << "\n";

    // Ensure chunk exists on server
    chunkManager->loadChunk((int)cx, (int)cz);
    Chunk* ch = chunkManager->getChunk((int)cx, (int)cz);
    if (!ch) return false;

    struct ChunkPacketHeader {
        uint32_t chunkX, chunkY, chunkZ;
        uint16_t width, height, depth;
    } header;

    header.chunkX = cx;
    header.chunkY = cy;
    header.chunkZ = cz;
    header.width = (uint16_t)ch->getWidth();
    header.height = (uint16_t)ch->getHeight();
    header.depth = (uint16_t)ch->getDepth();

    // send header
    if (send(clientSock, &header, sizeof(header), 0) != (ssize_t)sizeof(header)) return false;

    // FIXED: Send both block type and ramp direction as packed bytes
    const auto& blocks = ch->getBlocks();
    std::vector<uint8_t> packedData;
    packedData.reserve(blocks.size() * 2); // 2 bytes per block
    
    // Pack block type and ramp direction into separate bytes
    for (const auto& block : blocks) {
        packedData.push_back(static_cast<uint8_t>(block.type));
        packedData.push_back(static_cast<uint8_t>(block.ramp));
    }

    // send packed data
    size_t totalBytes = packedData.size();
    const uint8_t* ptr = packedData.data();
    size_t sent = 0;
    while (sent < totalBytes) {
        ssize_t s = send(clientSock, (const char*)(ptr + sent), (int)(totalBytes - sent), 0);
        if (s <= 0) return false;
        sent += (size_t)s;
    }
    
    std::cout << "Server: sent chunk data (" << totalBytes << " bytes = " << blocks.size() << " blocks)\n";
    return true;
}
