#include "Chunk.h"
#include <FastNoiseLite.h>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>
Chunk::Chunk(int chunkX, int chunkZ, int w, int h, int d)
    : cx(chunkX), cz(chunkZ), width(w), height(h), depth(d),
      blocks((size_t)w * h * d, Block{}) {}

Chunk::Chunk(int chunkX, int chunkZ, int w, int h, int d,
             const std::vector<uint8_t> &blockData)
    : cx(chunkX), cz(chunkZ), width(w), height(h), depth(d) {
  blocks.reserve(blockData.size());
  for (auto b : blockData) {
    Block blk;
    blk.type = static_cast<BlockType>(b);
    blk.ramp = RampDirection::None;
    blocks.push_back(blk);
  }
}



void Chunk::generateSimpleTerrain() {
    FastNoiseLite baseNoise;
    baseNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    baseNoise.SetFrequency(0.015f);  // Very low frequency → broad changes
    baseNoise.SetFractalOctaves(2);  // Slight complexity
    baseNoise.SetSeed(0);

    FastNoiseLite detailNoise;
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    detailNoise.SetFrequency(0.08f); // Small detail noise
    detailNoise.SetFractalOctaves(1);
    detailNoise.SetSeed(12345);

    FastNoiseLite caveNoise;
    caveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    caveNoise.SetFrequency(0.12f);
    caveNoise.SetSeed(54321);

    for (int x = 0; x < width; ++x) {
        for (int z = 0; z < depth; ++z) {
            int worldX = cx * width + x;
            int worldZ = cz * depth + z;

            // Gentle base height variation
            float heightNoise = baseNoise.GetNoise((float)worldX, (float)worldZ) * 0.2f; // small amplitude but not zero

            // Minimal fine detail
            float detail = detailNoise.GetNoise((float)worldX, (float)worldZ) * 0.05f;

            float combinedNoise = heightNoise + detail;

            // Gentle smoothing curve
            combinedNoise = powf(combinedNoise * 0.5f + 0.5f, 1.1f) * 2.0f - 1.0f;

            // Mostly flat with gentle undulations
            int terrainHeight = (int)((combinedNoise + 1.0f) * 0.5f * (height * 0.2f)) + height * 0.5f;
            terrainHeight = std::max(1, std::min(terrainHeight, height - 1));

            for (int y = 0; y < height; ++y) {
                size_t idx = x + width * (y + height * z);

                if (y < terrainHeight - 3) {
                    float caveVal = caveNoise.GetNoise((float)worldX, (float)y * 2.0f, (float)worldZ);
                    if (caveVal > 0.4f) {
                        blocks[idx].type = BlockType::Air;
                        blocks[idx].ramp = RampDirection::None;
                        continue;
                    }
                }

                if (y > terrainHeight) {
                    blocks[idx].type = BlockType::Air;
                    blocks[idx].ramp = RampDirection::None;
                } else if (y == terrainHeight) {
                    blocks[idx].type = BlockType::Grass;
                } else if (y >= terrainHeight - 2) {
                    blocks[idx].type = BlockType::Dirt;
                } else {
                    blocks[idx].type = BlockType::Stone;
                    if (rand() % 100 < 2) { // rarer ores
                        blocks[idx].type = BlockType::Ore;
                    }
                }
            }
        }
    }

    addRampsToTerrain();
}



