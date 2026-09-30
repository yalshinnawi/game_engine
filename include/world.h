#pragma once
#include "chunk.h"
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

namespace voxel {

// Hash for chunk coordinates
struct ChunkKeyHash {
    std::size_t operator()(const std::pair<int, int>& k) const {
        auto h1 = std::hash<int>{}(k.first);
        auto h2 = std::hash<int>{}(k.second);
        return h1 ^ (h2 << 16);
    }
};

class World {
public:
    World(int seed = 42, int renderDistance = 4);
    ~World() = default;

    void update(const glm::vec3& playerPosition);
    void render() const;

    int getSeed() const { return m_seed; }
    int getChunkCount() const { return static_cast<int>(m_chunks.size()); }
    int getTotalTriangles() const;

    void setRenderDistance(int distance) { m_renderDistance = distance; }
    int getRenderDistance() const { return m_renderDistance; }

    // Block query and modification
    BlockType getBlock(int wx, int wy, int wz) const;
    void setBlock(int wx, int wy, int wz, BlockType type);
    bool isSolidAt(int wx, int wy, int wz) const;
    int getHighestBlock(int wx, int wz) const;

    // Raycast for breaking/placing blocks
    bool raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist,
                 glm::ivec3& outBlock, glm::ivec3& outNormal) const;

private:
    int m_seed;
    int m_renderDistance;
    std::unordered_map<std::pair<int, int>, std::unique_ptr<Chunk>, ChunkKeyHash> m_chunks;

    int m_lastPlayerChunkX = INT_MAX;
    int m_lastPlayerChunkZ = INT_MAX;

    void loadChunksAroundPlayer(int playerChunkX, int playerChunkZ);
    void unloadDistantChunks(int playerChunkX, int playerChunkZ);
    void rebuildChunkAt(int chunkX, int chunkZ);
};

} // namespace voxel