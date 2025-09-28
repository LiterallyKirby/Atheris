#include "Player.h"
#include <cmath>

Player::Player(float startX, float startY, float startZ)
    : camera(),               // initialize first, matches declaration order
      position(startX, startY, startZ)
{}

glm::vec3 Player::getPosition() const {
    return position;
}

glm::vec3 Player::getChunkCoordinates(int chunkSize) const {
    int chunkX = std::floor(position.x / chunkSize);
    int chunkZ = std::floor(position.z / chunkSize);
    return glm::vec3(chunkX, 0, chunkZ);
}

void Player::update(float dt) {
    // update camera position if needed
    camera.position = position;
}
