#include "chunk.h"
#include "world.h"
#include <FastNoiseLite.h>
#include <cmath>
#include <algorithm>
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

            float continent = continentNoise.GetNoise(wx, wz);
            float detail = detailNoise.GetNoise(wx, wz);

            float heightNorm = (continent * 0.7f + detail * 0.3f + 1.0f) * 0.5f;
            int terrainHeight = static_cast<int>(heightNorm * 40.0f) + 40;
            terrainHeight = std::clamp(terrainHeight, 1, CHUNK_SIZE_Y - 1);

            int waterLevel = 52;

            for (int y = 0; y < CHUNK_SIZE_Y; y++) {
                if (y == 0) {
                    setBlock(x, y, z, BlockType::BEDROCK);
                    continue;
                }

                if (y < terrainHeight) {
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
                        if (terrainHeight > 72) {
                            setBlock(x, y, z, BlockType::SNOW);
                        } else if (terrainHeight < waterLevel + 2) {
                            setBlock(x, y, z, BlockType::SAND);
                        } else {
                            setBlock(x, y, z, BlockType::GRASS);
                        }
                    }
                } else if (y < waterLevel) {
                    setBlock(x, y, z, BlockType::WATER);
                }
            }
        }
    }
    m_dirty = true;
}

void Chunk::buildMesh(const World* world) {
    std::vector<BlockVertex> vertices;
    std::vector<GLuint> indices;

    vertices.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Z * 24);
    indices.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Z * 36);

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                BlockType block = getBlock(x, y, z);
                if (block == BlockType::AIR) continue;

                float x0 = static_cast<float>(m_chunkX * CHUNK_SIZE_X + x);
                float x1 = x0 + 1.0f;
                float y0 = static_cast<float>(y);
                float y1 = y0 + 1.0f;
                float z0 = static_cast<float>(m_chunkZ * CHUNK_SIZE_Z + z);
                float z1 = z0 + 1.0f;
                float bt = static_cast<float>(block);

                auto appendQuad = [&](const BlockVertex& v0, const BlockVertex& v1,
                                     const BlockVertex& v2, const BlockVertex& v3) {
                    GLuint base = static_cast<GLuint>(vertices.size());
                    vertices.push_back(v0);
                    vertices.push_back(v1);
                    vertices.push_back(v2);
                    vertices.push_back(v3);

                    // Counter-clockwise winding: 0-1-2 and 0-2-3
                    indices.push_back(base + 0);
                    indices.push_back(base + 1);
                    indices.push_back(base + 2);
                    indices.push_back(base + 0);
                    indices.push_back(base + 2);
                    indices.push_back(base + 3);
                };

                // 1. TOP (+Y)
                if (!isBlockSolid(x, y + 1, z, world)) {
                    glm::vec3 n(0, 1, 0);
                    appendQuad(
                        BlockVertex{{x0, y1, z1}, n, {0.0f, 0.0f}, bt},
                        BlockVertex{{x1, y1, z1}, n, {1.0f, 0.0f}, bt},
                        BlockVertex{{x1, y1, z0}, n, {1.0f, 1.0f}, bt},
                        BlockVertex{{x0, y1, z0}, n, {0.0f, 1.0f}, bt}
                    );
                }

                // 2. BOTTOM (-Y)
                if (!isBlockSolid(x, y - 1, z, world)) {
                    glm::vec3 n(0, -1, 0);
                    appendQuad(
                        BlockVertex{{x0, y0, z0}, n, {0.0f, 0.0f}, bt},
                        BlockVertex{{x1, y0, z0}, n, {1.0f, 0.0f}, bt},
                        BlockVertex{{x1, y0, z1}, n, {1.0f, 1.0f}, bt},
                        BlockVertex{{x0, y0, z1}, n, {0.0f, 1.0f}, bt}
                    );
                }

                // 3. FRONT (+Z)
                if (!isBlockSolid(x, y, z + 1, world)) {
                    glm::vec3 n(0, 0, 1);
                    appendQuad(
                        BlockVertex{{x0, y0, z1}, n, {0.0f, 0.0f}, bt},
                        BlockVertex{{x1, y0, z1}, n, {1.0f, 0.0f}, bt},
                        BlockVertex{{x1, y1, z1}, n, {1.0f, 1.0f}, bt},
                        BlockVertex{{x0, y1, z1}, n, {0.0f, 1.0f}, bt}
                    );
                }

                // 4. BACK (-Z)
                if (!isBlockSolid(x, y, z - 1, world)) {
                    glm::vec3 n(0, 0, -1);
                    appendQuad(
                        BlockVertex{{x1, y0, z0}, n, {0.0f, 0.0f}, bt},
                        BlockVertex{{x0, y0, z0}, n, {1.0f, 0.0f}, bt},
                        BlockVertex{{x0, y1, z0}, n, {1.0f, 1.0f}, bt},
                        BlockVertex{{x1, y1, z0}, n, {0.0f, 1.0f}, bt}
                    );
                }

                // 5. RIGHT (+X)
                if (!isBlockSolid(x + 1, y, z, world)) {
                    glm::vec3 n(1, 0, 0);
                    appendQuad(
                        BlockVertex{{x1, y0, z1}, n, {0.0f, 0.0f}, bt},
                        BlockVertex{{x1, y0, z0}, n, {1.0f, 0.0f}, bt},
                        BlockVertex{{x1, y1, z0}, n, {1.0f, 1.0f}, bt},
                        BlockVertex{{x1, y1, z1}, n, {0.0f, 1.0f}, bt}
                    );
                }

                // 6. LEFT (-X)
                if (!isBlockSolid(x - 1, y, z, world)) {
                    glm::vec3 n(-1, 0, 0);
                    appendQuad(
                        BlockVertex{{x0, y0, z0}, n, {0.0f, 0.0f}, bt},
                        BlockVertex{{x0, y0, z1}, n, {1.0f, 0.0f}, bt},
                        BlockVertex{{x0, y1, z1}, n, {1.0f, 1.0f}, bt},
                        BlockVertex{{x0, y1, z0}, n, {0.0f, 1.0f}, bt}
                    );
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

bool Chunk::isBlockSolid(int x, int y, int z, const World* world) const {
    if (x >= 0 && x < CHUNK_SIZE_X && y >= 0 && y < CHUNK_SIZE_Y && z >= 0 && z < CHUNK_SIZE_Z) {
        BlockType b = m_blocks[blockIndex(x, y, z)];
        return b != BlockType::AIR && b != BlockType::WATER;
    }
    if (y < 0 || y >= CHUNK_SIZE_Y) return false;
    if (world) {
        int wx = m_chunkX * CHUNK_SIZE_X + x;
        int wz = m_chunkZ * CHUNK_SIZE_Z + z;
        BlockType b = world->getBlock(wx, y, wz);
        return b != BlockType::AIR && b != BlockType::WATER;
    }
    return false;
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