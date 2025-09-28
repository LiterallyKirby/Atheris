#pragma once

#include <vector>
#include <cstdint>

enum class BlockType : uint8_t {
    Air = 0,
    Grass = 1,
    Stone = 2,
    Dirt = 3,
    Ore = 4
};

enum class RampDirection : uint8_t {
    None = 0,
    North = 1,
    South = 2,
    East = 3,
    West = 4,
    NorthEast = 5,
    NorthWest = 6,
    SouthEast = 7,
    SouthWest = 8
};

struct Block {
    BlockType type = BlockType::Air;
    RampDirection ramp = RampDirection::None;
};

class Chunk {
public:
    Chunk(int chunkX, int chunkZ, int w, int h, int d);
    Chunk(int chunkX, int chunkZ, int w, int h, int d, const std::vector<uint8_t>& blockData);

    void generateSimpleTerrain();
    void addRampsToTerrain();

    const std::vector<Block>& getBlocks() const { return blocks; }

    int getChunkX() const { return cx; }
    int getChunkZ() const { return cz; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getDepth() const { return depth; }

private:
    int cx, cz;
    int width, height, depth;
    std::vector<Block> blocks;

    int index(int x, int y, int z) const { return x + width * (y + height * z); }
};
