#include "Client.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cmath>

Client::Client() {}
Client::~Client() { disconnect(); }

bool Client::connectToServer(const std::string& ip, uint16_t port) {
    serverIP = ip; serverPort = port;
    return connectTcp();
}

bool Client::connectTcp() {
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket < 0) { perror("socket"); return false; }

    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP.c_str(), &addr.sin_addr) <= 0) { perror("inet_pton"); return false; }
    if (connect(tcpSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); close(tcpSocket); tcpSocket = -1; return false; }

    connected = true;
    std::cout << "Client: connected to server\n";
    return true;
}

void Client::disconnect() {
    if (tcpSocket != INVALID_SOCKET_VALUE) { close(tcpSocket); tcpSocket = INVALID_SOCKET_VALUE; }
    connected = false;
}

ChunkData Client::requestChunk(uint32_t x, uint32_t y, uint32_t z) {
    ChunkData out{};
    if (!connected) {
        std::cerr << "Client not connected\n";
        return out;
    }
    uint32_t coords[3] = {x,y,z};
    if (send(tcpSocket, coords, sizeof(coords), 0) != (ssize_t)sizeof(coords)) {
        std::cerr << "Failed to send chunk request\n"; return out;
    }
    // receive header
    struct ChunkPacketHeader {
        uint32_t chunkX, chunkY, chunkZ;
        uint16_t width, height, depth;
    } header;
    ssize_t got = recv(tcpSocket, &header, sizeof(header), MSG_WAITALL);
    if (got != (ssize_t)sizeof(header)) {
        std::cerr << "Failed to receive header got=" << got << std::endl;
        return out;
    }
    out.chunkX = header.chunkX; out.chunkY = header.chunkY; out.chunkZ = header.chunkZ;
    out.width = header.width; out.height = header.height; out.depth = header.depth;
    
    // FIXED: Server now sends 2 bytes per block (type + ramp direction)
    size_t numBlocks = (size_t)out.width * out.height * out.depth;
    size_t expectedBytes = numBlocks * 2; // 2 bytes per block now
    out.blocks.resize(expectedBytes);
    
    size_t recvTotal = 0;
    uint8_t* dest = out.blocks.data();
    while (recvTotal < expectedBytes) {
        ssize_t r = recv(tcpSocket, dest + recvTotal, expectedBytes - recvTotal, 0);
        if (r <= 0) { 
            std::cerr << "Failed to receive chunk data, got " << recvTotal << "/" << expectedBytes << " bytes\n"; 
            return ChunkData{}; 
        }
        recvTotal += (size_t)r;
    }
    std::cout << "Client: received chunk " << out.chunkX << "," << out.chunkZ 
              << " blocks=" << numBlocks << " bytes=" << expectedBytes << "\n";
    return out;
}


ChunkData Client::getChunkForPosition(float x, float y, float z) {
    constexpr int CHUNK_SIZE = 16; // Or whatever your chunk size is

    int chunkX = static_cast<int>(std::floor(x / CHUNK_SIZE));
    int chunkY = static_cast<int>(std::floor(y / CHUNK_SIZE));
    int chunkZ = static_cast<int>(std::floor(z / CHUNK_SIZE));

    std::tuple<int,int,int> chunkCoord = {chunkX, chunkY, chunkZ};

    // If chunk already loaded → return it
    if (loadedChunks.find(chunkCoord) != loadedChunks.end()) {
        return loadedChunks[chunkCoord];
    }

    // Otherwise request from server
    ChunkData chunk = requestChunk(chunkX, chunkY, chunkZ);

    loadedChunks[chunkCoord] = chunk; // cache it
    return chunk;
}
