
#pragma once

#include "ChunkManager.h" // for ChunkKey (your ChunkKey type)
#include "Client.h"
#include "Player.h"
#include "ChunkMeshBuilder.h"

#include <vector>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <set>

class ChunkLoader {
public:
    ChunkLoader(Client* client, Player* player, int chunkSize, int renderDistance);
    ~ChunkLoader();

    // start/stop background loader thread
    void start();
    void stop();

    // Return a combined mesh (copy) of all loaded chunks. Thread-safe.
    std::vector<Vertex> getCombinedMesh();

    // Optional: check if loader has any chunks loaded yet
    bool hasChunks() const;

private:
    void threadMain(); // background worker
    void requestAndStoreChunk(int chunkX, int chunkZ);

    Client* client;    // pointer owned externally (client must outlive loader)
    Player* player;    // pointer owned externally
    int chunkSize;
    int renderDistance;

    std::thread worker;
    std::atomic<bool> running;

    // loaded meshes keyed by chunk index
    std::unordered_map<ChunkKey, std::vector<Vertex>> loadedChunks;
    std::set<std::pair<int,int>> pendingRequests;

    mutable std::mutex mtx; // protects loadedChunks & pendingRequests
};
