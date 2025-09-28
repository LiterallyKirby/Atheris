// src/main.cpp - load a grid of chunks around the player (fixed)

#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "Camera.h"
#include "ChunkMeshBuilder.h"
#include "Client.h"
#include "Player.h"
#include "Renderer.h"
#include "Server.h"
#include "ChunkManager.h" // for ChunkKey

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <unordered_map>
#include <limits>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtc/type_ptr.hpp>

constexpr int SERVER_PORT = 42069;
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

constexpr int CHUNK_SIZE = 16;    // chunk width (blocks)
constexpr int RENDER_DISTANCE = 2; // in chunks (how many chunks away to load)

// Globals for mouse control
float dt = 0.0f;
float lastframe = 0.0f;
bool firstMouse = true;
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;

// Global player pointer (mouse callback)
Player *gPlayer = nullptr;

void mouse_callback(GLFWwindow* /*window*/, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoffset = (float)(xpos - lastX);
    float yoffset = (float)(lastY - ypos); // inverted
    lastX = (float)xpos;
    lastY = (float)ypos;

    if (gPlayer) gPlayer->camera.processMouseMovement(xoffset, yoffset);
}

static glm::ivec2 chunkCoordsFromPlayer(const Player& player, int chunkSize) {
    glm::vec3 c = player.getChunkCoordinates(chunkSize);
    return glm::ivec2((int)std::floor(c.x), (int)std::floor(c.z));
}

// offset a mesh's positions by world translation
static void offsetMesh(std::vector<Vertex>& verts, const glm::vec3& offset) {
    for (auto &v : verts) v.pos += offset;
}

int main() {
    std::cout << "Starting program..." << std::endl;

    // Create player
    Player player(0.0f, 0.0f, 0.3f);
    gPlayer = &player;

    // Start server (local single-player server)
    auto server = std::make_unique<Server>(SERVER_PORT);
    server->setPlayer(&player);
    server->start();

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        server->stop();
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Voxel Renderer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        server->stop();
        glfwTerminate();
        return -1;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        server->stop();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Renderer renderer(WINDOW_WIDTH, WINDOW_HEIGHT, "shaders/vertex.glsl", "shaders/frag.glsl");

    // Give server a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    Client client;
    if (!client.connectToServer("127.0.0.1", SERVER_PORT)) {
        std::cerr << "Failed to connect to server — continuing (will show default cube)\n";
    }

    // loadedChunks keyed by chunk index (chunkX, chunkZ)
    std::unordered_map<ChunkKey, std::vector<Vertex>> loadedChunks;

    // last player chunk so we only rebuild combined mesh when it changes
    glm::ivec2 lastPlayerChunk(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());

    // sky color
    glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // light sky blue

    while (!glfwWindowShouldClose(window)) {
        // Update timing
        float currentTime = (float)glfwGetTime();
        dt = currentTime - lastframe;
        lastframe = currentTime;

        // Update player position from camera
        player.position = player.camera.position;

        // Input handling
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) player.camera.processKeyboard('W', dt);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) player.camera.processKeyboard('S', dt);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) player.camera.processKeyboard('A', dt);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) player.camera.processKeyboard('D', dt);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) player.camera.processKeyboard(' ', dt);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) player.camera.processKeyboard('X', dt);

        // compute player's current chunk index
        glm::ivec2 playerChunk = chunkCoordsFromPlayer(player, CHUNK_SIZE);

        // If player moved into a new chunk, (re)request chunks around them
        if (playerChunk != lastPlayerChunk) {
            // We keep previously loaded chunks in the map. If you want to unload far chunks,
            // iterate and remove keys outside the render distance here.
            for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; ++dz) {
                for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; ++dx) {
                    int cx = playerChunk.x + dx; // chunk index in X
                    int cz = playerChunk.y + dz; // chunk index in Z (we used ivec2: x,y)
                    ChunkKey key{cx, cz};

                    if (loadedChunks.find(key) != loadedChunks.end()) continue; // already loaded

                    // Request chunk using chunk indices (server expects chunk indices)
                    auto chunkData = client.requestChunk((uint32_t)cx, 0u, (uint32_t)cz);
                    if (chunkData.blocks.empty()) {
                        std::cerr << "Client: empty chunk received for (" << cx << "," << cz << ")\n";
                        continue;
                    }

                    // Build mesh from chunk bytes (mesh positions are local to chunk: 0..CHUNK_SIZE-1)
                    auto verts = buildChunkMesh(chunkData.blocks.data(), chunkData.width, chunkData.height, chunkData.depth);

                    // offset to world coordinates: chunk index * CHUNK_SIZE
                    glm::vec3 worldOffset((float)cx * (float)CHUNK_SIZE, 0.0f, (float)cz * (float)CHUNK_SIZE);
                    offsetMesh(verts, worldOffset);

                    loadedChunks.emplace(key, std::move(verts));
                    std::cout << "Loaded chunk [" << cx << "," << cz << "]\n";
                }
            }

            // combine loaded chunk meshes into one big mesh for renderer
            std::vector<Vertex> combined;
            combined.reserve(loadedChunks.size() * 1000); // heuristic reserve
            for (auto& kv : loadedChunks) {
                auto& v = kv.second;
                combined.insert(combined.end(), v.begin(), v.end());
            }

            renderer.setMesh(combined);
            lastPlayerChunk = playerChunk;
            std::cout << "Rebuilt combined mesh around chunk " << playerChunk.x << ", " << playerChunk.y << std::endl;
        }

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.setView(player.camera.getViewMatrix());
        renderer.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    client.disconnect();
    server->stop();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "Program finished" << std::endl;
    return 0;
}