void Chunk::addRampsToTerrain() {
    auto getTopHeight = [&](int x, int z) -> int {
        if (x < 0 || x >= width || z < 0 || z >= depth)
            return -1;
        for (int y = height - 1; y >= 0; --y) {
            size_t idx = x + width * (y + height * z);
            if (blocks[idx].type != BlockType::Air) {
                return y;
            }
        }
        return -1;
    };

    auto isAir = [&](int x, int y, int z) -> bool {
        if (x < 0 || x >= width || z < 0 || z >= depth || y < 0 || y >= height)
            return false;
        size_t idx = x + width * (y + height * z);
        return blocks[idx].type == BlockType::Air;
    };

    struct Direction {
        int dx, dz;
        RampDirection rampDir;
        const char* name;
    };

    // Separate cardinal and diagonal directions
    Direction cardinalDirs[] = {
        {0, -1, RampDirection::North, "North"},
        {0, 1, RampDirection::South, "South"},
        {1, 0, RampDirection::East, "East"},
        {-1, 0, RampDirection::West, "West"}
    };

    Direction diagonalDirs[] = {
        {1, -1, RampDirection::NorthEast, "NorthEast"},
        {-1, -1, RampDirection::NorthWest, "NorthWest"},
        {1, 1, RampDirection::SouthEast, "SouthEast"},
        {-1, 1, RampDirection::SouthWest, "SouthWest"}
    };

    std::vector<std::vector<bool>> rampPlaced(width, std::vector<bool>(depth, false));

    // For each position in the chunk
    for (int x = 0; x < width; ++x) {
        for (int z = 0; z < depth; ++z) {
            if (rampPlaced[x][z]) continue;
            
            int currentHeight = getTopHeight(x, z);
            if (currentHeight == -1) continue;

            bool placedRamp = false;

            // FIRST: Try cardinal directions (N, S, E, W)
            for (auto& dir : cardinalDirs) {
                int neighborX = x + dir.dx;
                int neighborZ = z + dir.dz;
                
                if (neighborX < 0 || neighborX >= width || neighborZ < 0 || neighborZ >= depth)
                    continue;
                
                if (rampPlaced[neighborX][neighborZ]) continue;
                
                int neighborHeight = getTopHeight(neighborX, neighborZ);
                if (neighborHeight == -1) continue;
                
                int heightDiff = currentHeight - neighborHeight;
                
                if (heightDiff == 1) {
                    if (isAir(neighborX, neighborHeight + 1, neighborZ)) {
                        size_t currentIdx = x + width * (currentHeight + height * z);
                        BlockType materialType = blocks[currentIdx].type;
                        
                        size_t rampIdx = neighborX + width * ((neighborHeight + 1) + height * neighborZ);
                        blocks[rampIdx].type = materialType;
                        blocks[rampIdx].ramp = dir.rampDir;
                        rampPlaced[neighborX][neighborZ] = true;
                        
                        std::cout << "Placed " << dir.name << " ramp at (" 
                                  << neighborX << ", " << (neighborHeight + 1) << ", " << neighborZ 
                                  << ") connecting height " << currentHeight << " to " << neighborHeight << "\n";
                        
                        placedRamp = true;
                        break; // Stop after placing one ramp
                    }
                }
            }

            // SECOND: Only if no cardinal ramp was placed, try diagonal directions
            if (!placedRamp) {
                for (auto& dir : diagonalDirs) {
                    int neighborX = x + dir.dx;
                    int neighborZ = z + dir.dz;
                    
                    if (neighborX < 0 || neighborX >= width || neighborZ < 0 || neighborZ >= depth)
                        continue;
                    
                    if (rampPlaced[neighborX][neighborZ]) continue;
                    
                    int neighborHeight = getTopHeight(neighborX, neighborZ);
                    if (neighborHeight == -1) continue;
                    
                    int heightDiff = currentHeight - neighborHeight;
                    
                    if (heightDiff == 1) {
                        if (isAir(neighborX, neighborHeight + 1, neighborZ)) {
                            size_t currentIdx = x + width * (currentHeight + height * z);
                            BlockType materialType = blocks[currentIdx].type;
                            
                            size_t rampIdx = neighborX + width * ((neighborHeight + 1) + height * neighborZ);
                            blocks[rampIdx].type = materialType;
                            blocks[rampIdx].ramp = dir.rampDir;
                            rampPlaced[neighborX][neighborZ] = true;
                            
                            std::cout << "Placed " << dir.name << " ramp at (" 
                                      << neighborX << ", " << (neighborHeight + 1) << ", " << neighborZ 
                                      << ") connecting height " << currentHeight << " to " << neighborHeight << "\n";
                            
                            break; // Stop after placing one ramp
                        }
                    }
                }
            }
        }
    }
}
