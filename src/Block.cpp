
#include "Block.h"
// Updated texture coordinate function to handle BlockType instead of Block
glm::vec2 getTextureCoordForBlock(BlockType blockType) {
    switch (blockType) {
        case BlockType::Grass: return {0.0f, 0.0f};
        case BlockType::Stone: return {0.25f, 0.0f};
        case BlockType::Dirt:  return {0.5f, 0.0f};
        case BlockType::Ore:   return {0.75f, 0.0f};
        default: return {0.0f, 0.0f};
    }
}
