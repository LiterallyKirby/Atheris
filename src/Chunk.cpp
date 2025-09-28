#include "Chunk.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <glm/glm.hpp>
#include <FastNoiseLite.h>

Chunk::Chunk(int chunkX, int chunkZ, int w, int h, int d)
    : cx(chunkX), cz(chunkZ), width(w), height(h), depth(d), blocks((size_t)w*h*d, Block{})
{}

Chunk::Chunk(int chunkX, int chunkZ, int w, int h, int d, const std::vector<uint8_t>& blockData)
    : cx(chunkX), cz(chunkZ), width(w), height(h), depth(d)
{
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
    baseNoise.SetFrequency(0.06f); // slightly higher frequency → smoother hills
    baseNoise.SetFractalOctaves(2); // fewer octaves → smoother
    baseNoise.SetSeed(0);

    FastNoiseLite detailNoise;
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    detailNoise.SetFrequency(0.12f); // smaller fine detail
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

            // Base mountain shape
            float heightNoise = baseNoise.GetNoise((float)worldX, (float)worldZ);

            // Soft detail blending
            float detail = detailNoise.GetNoise((float)worldX, (float)worldZ) * 0.1f;

            float combinedNoise = heightNoise + detail;

            // Smooth mountain shaping
            combinedNoise = powf(combinedNoise * 0.5f + 0.5f, 1.3f) * 2.0f - 1.0f;

            int terrainHeight = (int)((combinedNoise + 1.0f) * 0.5f * (height * 0.35f)) + height / 4;
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
                    if (rand() % 100 < 3) {
                        blocks[idx].type = BlockType::Ore;
                    }
                }
            }
        }
    }

    addRampsToTerrain();
}








void Chunk::addRampsToTerrain() {
    auto isRamp = [&](int x, int y, int z) -> bool {
        if (x < 0 || x >= width || z < 0 || z >= depth || y < 0 || y >= height)
            return false;
        size_t idx = x + width * (y + height * z);
        return blocks[idx].ramp != RampDirection::None;
    };

    auto getTopHeight = [&](int x, int z) -> int {
        if (x < 0 || x >= width || z < 0 || z >= depth) return -1;
        for (int y = height - 1; y >= 0; --y) {
            size_t idx = x + width * (y + height * z);
            if (blocks[idx].type != BlockType::Air) {
                return y;
            }
        }
        return -1;
    };

    struct Direction {
        int dx, dz;
        RampDirection rampDir;
    };

    Direction dirs[] = {
        {0, -1, RampDirection::North},       // North
        {0, 1,  RampDirection::South},       // South
        {1, 0,  RampDirection::East},        // East
        {-1, 0, RampDirection::West},        // West
        {1, -1, RampDirection::NorthEast},   // Northeast
        {-1, -1, RampDirection::NorthWest},  // Northwest
        {1, 1,  RampDirection::SouthEast},   // Southeast
        {-1, 1,  RampDirection::SouthWest}   // Southwest
    };

    std::vector<std::vector<bool>> rampVisited(width, std::vector<bool>(depth, false));
    bool debugRampPlacement = false; // set true for debug

  
auto shouldPlaceRamp = [&](int x, int z, Direction dir) -> bool {
    int heightHere = getTopHeight(x, z);
    int heightAhead = getTopHeight(x + dir.dx, z + dir.dz);

    if (heightHere == -1 || heightAhead == -1) return false;

    // Only 1-block drop
    if (heightHere != heightAhead + 1) return false;

    // Check ahead slope to avoid large ramps on mountains
    int secondAheadHeight = getTopHeight(x + dir.dx * 2, z + dir.dz * 2);
    if (secondAheadHeight != -1 && secondAheadHeight + 1 == heightHere) {
        return false; // Slope continues, skip ramp
    }

    // Diagonal slopes require both orthogonal neighbors to match
    if (dir.dx != 0 && dir.dz != 0) {
        int heightX = getTopHeight(x + dir.dx, z);
        int heightZ = getTopHeight(x, z + dir.dz);
        if (heightHere - heightX != 1 || heightHere - heightZ != 1) {
            return false;
        }
    }

    // Randomize to keep chunkiness
   // if (rand() % 100 < 15) return false;

    return true;
};


    for (int x = 0; x < width; ++x) {
        for (int z = 0; z < depth; ++z) {
            int currentHeight = getTopHeight(x, z);
            if (currentHeight == -1) continue;

            for (auto& dir : dirs) {
                int nx = x + dir.dx;
                int nz = z + dir.dz;
                int neighborHeight = getTopHeight(nx, nz);

                if (neighborHeight != -1 &&
                    !isRamp(nx, neighborHeight, nz) &&
                    !rampVisited[nx][nz] &&
                    shouldPlaceRamp(x, z, dir))
                {
                    size_t idx = nx + width * ((neighborHeight + 1) + height * nz);

                    if (nx >= 0 && nx < width &&
                        nz >= 0 && nz < depth &&
                        neighborHeight + 1 < height &&
                        blocks[idx].type == BlockType::Air)
                    {
                        size_t higherIdx = x + width * (currentHeight + height * z);
                        BlockType materialType = blocks[higherIdx].type;

                        blocks[idx].type = materialType;
                        blocks[idx].ramp = dir.rampDir;
                        rampVisited[nx][nz] = true;

                        if (debugRampPlacement) {
                            blocks[idx].type = BlockType::Ore;
                        }

                        std::cout << "Placed "
                                  << (dir.rampDir == RampDirection::North ? "North" :
                                      dir.rampDir == RampDirection::South ? "South" :
                                      dir.rampDir == RampDirection::East ? "East" :
                                      dir.rampDir == RampDirection::West ? "West" :
                                      dir.rampDir == RampDirection::NorthEast ? "NE" :
                                      dir.rampDir == RampDirection::NorthWest ? "NW" :
                                      dir.rampDir == RampDirection::SouthEast ? "SE" : "SW")
                                  << " ramp at (" << nx << ", " << (neighborHeight + 1) << ", " << nz << ")\n";
                    }
                }
            }
        }
    }
}






