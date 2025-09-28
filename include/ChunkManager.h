#pragma once
#include "Chunk.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <glm/vec2.hpp>
#include <mutex>

struct ChunkKey {
    int x, z;
    bool operator==(const ChunkKey& o) const { return x==o.x && z==o.z; }
};
namespace std {
  template<> struct hash<ChunkKey> {
    size_t operator()(ChunkKey const& k) const noexcept {
      return (std::hash<int>()(k.x) * 73856093) ^ (std::hash<int>()(k.z) * 19349663);
    }
  };
}

class ChunkManager {
public:
    ChunkManager(int chunkSize = 16, int renderDistance = 4);

    // Server-side: load (generate) chunk
    void loadChunk(int chunkX, int chunkZ);
    Chunk* getChunk(int chunkX, int chunkZ); // pointer or nullptr

    // Client-side: accept chunk bytes from server
    void loadChunkFromData(int chunkX, int chunkZ, int w, int h, int d, const std::vector<uint8_t>& blocks);

    // Build one combined vertex array for all currently loaded chunks (client uses this to send to renderer)
    // The build of vertex positions/UVs is done by client's existing buildChunkMesh helper (we just gather bytes)
    std::vector<std::pair<ChunkKey, std::shared_ptr<Chunk>>> getLoadedChunksSnapshot();

    int getChunkSize() const { return chunkSize; }
    int getRenderDistance() const { return renderDistance; }
std::vector<uint8_t> serializeChunk(int chunkX, int chunkZ);
void unloadChunk(int chunkX, int chunkZ); 
size_t getLoadedChunkCount();
private:
    std::unordered_map<ChunkKey, std::unique_ptr<Chunk>> chunks;
    int chunkSize;
    int renderDistance;
    std::mutex mtx;
};
