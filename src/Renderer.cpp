// Renderer.cpp
#include "Renderer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring> // for offsetof
#include <cassert>
#include <string>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Renderer::Renderer(int screenWidth, int screenHeight,
                   const std::string &vertexShaderPath,
                   const std::string &fragmentShaderPath)
    : width(screenWidth), height(screenHeight) {

  // load shader sources from files
  std::string vertSrc = loadFileToString(vertexShaderPath);
  std::string fragSrc = loadFileToString(fragmentShaderPath);

  if (vertSrc.empty() || fragSrc.empty()) {
    std::cerr << "Shader source empty. Check file paths.\n";
  }

  GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
  shaderProgram = linkProgram(vs, fs);

  // compiled shaders can be deleted after linking
  if (vs) glDeleteShader(vs);
  if (fs) glDeleteShader(fs);

  // Ensure we have buffers and attribute setup ready
  createEmptyMeshBuffers();

  // populate VBO with default cube so we don't draw nothing initially
  initCube();

  // load texture from assets folder by default
  std::string texturePath = "assets/cat.png";
  texture = loadTexture(texturePath);
  if (!texture) {
    std::cerr << "Failed to load texture: " << texturePath << "\n";
  }

  model = glm::mat4(1.0f);
  view = glm::mat4(1.0f);
  projection = glm::perspective(glm::radians(45.0f),
                                (float)width / (float)height, 0.1f, 100.0f);
}

Renderer::~Renderer() {
  if (shaderProgram) glDeleteProgram(shaderProgram);
  if (texture) glDeleteTextures(1, &texture);
  if (VBO) glDeleteBuffers(1, &VBO);
  if (VAO) glDeleteVertexArrays(1, &VAO);
}

void Renderer::setView(const glm::mat4 &viewMatrix) { view = viewMatrix; }

std::string Renderer::loadFileToString(const std::string &path) {
  std::ifstream in(path, std::ios::in | std::ios::binary);
  if (!in) {
    std::cerr << "Failed to open file: " << path << "\n";
    return "";
  }
  std::ostringstream contents;
  contents << in.rdbuf();
  return contents.str();
}

GLuint Renderer::compileShader(GLenum type, const std::string &src) {
  GLuint shader = glCreateShader(type);
  const char *cstr = src.c_str();
  glShaderSource(shader, 1, &cstr, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLint logLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
    std::vector<char> log((size_t)logLen ? (size_t)logLen : 1);
    glGetShaderInfoLog(shader, logLen, nullptr, log.data());
    std::cerr << "Shader compile error: " << log.data() << "\n";
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

GLuint Renderer::linkProgram(GLuint vertShader, GLuint fragShader) {
  GLuint prog = glCreateProgram();
  if (vertShader) glAttachShader(prog, vertShader);
  if (fragShader) glAttachShader(prog, fragShader);
  glLinkProgram(prog);

  GLint success;
  glGetProgramiv(prog, GL_LINK_STATUS, &success);
  if (!success) {
    GLint logLen = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
    std::vector<char> log((size_t)logLen ? (size_t)logLen : 1);
    glGetProgramInfoLog(prog, logLen, nullptr, log.data());
    std::cerr << "Program link error: " << log.data() << "\n";
    glDeleteProgram(prog);
    return 0;
  }
  return prog;
}

GLuint Renderer::loadTexture(const std::string &path) {
  GLuint textureID = 0;
  glGenTextures(1, &textureID);

  int w = 0, h = 0, nrChannels = 0;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path.c_str(), &w, &h, &nrChannels, 0);
  if (!data) {
    std::cerr << "stb_image failed to load: " << path << "\n";
    glDeleteTextures(1, &textureID);
    return 0;
  }

  GLenum format = GL_RGB;
  if (nrChannels == 1) format = GL_RED;
  else if (nrChannels == 3) format = GL_RGB;
  else if (nrChannels == 4) format = GL_RGBA;

  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  // sensible defaults
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(data);
  return textureID;
}

void Renderer::createEmptyMeshBuffers() {
    if (VAO == 0) glGenVertexArrays(1, &VAO);
    if (VBO == 0) glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // position (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, pos)));

    // texcoord (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, tex)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::setMesh(const std::vector<Vertex>& vertices) {
    createEmptyMeshBuffers(); // ensure buffers & attribs exist

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    if (!vertices.empty()) {
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
        vertexCount = vertices.size();
    } else {
        // empty mesh
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        vertexCount = 0;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::initCube() {
  // creates a small 1x1 cube mesh and uploads it via setMesh()
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
      v.pos = glm::vec3(raw[i*5 + 0], raw[i*5 + 1], raw[i*5 + 2]);
      v.tex = glm::vec2(raw[i*5 + 3], raw[i*5 + 4]);
      cube.push_back(v);
  }

  setMesh(cube);
}

// ----------------- end mesh helpers -----------------

void Renderer::render() {
    if (!shaderProgram) return;
    if (vertexCount == 0) return; // nothing to draw

    glUseProgram(shaderProgram);

    glm::mat4 mvp = projection * view * model;
    GLint mvpLoc = glGetUniformLocation(shaderProgram, "uMVP");
    if (mvpLoc != -1)
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    if (modelLoc != -1)
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    if (lightPosLoc != -1)
        glUniform3f(lightPosLoc, 1.2f, 1.0f, 2.0f);

    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    if (viewPosLoc != -1)
        glUniform3f(viewPosLoc, 0.0f, 0.0f, 3.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    GLint texLoc = glGetUniformLocation(shaderProgram, "texture1");
    if (texLoc != -1)
        glUniform1i(texLoc, 0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
    glBindVertexArray(0);
}
