#include "world.h"
#include <cmath>
#include <iostream>

namespace voxel {

World::World(int seed, int renderDistance)
    : m_seed(seed), m_renderDistance(renderDistance) {}

void World::update(const glm::vec3& playerPosition) {
    int playerChunkX = static_cast<int>(std::floor(playerPosition.x / CHUNK_SIZE_X));
    int playerChunkZ = static_cast<int>(std::floor(playerPosition.z / CHUNK_SIZE_Z));

    // Only update if the player moved to a new chunk
    if (playerChunkX == m_lastPlayerChunkX && playerChunkZ == m_lastPlayerChunkZ) {
        // Still rebuild any dirty chunks
        for (auto& [key, chunk] : m_chunks) {
            if (chunk->isDirty()) {
                chunk->buildMesh();
            }
        }
        return;
    }

    m_lastPlayerChunkX = playerChunkX;
    m_lastPlayerChunkZ = playerChunkZ;

    loadChunksAroundPlayer(playerChunkX, playerChunkZ);
    unloadDistantChunks(playerChunkX, playerChunkZ);
}

void World::render() const {
    for (const auto& [key, chunk] : m_chunks) {
        chunk->render();
    }
}

int World::getTotalTriangles() const {
    int total = 0;
    for (const auto& [key, chunk] : m_chunks) {
        total += chunk->getTriangleCount();
    }
    return total;
}

void World::loadChunksAroundPlayer(int playerChunkX, int playerChunkZ) {
    for (int dx = -m_renderDistance; dx <= m_renderDistance; dx++) {
        for (int dz = -m_renderDistance; dz <= m_renderDistance; dz++) {
            // Circular render distance
            if (dx * dx + dz * dz > m_renderDistance * m_renderDistance) continue;

            int cx = playerChunkX + dx;
            int cz = playerChunkZ + dz;
            auto key = std::make_pair(cx, cz);

            if (m_chunks.find(key) == m_chunks.end()) {
                auto chunk = std::make_unique<Chunk>(cx, cz);
                chunk->generateTerrain(m_seed);
                chunk->buildMesh();
                m_chunks[key] = std::move(chunk);
            }
        }
    }
}

void World::unloadDistantChunks(int playerChunkX, int playerChunkZ) {
    int unloadDistance = m_renderDistance + 2;

    auto it = m_chunks.begin();
    while (it != m_chunks.end()) {
        int dx = it->first.first - playerChunkX;
        int dz = it->first.second - playerChunkZ;
        if (dx * dx + dz * dz > unloadDistance * unloadDistance) {
            it = m_chunks.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace voxel
