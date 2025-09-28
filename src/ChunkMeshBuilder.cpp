#include "ChunkMeshBuilder.h"
#include "Chunk.h"
#include <Block.h>
static std::vector<Vertex> makeCube(glm::ivec3 pos) {
    // positions + UV for a cube
    float raw[] = {
        // back face
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
        // front face
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
        // left face
        -0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
        // right face
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
        // bottom face
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
        // top face
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f
    };

    const int verts = 36;
    std::vector<Vertex> cube;
    cube.reserve(verts);
    for (int i = 0; i < verts; ++i) {
        Vertex v;
        v.pos = glm::vec3(raw[i * 5 + 0], raw[i * 5 + 1], raw[i * 5 + 2]) + glm::vec3(pos);
        v.tex = glm::vec2(raw[i * 5 + 3], raw[i * 5 + 4]);
        cube.push_back(v);
    }
    return cube;
}

static std::vector<Vertex> cubeVertexTemplate = makeCube(glm::ivec3(0,0,0));

std::vector<Vertex> buildChunkMesh(const unsigned char* data, int width, int height, int depth) {
    std::vector<Vertex> verts;

    auto index = [width, height](int x, int y, int z) {
        return x + width * (y + height * z);
    };

    for (int x = 0; x < width; ++x) {
        for (int z = 0; z < depth; ++z) {
            for (int y = 0; y < height; ++y) {
                int blockIndex = index(x, y, z);
                
                // Data is now packed as: [blockType, rampDirection, blockType, rampDirection, ...]
                // So each block takes 2 bytes
                const unsigned char rawBlockType = data[blockIndex * 2];
                const unsigned char rawRampDir = data[blockIndex * 2 + 1];

                // Convert raw bytes back to Block struct
                Block block;
                block.type = static_cast<BlockType>(rawBlockType);
                block.ramp = static_cast<RampDirection>(rawRampDir);

                if (block.type == BlockType::Air) continue;

                glm::vec3 pos = glm::vec3(x, y, z);

                if (block.ramp != RampDirection::None) {
                    addRampMesh(block.ramp, block.type, pos, verts);
                } else {
                    addCubeMesh(block.type, pos, verts);
                }
            }
        }
    }

    return verts;
}


void addCubeMesh(BlockType type, glm::vec3 pos, std::vector<Vertex>& verts) {
    // Generate cube vertices at pos
    // Set texture coordinates based on block type
    glm::vec2 texCoord = getTextureCoordForBlock(type);

    // Cube vertices (simplified)
    // You need your cube vertex positions & UVs here
    for (auto& vert : cubeVertexTemplate) {
        Vertex v = vert;
        v.pos += pos;
        v.tex += texCoord;
        verts.push_back(v);
    }
}


void addRampMesh(RampDirection dir, BlockType type, glm::vec3 pos, std::vector<Vertex>& verts) {
    glm::vec2 texCoord = getTextureCoordForBlock(type);

    switch (dir) {
        case RampDirection::North:
            // Add vertices for a ramp facing north
            addVerticesForRampNorth(pos, texCoord, verts);
            break;
        case RampDirection::South:
            addVerticesForRampSouth(pos, texCoord, verts);
            break;
        case RampDirection::East:
            addVerticesForRampEast(pos, texCoord, verts);
            break;
        case RampDirection::West:
            addVerticesForRampWest(pos, texCoord, verts);
            break;
        default:
            break;
    }
}





void addVerticesForRampNorth(glm::vec3 pos, glm::vec2 texCoord, std::vector<Vertex>& verts) {
    // North-facing ramp: high at Z=+0.5, low at Z=-0.5
    float vertices[] = {
        // Bottom face (flat)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
         
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // Sloped top face (from high Z to low Z)
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  // high end
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  // high end
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  // low end
         
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  // low end
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  // low end
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  // high end

        // Back face (vertical at high end)
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        // Left side triangle
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.5f, 1.0f,

        // Right side triangle  
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.5f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f
    };
    
    int numVerts = sizeof(vertices) / (5 * sizeof(float));
    for (int i = 0; i < numVerts; i++) {
        Vertex v;
        v.pos = glm::vec3(vertices[i*5+0], vertices[i*5+1], vertices[i*5+2]) + pos;
        v.tex = glm::vec2(vertices[i*5+3], vertices[i*5+4]) + texCoord;
        verts.push_back(v);
    }
}

void addVerticesForRampSouth(glm::vec3 pos, glm::vec2 texCoord, std::vector<Vertex>& verts) {
    // South-facing ramp: high at Z=-0.5, low at Z=+0.5
    float vertices[] = {
        // Bottom face (flat)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
         
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // Sloped top face (from high Z to low Z)
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  // high end
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  // high end
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  // low end
         
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  // low end
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  // low end
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  // high end

        // Front face (vertical at high end)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // Left side triangle
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.5f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,

        // Right side triangle  
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.5f, 1.0f
    };
    
    int numVerts = sizeof(vertices) / (5 * sizeof(float));
    for (int i = 0; i < numVerts; i++) {
        Vertex v;
        v.pos = glm::vec3(vertices[i*5+0], vertices[i*5+1], vertices[i*5+2]) + pos;
        v.tex = glm::vec2(vertices[i*5+3], vertices[i*5+4]) + texCoord;
        verts.push_back(v);
    }
}

void addVerticesForRampEast(glm::vec3 pos, glm::vec2 texCoord, std::vector<Vertex>& verts) {
    // East-facing ramp: high at X=-0.5, low at X=+0.5
    float vertices[] = {
        // Bottom face (flat)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
         
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // Sloped top face (from high X to low X)
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  // high end
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  // high end
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  // low end
         
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  // low end
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  // low end
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  // high end

        // Left face (vertical at high end)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // Front side triangle
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.5f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,

        // Back side triangle  
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.5f, 1.0f
    };
    
    int numVerts = sizeof(vertices) / (5 * sizeof(float));
    for (int i = 0; i < numVerts; i++) {
        Vertex v;
        v.pos = glm::vec3(vertices[i*5+0], vertices[i*5+1], vertices[i*5+2]) + pos;
        v.tex = glm::vec2(vertices[i*5+3], vertices[i*5+4]) + texCoord;
        verts.push_back(v);
    }
}

void addVerticesForRampWest(glm::vec3 pos, glm::vec2 texCoord, std::vector<Vertex>& verts) {
    // West-facing ramp: high at X=+0.5, low at X=-0.5
    float vertices[] = {
        // Bottom face (flat)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
         
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // Sloped top face (from high X to low X)
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  // high end
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  // low end
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  // low end
         
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  // low end
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  // high end
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  // high end

        // Right face (vertical at high end)
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        // Front side triangle
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.5f, 1.0f,

        // Back side triangle  
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.5f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f
    };
    
    int numVerts = sizeof(vertices) / (5 * sizeof(float));
    for (int i = 0; i < numVerts; i++) {
        Vertex v;
        v.pos = glm::vec3(vertices[i*5+0], vertices[i*5+1], vertices[i*5+2]) + pos;
        v.tex = glm::vec2(vertices[i*5+3], vertices[i*5+4]) + texCoord;
        verts.push_back(v);
    }
}

glm::vec2 getTextureCoordForBlock(const Block& block) {
    // Texture depends ONLY on material, not ramp
    switch (block.type) {
        case BlockType::Grass: return {0.0f, 0.0f};
        case BlockType::Stone: return {0.25f, 0.0f};
        case BlockType::Dirt:  return {0.5f, 0.0f};
        case BlockType::Ore:   return {0.75f, 0.0f};
        default: return {0.0f, 0.0f};
    }
}
