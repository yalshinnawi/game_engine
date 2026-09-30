/*
 * VoxelEngine - Minecraft-inspired Voxel Engine
 * Built with C++20 and OpenGL 4.3 Core
 * 
 * Features:
 *   - Survival Walking Mode with Gravity, Jump & Block Collisions
 *   - Flight / Creative Mode toggle (F key or Pause Menu)
 *   - Left Click: Break Block
 *   - Right Click: Place Block (1-5 to select block type)
 *   - Escape: In-game Pause Menu with interactive buttons
 *   - Window Resizing: Fully dynamic and crash-proof
 *   - Crosshair: Center screen aiming
 *   - F1: Wireframe toggle
 *   - F5 / R: Hot-reload shaders
 *   - Q: Quit
 */

#include "window.h"
#include "shader.h"
#include "camera.h"
#include "world.h"
#include "renderer.h"
#include "input.h"
#include "player.h"
#include "ui.h"

#include <iostream>
#include <sstream>

int main() {
    try {
        // ── Window (1280x720 default, fully resizable) ──
        voxel::Window window(1280, 720, "VoxelEngine");

        // ── Camera ──
        voxel::Camera camera(glm::vec3(0.0f, 60.0f, 0.0f));

        // ── Input ──
        voxel::Input input(window, camera);

        // ── World ──
        voxel::World world(42, 6);

        // ── Renderer ──
        voxel::Renderer renderer;
        renderer.init("shaders");

        // ── UI Renderer (Crosshair & Pause Menu) ──
        voxel::UIRenderer uiRenderer;
        uiRenderer.init("shaders");

        // Initial world pre-generation around spawn
        world.update(glm::vec3(0.0f, 50.0f, 0.0f));

        // Spawn player on ground surface
        int spawnGroundY = world.getHighestBlock(0, 0);
        float spawnY = static_cast<float>(spawnGroundY + 1);
        voxel::Player player(glm::vec3(0.5f, spawnY, 0.5f));

        // Start in Survival Walking Mode!
        player.setFlying(false);
        camera.setPosition(player.getEyePosition());

        // State
        bool isPaused = false;
        float deltaTime = 0.0f;
        float lastFrame = 0.0f;
        int frameCount = 0;
        float fpsTimer = 0.0f;
        float currentFPS = 0.0f;

        std::cout << "\n=== VoxelEngine Running ===" << std::endl;
        std::cout << "SURVIVAL MODE (Default):" << std::endl;
        std::cout << "  WASD: Walk | Space: Jump | Ctrl: Sprint | F: Toggle Flight" << std::endl;
        std::cout << "  Left Click: Break Block | Right Click: Place Block (1-5 to switch block)" << std::endl;
        std::cout << "  Escape: Open Pause Menu | F1: Wireframe | Q: Quit\n" << std::endl;

        // ── Main Loop ──
        while (!window.shouldClose()) {
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            if (deltaTime > 0.1f) deltaTime = 0.1f; // Clamp delta time

            // Poll events
            window.pollEvents();

            // FPS Counter in window title
            frameCount++;
            fpsTimer += deltaTime;
            if (fpsTimer >= 1.0f) {
                currentFPS = static_cast<float>(frameCount) / fpsTimer;
                std::ostringstream title;
                title << "VoxelEngine | " << (player.isFlying() ? "CREATIVE (FLY)" : "SURVIVAL (WALK)")
                      << " | FPS: " << static_cast<int>(currentFPS)
                      << " | Chunks: " << world.getChunkCount()
                      << " | Pos: ("
                      << static_cast<int>(player.getPosition().x) << ", "
                      << static_cast<int>(player.getPosition().y) << ", "
                      << static_cast<int>(player.getPosition().z) << ")";
                glfwSetWindowTitle(window.getHandle(), title.str().c_str());
                frameCount = 0;
                fpsTimer = 0.0f;
            }

            // ── Escape: Toggle Pause Menu ──
            if (input.wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
                isPaused = !isPaused;
                input.setCursorCaptured(!isPaused);
            }

            // Quick quit
            if (input.wasKeyJustPressed(GLFW_KEY_Q) && isPaused) {
                break;
            }

            // ── In-Game Logic ──
            if (!isPaused) {
                // Toggle flight mode with F
                if (input.wasKeyJustPressed(GLFW_KEY_F)) {
                    player.toggleFlying();
                    std::cout << "[Engine] Mode: " << (player.isFlying() ? "FLYING / CREATIVE" : "SURVIVAL / WALKING") << std::endl;
                }

                // Hot-reload shaders
                if (input.wasKeyJustPressed(GLFW_KEY_R) || input.wasKeyJustPressed(GLFW_KEY_F5)) {
                    renderer.getChunkShader().reload();
                    uiRenderer.init("shaders");
                    std::cout << "[Engine] Shaders reloaded!" << std::endl;
                }

                // Toggle wireframe
                if (input.wasKeyJustPressed(GLFW_KEY_F1)) {
                    renderer.setWireframe(!renderer.isWireframe());
                }

                // Select block type with keys 1..5 or mouse wheel
                if (input.wasKeyJustPressed(GLFW_KEY_1)) player.setSelectedBlock(voxel::BlockType::DIRT);
                if (input.wasKeyJustPressed(GLFW_KEY_2)) player.setSelectedBlock(voxel::BlockType::STONE);
                if (input.wasKeyJustPressed(GLFW_KEY_3)) player.setSelectedBlock(voxel::BlockType::WOOD);
                if (input.wasKeyJustPressed(GLFW_KEY_4)) player.setSelectedBlock(voxel::BlockType::SAND);
                if (input.wasKeyJustPressed(GLFW_KEY_5)) player.setSelectedBlock(voxel::BlockType::LEAVES);

                int scrollDelta = input.getScrollDelta();
                if (scrollDelta != 0) {
                    player.cycleSelectedBlock(scrollDelta > 0 ? 1 : -1);
                }

                // Update player physics (Walking, Gravity, Jump, Collisions)
                player.update(deltaTime, world, camera, input);

                // Update world chunks around player
                world.update(player.getPosition());

                // ── Block Interaction (Break & Place) ──
                glm::ivec3 hitBlock(0), hitNormal(0);
                bool hasTarget = world.raycast(camera.getPosition(), camera.getFront(), 5.0f, hitBlock, hitNormal);

                // Left click: Break Block
                if (hasTarget && input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                    world.setBlock(hitBlock.x, hitBlock.y, hitBlock.z, voxel::BlockType::AIR);
                    std::cout << "[Voxel] Broke block at (" << hitBlock.x << ", " << hitBlock.y << ", " << hitBlock.z << ")" << std::endl;
                }

                // Right click: Place Block
                if (hasTarget && input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
                    glm::ivec3 placePos = hitBlock + hitNormal;

                    // Ensure not placing inside player's body
                    float halfW = player.getWidth() * 0.5f;
                    glm::vec3 pMin = player.getPosition() - glm::vec3(halfW, 0.0f, halfW);
                    glm::vec3 pMax = player.getPosition() + glm::vec3(halfW, player.getHeight(), halfW);

                    bool insidePlayer = (placePos.x >= std::floor(pMin.x) && placePos.x <= std::floor(pMax.x) &&
                                         placePos.y >= std::floor(pMin.y) && placePos.y <= std::floor(pMax.y) &&
                                         placePos.z >= std::floor(pMin.z) && placePos.z <= std::floor(pMax.z));

                    if (!insidePlayer) {
                        world.setBlock(placePos.x, placePos.y, placePos.z, player.getSelectedBlock());
                        std::cout << "[Voxel] Placed block at (" << placePos.x << ", " << placePos.y << ", " << placePos.z << ")" << std::endl;
                    }
                }
            } else {
                // ── Pause Menu Interaction ──
                int clickedBtn = -1;
                if (input.wasMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
                    clickedBtn = uiRenderer.getClickedMenuButton(window.getWidth(), window.getHeight(),
                                                                 input.getMouseX(), input.getMouseY());
                }

                // Keyboard options in pause menu
                if (input.wasKeyJustPressed(GLFW_KEY_ENTER) || clickedBtn == 0) {
                    // Resume Game
                    isPaused = false;
                    input.setCursorCaptured(true);
                } else if (input.wasKeyJustPressed(GLFW_KEY_M) || clickedBtn == 1) {
                    // Toggle Game Mode
                    player.toggleFlying();
                } else if (clickedBtn == 2) {
                    // Quit to Desktop
                    break;
                }
            }

            // ── Render 3D World ──
            if (window.getWidth() > 0 && window.getHeight() > 0) {
                renderer.beginFrame(camera, window.getAspectRatio());
                renderer.renderWorld(world);
                renderer.endFrame();

                // ── Render 2D UI ──
                uiRenderer.begin(window.getWidth(), window.getHeight());
                if (!isPaused) {
                    // Crosshair in center of screen
                    uiRenderer.drawCrosshair(window.getWidth(), window.getHeight());
                } else {
                    // Pause Menu
                    int hovered = -1;
                    uiRenderer.drawPauseMenu(window.getWidth(), window.getHeight(), player.isFlying(),
                                            input.getMouseX(), input.getMouseY(), hovered);
                }
                uiRenderer.end();

                window.swapBuffers();
            }

            // Clear frame input flags
            input.update();
        }

        std::cout << "\n[Engine] Shutting down cleanly..." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}