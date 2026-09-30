/*
 * VoxelEngine - Minecraft-inspired Voxel Engine
 * Built with C++20 and OpenGL 4.3 Core
 * 
 * Features:
 *   - Multiplayer Networking: Host server or join client from any IP
 *   - Configurable server IP via server.txt or CLI argument (--join <IP>)
 *   - Synchronized Player Avatars (rendered 3D Minecraft character models)
 *   - Synchronized Block Breaking & Placing across the network
 *   - Survival Walking Mode with Gravity, Jump & Block Collisions
 *   - Flight / Creative Mode toggle (F key or Pause Menu)
 *   - Left Click: Break Block | Right Click: Place Block (1-5 to select block)
 *   - Escape: Pause Menu with interactive buttons (Host / Join / Mode / Quit)
 *   - Window Resizing: Crash-proof and dynamic
 *   - Crosshair: Center screen aiming
 *   - F1: Wireframe toggle | F5/R: Hot-reload shaders | Q: Quit
 */

#include "window.h"
#include "shader.h"
#include "camera.h"
#include "world.h"
#include "renderer.h"
#include "input.h"
#include "player.h"
#include "ui.h"
#include "network.h"
#include "avatar.h"

#include <iostream>
#include <sstream>
#include <fstream>

int main(int argc, char* argv[]) {
    try {
        // ── Window (1280x720, resizable) ──
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

        // ── Multiplayer System ──
        voxel::NetworkManager network;
        voxel::AvatarRenderer avatarRenderer;
        avatarRenderer.init("shaders");

        // Server IP configuration
        std::string targetIP = "127.0.0.1";
        uint16_t targetPort = 25565;

        // Read server.txt if available
        std::ifstream configFile("server.txt");
        if (configFile.is_open()) {
            std::string line;
            if (std::getline(configFile, line)) {
                size_t first = line.find_first_not_of(" \t\r\n");
                size_t last = line.find_last_not_of(" \t\r\n");
                if (first != std::string::npos && last != std::string::npos) {
                    targetIP = line.substr(first, (last - first + 1));
                }
            }
        } else {
            // Create default server.txt template
            std::ofstream outConfig("server.txt");
            outConfig << "127.0.0.1\n";
        }

        // Parse command line arguments
        bool autoHost = false;
        bool autoJoin = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--host" || arg == "-h") {
                autoHost = true;
            } else if ((arg == "--join" || arg == "--connect" || arg == "-c") && i + 1 < argc) {
                targetIP = argv[++i];
                autoJoin = true;
            }
        }

        if (autoHost) {
            network.startServer(targetPort);
        } else if (autoJoin) {
            network.connectClient(targetIP, targetPort);
        }

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
        float netBroadcastTimer = 0.0f;

        std::cout << "\n=== VoxelEngine Running ===" << std::endl;
        std::cout << "CONTROLS:" << std::endl;
        std::cout << "  WASD: Walk | Space: Jump | Ctrl: Sprint | F: Toggle Flight" << std::endl;
        std::cout << "  Left Click: Break Block | Right Click: Place Block (1-5 to switch block)" << std::endl;
        std::cout << "  H: Host Server (Port " << targetPort << ") | J: Join Server (" << targetIP << ")" << std::endl;
        std::cout << "  Escape: Open Pause Menu | F1: Wireframe | Q: Quit\n" << std::endl;

        // ── Main Loop ──
        while (!window.shouldClose()) {
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            if (deltaTime > 0.1f) deltaTime = 0.1f;

            // Poll GLFW events
            window.pollEvents();

            // ── Multiplayer Network Polling ──
            std::vector<voxel::RemotePlayer> remotePlayers;
            std::vector<voxel::BlockEvent> incomingBlocks;
            if (network.isConnected()) {
                network.poll(remotePlayers, incomingBlocks);

                // Apply incoming block changes from other players
                for (const auto& ev : incomingBlocks) {
                    world.setBlock(ev.x, ev.y, ev.z, ev.type);
                }

                // Broadcast local player transform ~30 times per second
                netBroadcastTimer += deltaTime;
                if (netBroadcastTimer >= 0.033f) {
                    netBroadcastTimer = 0.0f;
                    network.broadcastTransform(player.getPosition(), camera.getYaw(), camera.getPitch());
                }
            }

            // ── FPS & Status Title Bar ──
            frameCount++;
            fpsTimer += deltaTime;
            if (fpsTimer >= 1.0f) {
                currentFPS = static_cast<float>(frameCount) / fpsTimer;
                std::ostringstream title;
                title << "VoxelEngine | " << (player.isFlying() ? "CREATIVE" : "SURVIVAL")
                      << " | FPS: " << static_cast<int>(currentFPS);

                if (network.getMode() == voxel::NetworkMode::SERVER) {
                    title << " | [HOST: " << (network.getClientCount() + 1) << " Players]";
                } else if (network.getMode() == voxel::NetworkMode::CLIENT) {
                    title << " | [CLIENT: Connected #" << network.getLocalId() << "]";
                } else {
                    title << " | [OFFLINE]";
                }

                title << " | Pos: ("
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

            // ── In-Game Logic (When not paused) ──
            if (!isPaused) {
                // Hotkey H: Host server
                if (input.wasKeyJustPressed(GLFW_KEY_H)) {
                    if (!network.isConnected()) {
                        network.startServer(targetPort);
                        std::cout << "[Engine] Hosting server on port " << targetPort << "..." << std::endl;
                    }
                }

                // Hotkey J: Join target server
                if (input.wasKeyJustPressed(GLFW_KEY_J)) {
                    if (!network.isConnected()) {
                        std::ifstream reloadCfg("server.txt");
                        if (reloadCfg.is_open()) {
                            std::string line;
                            if (std::getline(reloadCfg, line)) {
                                size_t first = line.find_first_not_of(" \t\r\n");
                                size_t last = line.find_last_not_of(" \t\r\n");
                                if (first != std::string::npos && last != std::string::npos) {
                                    targetIP = line.substr(first, (last - first + 1));
                                }
                            }
                        }
                        network.connectClient(targetIP, targetPort);
                        std::cout << "[Engine] Joining server at " << targetIP << ":" << targetPort << "..." << std::endl;
                    }
                }

                // Toggle flight mode with F
                if (input.wasKeyJustPressed(GLFW_KEY_F)) {
                    player.toggleFlying();
                    std::cout << "[Engine] Mode: " << (player.isFlying() ? "FLYING / CREATIVE" : "SURVIVAL / WALKING") << std::endl;
                }

                // Hot-reload shaders
                if (input.wasKeyJustPressed(GLFW_KEY_R) || input.wasKeyJustPressed(GLFW_KEY_F5)) {
                    renderer.getChunkShader().reload();
                    uiRenderer.init("shaders");
                    avatarRenderer.init("shaders");
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
                    if (network.isConnected()) {
                        network.broadcastBlockChange(hitBlock.x, hitBlock.y, hitBlock.z, voxel::BlockType::AIR);
                    }
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
                        if (network.isConnected()) {
                            network.broadcastBlockChange(placePos.x, placePos.y, placePos.z, player.getSelectedBlock());
                        }
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

                if (input.wasKeyJustPressed(GLFW_KEY_ENTER) || clickedBtn == 0) {
                    // Button 0: Resume
                    isPaused = false;
                    input.setCursorCaptured(true);
                } else if (input.wasKeyJustPressed(GLFW_KEY_M) || clickedBtn == 1) {
                    // Button 1: Toggle Mode
                    player.toggleFlying();
                } else if (clickedBtn == 2) {
                    // Button 2: Host Server
                    if (network.getMode() != voxel::NetworkMode::SERVER) {
                        network.startServer(targetPort);
                    }
                } else if (clickedBtn == 3) {
                    // Button 3: Connect to Server (targetIP)
                    if (!network.isConnected()) {
                        // Re-check server.txt in case user updated it live
                        std::ifstream reloadCfg("server.txt");
                        if (reloadCfg.is_open()) {
                            std::string line;
                            if (std::getline(reloadCfg, line)) {
                                size_t first = line.find_first_not_of(" \t\r\n");
                                size_t last = line.find_last_not_of(" \t\r\n");
                                if (first != std::string::npos && last != std::string::npos) {
                                    targetIP = line.substr(first, (last - first + 1));
                                }
                            }
                        }
                        network.connectClient(targetIP, targetPort);
                    }
                } else if (input.wasKeyJustPressed(GLFW_KEY_Q) || clickedBtn == 4) {
                    // Button 4: Quit
                    break;
                }
            }

            // ── Render 3D World & Avatars ──
            if (window.getWidth() > 0 && window.getHeight() > 0) {
                renderer.beginFrame(camera, window.getAspectRatio());

                // 1. Render voxel terrain
                renderer.renderWorld(world);

                // 2. Render other connected players' 3D avatars
                if (!remotePlayers.empty()) {
                    avatarRenderer.render(camera, window.getAspectRatio(), remotePlayers,
                                          renderer.getSkyColor(), renderer.getFogDistance());
                }

                renderer.endFrame();

                // ── Render 2D UI ──
                uiRenderer.begin(window.getWidth(), window.getHeight());
                if (!isPaused) {
                    // Aim crosshair
                    uiRenderer.drawCrosshair(window.getWidth(), window.getHeight());
                } else {
                    // In-game Pause & Multiplayer Menu
                    int hovered = -1;
                    uiRenderer.drawPauseMenu(window.getWidth(), window.getHeight(), player.isFlying(),
                                            network.getMode(), network.getClientCount(), targetIP,
                                            input.getMouseX(), input.getMouseY(), hovered);
                }
                uiRenderer.end();

                window.swapBuffers();
            }

            // Clear frame input flags
            input.update();
        }

        std::cout << "\n[Engine] Shutting down cleanly..." << std::endl;
        network.disconnect();

    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}