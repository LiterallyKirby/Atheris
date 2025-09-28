#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "ChunkManager.h"
#include "Constants.h"
using socket_t = int;
#include <map>

struct ChunkData {
    uint32_t chunkX, chunkY, chunkZ;
    uint16_t width, height, depth;
    std::vector<uint8_t> blocks;
};

class Client {
public:
    Client();
    ~Client();

    bool connectToServer(const std::string& ip, uint16_t port);
    void disconnect();

    // Request a single chunk; will block until received or error -> returns empty on failure
    ChunkData requestChunk(uint32_t x, uint32_t y, uint32_t z);
  ChunkData getChunkForPosition(float x, float y, float z);

private:
  std::map<std::tuple<int,int,int>, ChunkData> loadedChunks;
    int currentChunkX = INT32_MIN;
    int currentChunkY = INT32_MIN;
    int currentChunkZ = INT32_MIN;
    bool connectTcp();
    void cleanup();

    std::string serverIP;
    uint16_t serverPort;
    socket_t tcpSocket = INVALID_SOCKET_VALUE;
    bool connected = false;
};
