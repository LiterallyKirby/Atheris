
#pragma once
#include <vector>
#include "Renderer.h"
#include "Chunk.h"

void addVerticesForRampNorth(glm::vec3 pos, glm::vec2 texCoord, std::vector<Vertex>& verts);
void addVerticesForRampSouth(glm::vec3 pos, glm::vec2 texCoord, std::vector<Vertex>& verts);
void addVerticesForRampEast(glm::vec3 pos, glm::vec2 texCoord, std::vector<Vertex>& verts);
void addVerticesForRampWest(glm::vec3 pos, glm::vec2 texCoord, std::vector<Vertex>& verts);


std::vector<Vertex> buildChunkMesh(const unsigned char* blocks, int width, int height, int depth);


void addCubeMesh(BlockType type, glm::vec3 pos, std::vector<Vertex>& verts);
void addRampMesh(RampDirection dir, BlockType type, glm::vec3 pos, std::vector<Vertex>& verts);
glm::vec2 getTextureCoordForBlock(BlockType type);


