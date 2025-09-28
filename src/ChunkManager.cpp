#include "ChunkManager.h"
#include <iostream>

ChunkManager::ChunkManager(int chunkSize_, int renderDistance_)
    : chunkSize(chunkSize_), renderDistance(renderDistance_) {}

void ChunkManager::loadChunk(int chunkX, int chunkZ) {
    ChunkKey key{chunkX, chunkZ};
    std::lock_guard<std::mutex> lk(mtx);
    if (chunks.find(key) != chunks.end()) return;
    
    // Fixed: Use proper height parameter (should be different from width/depth for realistic terrain)
    auto c = std::make_unique<Chunk>(chunkX, chunkZ, chunkSize, 64, chunkSize); // Using 64 for height
    c->generateSimpleTerrain();
    chunks[key] = std::move(c);
    std::cout << "Server: generated chunk " << chunkX << "," << chunkZ << "\n";
}

Chunk* ChunkManager::getChunk(int chunkX, int chunkZ) {
    ChunkKey key{chunkX, chunkZ};
    std::lock_guard<std::mutex> lk(mtx);
    auto it = chunks.find(key);
    if (it == chunks.end()) return nullptr;
    return it->second.get();
}

void ChunkManager::loadChunkFromData(int chunkX, int chunkZ, int w, int h, int d, const std::vector<uint8_t>& blocks) {
    ChunkKey key{chunkX, chunkZ};
    std::lock_guard<std::mutex> lk(mtx);
    if (chunks.find(key) != chunks.end()) return;
    
    // Fixed: Use the constructor that takes block data
    auto c = std::make_unique<Chunk>(chunkX, chunkZ, w, h, d, blocks);
    chunks[key] = std::move(c);
}

std::vector<std::pair<ChunkKey, std::shared_ptr<Chunk>>> ChunkManager::getLoadedChunksSnapshot() {
    std::vector<std::pair<ChunkKey, std::shared_ptr<Chunk>>> out;
    std::lock_guard<std::mutex> lk(mtx);
    out.reserve(chunks.size());
    for (auto &kv : chunks) {
        // Fixed: Create a proper shared_ptr with custom deleter that does nothing
        // This allows safe sharing without interfering with the unique_ptr ownership
        std::shared_ptr<Chunk> sharedChunk(kv.second.get(), [](Chunk*){ /* no-op deleter */ });
        out.emplace_back(kv.first, sharedChunk);
    }
    return out;
}

// Additional helper methods that might be useful:

std::vector<uint8_t> ChunkManager::serializeChunk(int chunkX, int chunkZ) {
    ChunkKey key{chunkX, chunkZ};
    std::lock_guard<std::mutex> lk(mtx);
    auto it = chunks.find(key);
    if (it == chunks.end()) return {};
    
    const auto& blocks = it->second->getBlocks();
    std::vector<uint8_t> serialized;
    
    // Pack Block data: [blockType, isRamp, rampDirection] for each block
    serialized.reserve(blocks.size() * 3);
    for (const auto& block : blocks) {
        serialized.push_back(static_cast<uint8_t>(block.type));

        serialized.push_back(static_cast<uint8_t>(block.ramp));
    }
    
    return serialized;
}

void ChunkManager::unloadChunk(int chunkX, int chunkZ) {
    ChunkKey key{chunkX, chunkZ};
    std::lock_guard<std::mutex> lk(mtx);
    auto it = chunks.find(key);
    if (it != chunks.end()) {
        chunks.erase(it);
        std::cout << "Server: unloaded chunk " << chunkX << "," << chunkZ << "\n";
    }
}

size_t ChunkManager::getLoadedChunkCount() {
    std::lock_guard<std::mutex> lk(mtx);
    return chunks.size();
}
