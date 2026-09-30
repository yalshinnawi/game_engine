#include "world.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace voxel {

World::World(int seed, int renderDistance)
    : m_seed(seed), m_renderDistance(renderDistance) {}

void World::update(const glm::vec3& playerPosition) {
    int playerChunkX = static_cast<int>(std::floor(playerPosition.x / CHUNK_SIZE_X));
    int playerChunkZ = static_cast<int>(std::floor(playerPosition.z / CHUNK_SIZE_Z));

    if (playerChunkX == m_lastPlayerChunkX && playerChunkZ == m_lastPlayerChunkZ) {
        for (auto& [key, chunk] : m_chunks) {
            if (chunk->isDirty()) {
                chunk->buildMesh(this);
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

BlockType World::getBlock(int wx, int wy, int wz) const {
    if (wy < 0 || wy >= CHUNK_SIZE_Y) return BlockType::AIR;
    int cx = static_cast<int>(std::floor(static_cast<float>(wx) / CHUNK_SIZE_X));
    int cz = static_cast<int>(std::floor(static_cast<float>(wz) / CHUNK_SIZE_Z));

    auto it = m_chunks.find({cx, cz});
    if (it == m_chunks.end()) return BlockType::AIR;

    int lx = wx - cx * CHUNK_SIZE_X;
    int lz = wz - cz * CHUNK_SIZE_Z;
    return it->second->getBlock(lx, wy, lz);
}

void World::setBlock(int wx, int wy, int wz, BlockType type) {
    if (wy < 0 || wy >= CHUNK_SIZE_Y) return;
    int cx = static_cast<int>(std::floor(static_cast<float>(wx) / CHUNK_SIZE_X));
    int cz = static_cast<int>(std::floor(static_cast<float>(wz) / CHUNK_SIZE_Z));

    auto it = m_chunks.find({cx, cz});
    if (it != m_chunks.end()) {
        int lx = wx - cx * CHUNK_SIZE_X;
        int lz = wz - cz * CHUNK_SIZE_Z;
        it->second->setBlock(lx, wy, lz, type);
        it->second->buildMesh(this);

        // If on chunk border, rebuild neighboring chunk mesh
        if (lx == 0) rebuildChunkAt(cx - 1, cz);
        if (lx == CHUNK_SIZE_X - 1) rebuildChunkAt(cx + 1, cz);
        if (lz == 0) rebuildChunkAt(cx, cz - 1);
        if (lz == CHUNK_SIZE_Z - 1) rebuildChunkAt(cx, cz + 1);
    }
}

bool World::isSolidAt(int wx, int wy, int wz) const {
    BlockType b = getBlock(wx, wy, wz);
    return b != BlockType::AIR && b != BlockType::WATER;
}

int World::getHighestBlock(int wx, int wz) const {
    for (int y = CHUNK_SIZE_Y - 1; y >= 0; y--) {
        if (isSolidAt(wx, y, wz)) {
            return y;
        }
    }
    return 60; // Fallback
}

bool World::raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist,
                    glm::ivec3& outBlock, glm::ivec3& outNormal) const {
    glm::vec3 rayDir = glm::normalize(dir);

    int x = static_cast<int>(std::floor(origin.x));
    int y = static_cast<int>(std::floor(origin.y));
    int z = static_cast<int>(std::floor(origin.z));

    int stepX = (rayDir.x > 0) ? 1 : ((rayDir.x < 0) ? -1 : 0);
    int stepY = (rayDir.y > 0) ? 1 : ((rayDir.y < 0) ? -1 : 0);
    int stepZ = (rayDir.z > 0) ? 1 : ((rayDir.z < 0) ? -1 : 0);

    float tDeltaX = (stepX != 0) ? std::abs(1.0f / rayDir.x) : 1e30f;
    float tDeltaY = (stepY != 0) ? std::abs(1.0f / rayDir.y) : 1e30f;
    float tDeltaZ = (stepZ != 0) ? std::abs(1.0f / rayDir.z) : 1e30f;

    float tMaxX = (stepX > 0) ? (x + 1.0f - origin.x) * tDeltaX : (origin.x - x) * tDeltaX;
    float tMaxY = (stepY > 0) ? (y + 1.0f - origin.y) * tDeltaY : (origin.y - y) * tDeltaY;
    float tMaxZ = (stepZ > 0) ? (z + 1.0f - origin.z) * tDeltaZ : (origin.z - z) * tDeltaZ;

    glm::ivec3 normal(0);
    float distance = 0.0f;

    while (distance <= maxDist) {
        if (isSolidAt(x, y, z)) {
            outBlock = glm::ivec3(x, y, z);
            outNormal = normal;
            return true;
        }

        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                distance = tMaxX;
                tMaxX += tDeltaX;
                x += stepX;
                normal = glm::ivec3(-stepX, 0, 0);
            } else {
                distance = tMaxZ;
                tMaxZ += tDeltaZ;
                z += stepZ;
                normal = glm::ivec3(0, 0, -stepZ);
            }
        } else {
            if (tMaxY < tMaxZ) {
                distance = tMaxY;
                tMaxY += tDeltaY;
                y += stepY;
                normal = glm::ivec3(0, -stepY, 0);
            } else {
                distance = tMaxZ;
                tMaxZ += tDeltaZ;
                z += stepZ;
                normal = glm::ivec3(0, 0, -stepZ);
            }
        }
    }

    return false;
}

void World::loadChunksAroundPlayer(int playerChunkX, int playerChunkZ) {
    for (int dx = -m_renderDistance; dx <= m_renderDistance; dx++) {
        for (int dz = -m_renderDistance; dz <= m_renderDistance; dz++) {
            if (dx * dx + dz * dz > m_renderDistance * m_renderDistance) continue;

            int cx = playerChunkX + dx;
            int cz = playerChunkZ + dz;
            auto key = std::make_pair(cx, cz);

            if (m_chunks.find(key) == m_chunks.end()) {
                auto chunk = std::make_unique<Chunk>(cx, cz);
                chunk->generateTerrain(m_seed);
                chunk->buildMesh(this);
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

void World::rebuildChunkAt(int chunkX, int chunkZ) {
    auto it = m_chunks.find({chunkX, chunkZ});
    if (it != m_chunks.end()) {
        it->second->buildMesh(this);
    }
}

} // namespace voxel