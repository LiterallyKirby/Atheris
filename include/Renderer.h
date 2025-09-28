#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Constants.h"
struct Vertex {
    glm::vec3 pos;
    glm::vec2 tex;
};

class Renderer {
public:
    Renderer(int screenWidth, int screenHeight,
             const std::string& vertexShaderPath,
             const std::string& fragmentShaderPath);
    ~Renderer();

    void render();
    void setView(const glm::mat4& viewMatrix);
    void initCube();

    // Upload mesh (pos + tex) and replace any previous mesh
    void setMesh(const std::vector<Vertex>& vertices);

private:
    // helper functions (file load, shader compile, texture load)
    std::string loadFileToString(const std::string& path);
    GLuint compileShader(GLenum type, const std::string& src);
    GLuint linkProgram(GLuint vs, GLuint fs);
    GLuint loadTexture(const std::string& path);

    void createEmptyMeshBuffers(); // create VAO/VBO for mesh if not exist

    GLuint VAO{0}, VBO{0}; // for dynamic mesh
    GLuint shaderProgram{0};
    GLuint texture{0};

    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    int width, height;

    size_t vertexCount{0}; // number of vertices currently in VBO
};
