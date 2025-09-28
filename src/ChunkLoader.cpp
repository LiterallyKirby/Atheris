
#include "ChunkLoader.h"

#include <chrono>
#include <iostream>
#include <cmath>

ChunkLoader::ChunkLoader(Client* client_, Player* player_, int chunkSize_, int renderDistance_)
    : client(client_), player(player_), chunkSize(chunkSize_), renderDistance(renderDistance_), running(false)
{
}

ChunkLoader::~ChunkLoader() {
    stop();
}

void ChunkLoader::start() {
    if (running.exchange(true)) return; // already running
    worker = std::thread(&ChunkLoader::threadMain, this);
}

void ChunkLoader::stop() {
    if (!running.exchange(false)) return;
    if (worker.joinable()) worker.join();
}

static glm::ivec2 playerChunkIndex(const Player& p, int chunkSize) {
    glm::vec3 c = p.getChunkCoordinates(chunkSize);
    // we want integer chunk indices (each chunk represents chunkSize world units)
    int cx = (int)std::floor(c.x);
    int cz = (int)std::floor(c.z);
    return glm::ivec2(cx, cz);
}

void ChunkLoader::threadMain() {
    // Simple background loop: query the player's chunk, then request nearby chunks not yet loaded.
    while (running) {
        if (!client || !player) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        glm::ivec2 pChunk = playerChunkIndex(*player, chunkSize);

        for (int dz = -renderDistance; dz <= renderDistance && running; ++dz) {
            for (int dx = -renderDistance; dx <= renderDistance && running; ++dx) {
                int cx = pChunk.x + dx;
                int cz = pChunk.y + dz; // ivec2: x,y

                std::pair<int,int> keyPair{cx, cz};

                {
                    // quick check + mark pending
                    std::lock_guard<std::mutex> lock(mtx);
                    ChunkKey ck{cx, cz};
                    if (loadedChunks.find(ck) != loadedChunks.end()) continue;
                    if (pendingRequests.count(keyPair)) continue;
                    pendingRequests.insert(keyPair);
                }

                // do network + mesh build outside lock
                requestAndStoreChunk(cx, cz);

                // slight throttle so we don't spam the server
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // Sleep a bit between sweeps to avoid CPU burn
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void ChunkLoader::requestAndStoreChunk(int chunkX, int chunkZ) {
    // IMPORTANT: your Client::requestChunk signature must be callable from this thread.
    // If your client is not thread-safe, ensure only the loader thread uses it.
    auto chunkData = client->requestChunk((uint32_t)chunkX, 0u, (uint32_t)chunkZ);

    if (chunkData.blocks.empty()) {
        std::cerr << "ChunkLoader: empty chunk from server for (" << chunkX << "," << chunkZ << ")\n";
        std::lock_guard<std::mutex> lock(mtx);
        pendingRequests.erase(std::pair<int,int>(chunkX, chunkZ));
        return;
    }

    // build mesh (local positions 0..chunkSize-1)
    auto verts = buildChunkMesh(chunkData.blocks.data(), chunkData.width, chunkData.height, chunkData.depth);

    // offset into world space (chunk index * chunkSize)
    glm::vec3 worldOffset((float)chunkX * (float)chunkSize, 0.0f, (float)chunkZ * (float)chunkSize);
    for (auto &v : verts) v.pos += worldOffset;

    {
        std::lock_guard<std::mutex> lock(mtx);
        ChunkKey k{chunkX, chunkZ};
        loadedChunks.emplace(k, std::move(verts));
        pendingRequests.erase(std::pair<int,int>(chunkX, chunkZ));
    }

    std::cout << "ChunkLoader: loaded chunk (" << chunkX << ", " << chunkZ << ")\n";
}

std::vector<Vertex> ChunkLoader::getCombinedMesh() {
    std::vector<Vertex> combined;
    std::lock_guard<std::mutex> lock(mtx);
    // estimate reserve
    size_t totalVerts = 0;
    for (auto &kv : loadedChunks) totalVerts += kv.second.size();
    combined.reserve(totalVerts);
    for (auto &kv : loadedChunks) {
        combined.insert(combined.end(), kv.second.begin(), kv.second.end());
    }
    return combined; // copy
}

bool ChunkLoader::hasChunks() const {
    std::lock_guard<std::mutex> lock(mtx);
    return !loadedChunks.empty();
}
