#pragma once
#include <glm/glm.hpp>
#include "Camera.h"

class Player {
public:
    Player(float startX, float startY, float startZ);

    glm::vec3 getPosition() const;
    glm::vec3 getChunkCoordinates(int chunkSize) const;
    void update(float dt);

    Camera camera; // 🎯 Camera inside Player

    glm::vec3 position;
private:

};
