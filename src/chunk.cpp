#include "chunk.h"
#include <FastNoiseLite.h>
#include <cmath>
#include <iostream>

namespace voxel {

Chunk::Chunk(int chunkX, int chunkZ)
    : m_chunkX(chunkX), m_chunkZ(chunkZ) {
    m_blocks.fill(BlockType::AIR);
}

Chunk::~Chunk() {
    cleanup();
}

Chunk::Chunk(Chunk&& other) noexcept
    : m_chunkX(other.m_chunkX), m_chunkZ(other.m_chunkZ),
      m_blocks(std::move(other.m_blocks)),
      m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo),
      m_indexCount(other.m_indexCount), m_dirty(other.m_dirty),
      m_meshBuilt(other.m_meshBuilt) {
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_chunkX = other.m_chunkX;
        m_chunkZ = other.m_chunkZ;
        m_blocks = std::move(other.m_blocks);
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;
        m_dirty = other.m_dirty;
        m_meshBuilt = other.m_meshBuilt;
        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
    }
    return *this;
}

void Chunk::generateTerrain(int seed) {
    FastNoiseLite continentNoise;
    continentNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    continentNoise.SetSeed(seed);
    continentNoise.SetFrequency(0.005f);
    continentNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    continentNoise.SetFractalOctaves(4);

    FastNoiseLite detailNoise;
    detailNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    detailNoise.SetSeed(seed + 1);
    detailNoise.SetFrequency(0.02f);
    detailNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    detailNoise.SetFractalOctaves(3);

    FastNoiseLite caveNoise;
    caveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    caveNoise.SetSeed(seed + 2);
    caveNoise.SetFrequency(0.04f);

    int worldX = m_chunkX * CHUNK_SIZE_X;
    int worldZ = m_chunkZ * CHUNK_SIZE_Z;

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            float wx = static_cast<float>(worldX + x);
            float wz = static_cast<float>(worldZ + z);

            // Multi-octave terrain height
            float continent = continentNoise.GetNoise(wx, wz);
            float detail = detailNoise.GetNoise(wx, wz);

            // Combine noise layers for varied terrain
            float heightNorm = (continent * 0.7f + detail * 0.3f + 1.0f) * 0.5f;
            int terrainHeight = static_cast<int>(heightNorm * 40.0f) + 40;
            terrainHeight = std::clamp(terrainHeight, 1, CHUNK_SIZE_Y - 1);

            int waterLevel = 52;

            for (int y = 0; y < CHUNK_SIZE_Y; y++) {
                // Bedrock layer
                if (y == 0) {
                    setBlock(x, y, z, BlockType::BEDROCK);
                    continue;
                }

                // Below terrain surface
                if (y < terrainHeight) {
                    // Check for caves
                    float caveVal = caveNoise.GetNoise(wx, static_cast<float>(y), wz);
                    if (caveVal > 0.6f && y > 5 && y < terrainHeight - 3) {
                        setBlock(x, y, z, BlockType::AIR);
                        continue;
                    }

                    if (y < terrainHeight - 4) {
                        setBlock(x, y, z, BlockType::STONE);
                    } else if (y < terrainHeight - 1) {
                        setBlock(x, y, z, BlockType::DIRT);
                    } else {
                        // Surface block
                        if (terrainHeight > 72) {
                            setBlock(x, y, z, BlockType::SNOW);
                        } else if (terrainHeight < waterLevel + 2) {
                            setBlock(x, y, z, BlockType::SAND);
                        } else {
                            setBlock(x, y, z, BlockType::GRASS);
                        }
                    }
                }
                // Water fill
                else if (y < waterLevel) {
                    setBlock(x, y, z, BlockType::WATER);
                }
            }
        }
    }
    m_dirty = true;
}

