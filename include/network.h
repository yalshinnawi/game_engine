#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <glm/glm.hpp>
#include "chunk.h"

namespace voxel {

enum class NetworkMode {
    OFFLINE,
    SERVER,
    CLIENT
};

enum class PacketType : uint8_t {
    CONNECT_REQ = 1,
    CONNECT_ACK = 2,
    PLAYER_TRANSFORM = 3,
    BLOCK_CHANGE = 4,
    PLAYER_DISCONNECT = 5
};

#pragma pack(push, 1)
struct PacketHeader {
    PacketType type;
    uint32_t senderId;
    uint32_t dataSize;
};

struct PlayerTransformData {
    uint32_t id;
    float x, y, z;
    float yaw, pitch;
};

struct BlockChangeData {
    int32_t x, y, z;
    uint8_t blockType;
};
#pragma pack(pop)

struct RemotePlayer {
    uint32_t id = 0;
    glm::vec3 position = glm::vec3(0.0f);
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct BlockEvent {
    int x, y, z;
    BlockType type;
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool startServer(uint16_t port = 25565);
    bool connectClient(const std::string& host = "127.0.0.1", uint16_t port = 25565);
    void disconnect();

    void broadcastTransform(const glm::vec3& pos, float yaw, float pitch);
    void broadcastBlockChange(int x, int y, int z, BlockType type);

    // Call from main game loop
    void poll(std::vector<RemotePlayer>& outPlayers, std::vector<BlockEvent>& outBlockChanges);

    NetworkMode getMode() const { return m_mode; }
    bool isConnected() const { return m_mode != NetworkMode::OFFLINE; }
    int getClientCount() const;
    uint32_t getLocalId() const { return m_localId; }

private:
    NetworkMode m_mode = NetworkMode::OFFLINE;
    uint32_t m_localId = 0;
    std::atomic<bool> m_running{false};

    std::thread m_networkThread;
    std::mutex m_mutex;

    // Incoming events for main thread
    std::unordered_map<uint32_t, RemotePlayer> m_remotePlayers;
    std::vector<BlockEvent> m_pendingBlockChanges;

    // Internal socket descriptors
    uint64_t m_serverSocket = ~0ULL;
    struct ClientNode {
        uint64_t socket;
        uint32_t id;
    };
    std::vector<ClientNode> m_serverClients;
    uint64_t m_clientSocket = ~0ULL;

    void serverLoop(uint16_t port);
    void clientLoop(std::string host, uint16_t port);
    void cleanupSockets();

    bool sendRaw(uint64_t sock, const void* data, int size);
};

} // namespace voxel