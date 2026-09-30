#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <cstdint>

namespace voxel {

class World;

// Block type enumeration
enum class BlockType : uint8_t {
    AIR = 0,
    GRASS,
    DIRT,
    STONE,
    SAND,
    WATER,
    WOOD,
    LEAVES,
    BEDROCK,
    SNOW,
    COUNT
};

// Vertex data: position (3) + normal (3) + texcoord (2) + block type (1)
struct BlockVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    float     blockType;
};

// Chunk dimensions
constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Y = 128;
constexpr int CHUNK_SIZE_Z = 16;

class Chunk {
public:
    Chunk(int chunkX, int chunkZ);
    ~Chunk();

    // Non-copyable, movable
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;
    Chunk(Chunk&& other) noexcept;
    Chunk& operator=(Chunk&& other) noexcept;

    void generateTerrain(int seed);
    void buildMesh(const World* world = nullptr);
    void render() const;

    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);

    int getChunkX() const { return m_chunkX; }
    int getChunkZ() const { return m_chunkZ; }
    glm::vec3 getWorldPosition() const;
    int getTriangleCount() const { return m_indexCount / 3; }
    bool isDirty() const { return m_dirty; }

private:
    int m_chunkX, m_chunkZ;
    std::array<BlockType, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z> m_blocks;

    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    int m_indexCount = 0;
    bool m_dirty = true;
    bool m_meshBuilt = false;

    int blockIndex(int x, int y, int z) const;
    bool isBlockSolid(int x, int y, int z, const World* world) const;
    void uploadMesh(const std::vector<BlockVertex>& vertices, const std::vector<GLuint>& indices);
    void cleanup();
};

} // namespace voxel