void Chunk::buildMesh() {
    std::vector<BlockVertex> vertices;
    std::vector<GLuint> indices;

    // Reserve rough estimate
    vertices.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Z * 24);
    indices.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Z * 36);

    // Face normals and tangent vectors
    const glm::vec3 normals[] = {
        { 0,  1,  0}, // Top
        { 0, -1,  0}, // Bottom
        { 0,  0,  1}, // Front (+Z)
        { 0,  0, -1}, // Back (-Z)
        { 1,  0,  0}, // Right (+X)
        {-1,  0,  0}, // Left (-X)
    };

    // Right and Up vectors for each face
    const glm::vec3 rights[] = {
        { 1, 0,  0}, // Top
        { 1, 0,  0}, // Bottom
        { 1, 0,  0}, // Front
        {-1, 0,  0}, // Back
        { 0, 0, -1}, // Right
        { 0, 0,  1}, // Left
    };
    const glm::vec3 ups[] = {
        {0, 0, 1},  // Top
        {0, 0, 1},  // Bottom
        {0, 1, 0},  // Front
        {0, 1, 0},  // Back
        {0, 1, 0},  // Right
        {0, 1, 0},  // Left
    };

    // Neighbor offsets matching normals order
    const int dx[] = { 0,  0,  0,  0,  1, -1};
    const int dy[] = { 1, -1,  0,  0,  0,  0};
    const int dz[] = { 0,  0,  1, -1,  0,  0};

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                BlockType block = getBlock(x, y, z);
                if (block == BlockType::AIR) continue;

                glm::vec3 pos(
                    static_cast<float>(m_chunkX * CHUNK_SIZE_X + x),
                    static_cast<float>(y),
                    static_cast<float>(m_chunkZ * CHUNK_SIZE_Z + z)
                );

                // Check each face
                for (int face = 0; face < 6; face++) {
                    int nx = x + dx[face];
                    int ny = y + dy[face];
                    int nz = z + dz[face];

                    if (!isBlockSolid(nx, ny, nz)) {
                        addFace(vertices, indices, pos, normals[face],
                                rights[face], ups[face], block);
                    }
                }
            }
        }
    }

    uploadMesh(vertices, indices);
    m_dirty = false;
    m_meshBuilt = true;
}

void Chunk::render() const {
    if (!m_meshBuilt || m_indexCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return BlockType::AIR;
    }
    return m_blocks[blockIndex(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (x < 0 || x >= CHUNK_SIZE_X || y < 0 || y >= CHUNK_SIZE_Y || z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    m_blocks[blockIndex(x, y, z)] = type;
    m_dirty = true;
}

glm::vec3 Chunk::getWorldPosition() const {
    return glm::vec3(
        static_cast<float>(m_chunkX * CHUNK_SIZE_X),
        0.0f,
        static_cast<float>(m_chunkZ * CHUNK_SIZE_Z)
    );
}

int Chunk::blockIndex(int x, int y, int z) const {
    return y * CHUNK_SIZE_X * CHUNK_SIZE_Z + z * CHUNK_SIZE_X + x;
}

bool Chunk::isBlockSolid(int x, int y, int z) const {
    BlockType b = getBlock(x, y, z);
    return b != BlockType::AIR && b != BlockType::WATER;
}

void Chunk::addFace(std::vector<BlockVertex>& vertices, std::vector<GLuint>& indices,
                    const glm::vec3& pos, const glm::vec3& normal,
                    const glm::vec3& right, const glm::vec3& up,
                    BlockType type) {

    GLuint baseIndex = static_cast<GLuint>(vertices.size());

    float blockTypeF = static_cast<float>(type);

    // Offset position to face center, then build 4 corners
    glm::vec3 faceCenter = pos + normal * 0.5f;

    glm::vec3 halfRight = right * 0.5f;
    glm::vec3 halfUp = up * 0.5f;

    // 4 vertices of the face quad
    vertices.push_back({faceCenter - halfRight - halfUp + glm::vec3(0.5f), normal, {0.0f, 0.0f}, blockTypeF});
    vertices.push_back({faceCenter + halfRight - halfUp + glm::vec3(0.5f), normal, {1.0f, 0.0f}, blockTypeF});
    vertices.push_back({faceCenter + halfRight + halfUp + glm::vec3(0.5f), normal, {1.0f, 1.0f}, blockTypeF});
    vertices.push_back({faceCenter - halfRight + halfUp + glm::vec3(0.5f), normal, {0.0f, 1.0f}, blockTypeF});

    // Two triangles per face
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

void Chunk::uploadMesh(const std::vector<BlockVertex>& vertices, const std::vector<GLuint>& indices) {
    cleanup();

    m_indexCount = static_cast<int>(indices.size());
    if (m_indexCount == 0) return;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(BlockVertex)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
                 indices.data(), GL_STATIC_DRAW);

    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex),
                          reinterpret_cast<void*>(offsetof(BlockVertex, position)));
    glEnableVertexAttribArray(0);

    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BlockVertex),
                          reinterpret_cast<void*>(offsetof(BlockVertex, normal)));
    glEnableVertexAttribArray(1);

    // TexCoord (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(BlockVertex),
                          reinterpret_cast<void*>(offsetof(BlockVertex, texCoord)));
    glEnableVertexAttribArray(2);

    // BlockType (location 3)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(BlockVertex),
                          reinterpret_cast<void*>(offsetof(BlockVertex, blockType)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

void Chunk::cleanup() {
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_ebo) { glDeleteBuffers(1, &m_ebo); m_ebo = 0; }
    m_indexCount = 0;
    m_meshBuilt = false;
}

} // namespace voxel
