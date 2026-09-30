/*
 * VoxelEngine - Main Entry Point
 * 
 * A Minecraft-inspired voxel engine built with OpenGL 4.3.
 * 
 * Controls:
 *   WASD       - Move
 *   Mouse      - Look around
 *   Space      - Fly up
 *   Shift      - Fly down
 *   Ctrl       - Sprint
 *   F1         - Toggle wireframe
 *   F5         - Hot-reload shaders
 *   R          - Reload shaders
 *   Escape     - Toggle cursor capture
 *   Q          - Quit
 */

#include "window.h"
#include "shader.h"
#include "camera.h"
#include "world.h"
#include "renderer.h"
#include "input.h"

#include <iostream>
#include <sstream>
#include <chrono>

int main() {
    try {
        // ── Create window ──
        voxel::Window window(1280, 720, "VoxelEngine");

        // ── Create camera (spawn above terrain) ──
        voxel::Camera camera(glm::vec3(0.0f, 80.0f, 0.0f));

        // ── Create input handler ──
        voxel::Input input(window.getHandle(), camera);

        // ── Create renderer ──
        voxel::Renderer renderer;
        renderer.init("shaders");

        // ── Create world ──
        voxel::World world(42, 6); // seed=42, renderDistance=6

        // ── Timing ──
        float deltaTime = 0.0f;
        float lastFrame = 0.0f;
        int frameCount = 0;
        float fpsTimer = 0.0f;
        float currentFPS = 0.0f;

        std::cout << "\n=== VoxelEngine Running ===" << std::endl;
        std::cout << "WASD to move, Mouse to look, Space/Shift for up/down" << std::endl;
        std::cout << "Ctrl to sprint, F1 wireframe, R reload shaders, Q quit\n" << std::endl;

        // ── Main loop ──
        while (!window.shouldClose()) {
            // Calculate delta time
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            // FPS counter
            frameCount++;
            fpsTimer += deltaTime;
            if (fpsTimer >= 1.0f) {
                currentFPS = static_cast<float>(frameCount) / fpsTimer;

                std::ostringstream title;
                title << "VoxelEngine | FPS: " << static_cast<int>(currentFPS)
                      << " | Chunks: " << world.getChunkCount()
                      << " | Triangles: " << world.getTotalTriangles()
                      << " | Pos: ("
                      << static_cast<int>(camera.getPosition().x) << ", "
                      << static_cast<int>(camera.getPosition().y) << ", "
                      << static_cast<int>(camera.getPosition().z) << ")";
                glfwSetWindowTitle(window.getHandle(), title.str().c_str());

                frameCount = 0;
                fpsTimer = 0.0f;
            }

            // Input
            window.pollEvents();
            input.processInput(deltaTime);

            // Toggle wireframe (F1)
            if (input.wasKeyJustPressed(GLFW_KEY_F1)) {
                renderer.setWireframe(!renderer.isWireframe());
                std::cout << "[Engine] Wireframe: " << (renderer.isWireframe() ? "ON" : "OFF") << std::endl;
            }

            // Hot-reload shaders (R or F5)
            if (input.wasKeyJustPressed(GLFW_KEY_R) || input.wasKeyJustPressed(GLFW_KEY_F5)) {
                renderer.getChunkShader().reload();
            }

            // Update world (load/unload chunks around camera)
            world.update(camera.getPosition());

            // Render
            renderer.beginFrame(camera, window.getAspectRatio());
            renderer.renderWorld(world);
            renderer.endFrame();

            window.swapBuffers();
        }

        std::cout << "\n[Engine] Shutting down..." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
