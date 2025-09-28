#include "Camera.h"
#include <algorithm>

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 10.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

// Constructor with vectors
Camera::Camera(glm::vec3 pos, float y, float p) 
    : position(pos)
    , front(glm::vec3(0.0f, 0.0f, -1.0f))
    , up(glm::vec3(0.0f, 1.0f, 0.0f))
    , right(glm::vec3(1.0f, 0.0f, 0.0f))
    , worldUp(glm::vec3(0.0f, 1.0f, 0.0f))  // Initialize in correct order
    , yaw(y)
    , pitch(p)
    , movementSpeed(SPEED)
    , mouseSensitivity(SENSITIVITY)  // Initialize after worldUp
    , zoom(ZOOM)
{
    updateCameraVectors();
}

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + front, up);
}

// Processes input received from any keyboard-like input system
void Camera::processKeyboard(char direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    
    switch (direction) {
        case 'W':
        case 'w':
            position += front * velocity;
            break;
        case 'S':
        case 's':
            position -= front * velocity;
            break;
        case 'A':
        case 'a':
            position -= right * velocity;
            break;
        case 'D':
        case 'd':
            position += right * velocity;
            break;
        case ' ':  // Space key for moving up
            position += up * velocity;
            break;
        case 'X':
        case 'x':  // X key for moving down
            position -= up * velocity;
            break;
    }
}

// Processes input received from a mouse input system
void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch) {
        pitch = std::clamp(pitch, -89.0f, 89.0f);
    }

    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

// Processes input received from a mouse scroll-wheel event
void Camera::processMouseScroll(float yoffset) {
    zoom -= yoffset;
    zoom = std::clamp(zoom, 1.0f, 45.0f);
}

// Calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors() {
    // Calculate the new Front vector
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    // Also re-calculate the Right and Up vector
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}


glm::vec3 Camera::getChunkCoordinates(int chunkSize) const {
    int chunkX = floor(position.x / chunkSize);
    int chunkZ = floor(position.z / chunkSize);
    return glm::vec3(chunkX, 0, chunkZ);
}
