#include "network.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>

static void writeNetworkLog(const std::string& level, const std::string& msg, const std::string& advice = "") {
    try {
        std::ofstream logFile("network_log.txt", std::ios::app);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm_buf;
#ifdef _WIN32
            localtime_s(&tm_buf, &in_time_t);
#else
            localtime_r(&in_time_t, &tm_buf);
#endif
            logFile << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "] ["
                    << level << "] " << msg << "\n";
            if (!advice.empty()) {
                logFile << "  -> ADVICE: " << advice << "\n";
            }
            logFile.flush();
        }
    } catch (...) {}
}

static std::pair<std::string, std::string> translateWinsockError(int err) {
    switch (err) {
        case 10061: // WSAECONNREFUSED
            return {"Connection refused by target IP.",
                    "Host has not pressed [HOST SERVER] (hotkey H), or port 25565 is blocked by router/firewall."};
        case 10060: // WSAETIMEDOUT
            return {"Connection timed out (no response).",
                    "Windows Firewall on Host PC is blocking incoming traffic. Ensure VoxelEngine is allowed on both Private and Public networks."};
        case 10065: // WSAEHOSTUNREACH
            return {"Host unreachable.",
                    "The IP address in server.txt cannot be reached. If using Hamachi, verify both are in the same Hamachi room with a green dot."};
        case 10054: // WSAECONNRESET
            return {"Connection reset by host.",
                    "The host closed the game or terminated the server."};
        case 10049: // WSAEADDRNOTAVAIL
            return {"Invalid IP address.",
                    "Check server.txt syntax (should be an IPv4 like 25.x.x.x or 192.168.x.x with no extra characters)."};
        default:
            return {"Winsock error code: " + std::to_string(err),
                    "Check network connection and firewall settings."};
    }
}


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define closesocket close
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace voxel {

NetworkManager::NetworkManager() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

NetworkManager::~NetworkManager() {
    disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool NetworkManager::sendRaw(uint64_t sock, const void* data, int size) {
    if (sock == ~0ULL) return false;
    SOCKET s = static_cast<SOCKET>(sock);
    int sent = send(s, reinterpret_cast<const char*>(data), size, 0);
    return sent == size;
}

bool NetworkManager::startServer(uint16_t port) {
    disconnect();

    m_mode = NetworkMode::SERVER;
    m_connState = ConnectionState::CONNECTED;
    m_localId = 1; // Server host is player ID 1
    m_running = true;

    m_networkThread = std::thread(&NetworkManager::serverLoop, this, port);
    return true;
}

bool NetworkManager::connectClient(const std::string& host, uint16_t port) {
    disconnect();

    m_mode = NetworkMode::CLIENT;
    m_connState = ConnectionState::CONNECTING;
    m_localId = 0; // Will be assigned by server
    m_running = true;

    m_networkThread = std::thread(&NetworkManager::clientLoop, this, host, port);
    return true;
}

void NetworkManager::disconnect() {
    m_running = false;

    if (m_serverSocket != ~0ULL) {
        closesocket(static_cast<SOCKET>(m_serverSocket));
        m_serverSocket = ~0ULL;
    }

    if (m_clientSocket != ~0ULL) {
        closesocket(static_cast<SOCKET>(m_clientSocket));
        m_clientSocket = ~0ULL;
    }

    if (m_networkThread.joinable()) {
        m_networkThread.join();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& client : m_serverClients) {
        closesocket(static_cast<SOCKET>(client.socket));
    }
    m_serverClients.clear();
    m_remotePlayers.clear();
    m_pendingBlockChanges.clear();

    m_mode = NetworkMode::OFFLINE;
    m_connState = ConnectionState::OFFLINE;
    m_localId = 0;
}

void NetworkManager::broadcastTransform(const glm::vec3& pos, float yaw, float pitch) {
    if (!m_running) return;

    PlayerTransformData data{
        m_localId,
        pos.x, pos.y, pos.z,
        yaw, pitch
    };

    PacketHeader header{
        PacketType::PLAYER_TRANSFORM,
        m_localId,
        sizeof(PlayerTransformData)
    };

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(data));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &data, sizeof(data));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_mode == NetworkMode::SERVER) {
        for (const auto& client : m_serverClients) {
            sendRaw(client.socket, buffer.data(), static_cast<int>(buffer.size()));
        }
    } else if (m_mode == NetworkMode::CLIENT) {
        sendRaw(m_clientSocket, buffer.data(), static_cast<int>(buffer.size()));
    }
}

void NetworkManager::broadcastBlockChange(int x, int y, int z, BlockType type) {
    if (!m_running) return;

    BlockChangeData data{
        x, y, z,
        static_cast<uint8_t>(type)
    };

    PacketHeader header{
        PacketType::BLOCK_CHANGE,
        m_localId,
        sizeof(BlockChangeData)
    };

    std::vector<uint8_t> buffer(sizeof(header) + sizeof(data));
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), &data, sizeof(data));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_mode == NetworkMode::SERVER) {
        for (const auto& client : m_serverClients) {
            sendRaw(client.socket, buffer.data(), static_cast<int>(buffer.size()));
        }
    } else if (m_mode == NetworkMode::CLIENT) {
        sendRaw(m_clientSocket, buffer.data(), static_cast<int>(buffer.size()));
    }
}

void NetworkManager::poll(std::vector<RemotePlayer>& outPlayers, std::vector<BlockEvent>& outBlockChanges) {
    std::lock_guard<std::mutex> lock(m_mutex);

    outPlayers.clear();
    for (const auto& [id, player] : m_remotePlayers) {
        if (id != m_localId) {
            outPlayers.push_back(player);
        }
    }

    outBlockChanges = std::move(m_pendingBlockChanges);
    m_pendingBlockChanges.clear();
}

int NetworkManager::getClientCount() const {
    if (m_mode == NetworkMode::SERVER) {
        return static_cast<int>(m_serverClients.size());
    } else if (m_mode == NetworkMode::CLIENT) {
        return static_cast<int>(m_remotePlayers.size());
    }
    return 0;
}

void NetworkManager::serverLoop(uint16_t port) {
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "[Network] Failed to create server socket." << std::endl;
        m_running = false;
        return;
    }

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[Network] Bind failed on port " << port << std::endl;
        m_lastError = "PORT " + std::to_string(port) + " IN USE";
        m_errorDiagnosis = "Another program is already using port " + std::to_string(port);
        writeNetworkLog("ERROR", "Server bind failed on port " + std::to_string(port), m_errorDiagnosis);
        closesocket(listenSock);
        m_connState = ConnectionState::FAILED;
        m_mode = NetworkMode::OFFLINE;
        m_running = false;
        return;
    }
    writeNetworkLog("SERVER", "Server successfully listening on port " + std::to_string(port));

    if (listen(listenSock, 8) == SOCKET_ERROR) {
        std::cerr << "[Network] Listen failed." << std::endl;
        closesocket(listenSock);
        m_running = false;
        return;
    }

    m_serverSocket = static_cast<uint64_t>(listenSock);
    std::cout << "[Network] Server listening on port " << port << std::endl;

    uint32_t nextClientId = 2;

    while (m_running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSock, &readSet);

        SOCKET maxSock = listenSock;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& c : m_serverClients) {
                FD_SET(static_cast<SOCKET>(c.socket), &readSet);
                if (static_cast<SOCKET>(c.socket) > maxSock) {
                    maxSock = static_cast<SOCKET>(c.socket);
                }
            }
        }

        timeval tv{0, 50000}; // 50ms timeout
        int activity = select(static_cast<int>(maxSock + 1), &readSet, nullptr, nullptr, &tv);

        if (activity < 0) {
            if (!m_running) break;
            continue;
        }

        // Accept new connections
        if (FD_ISSET(listenSock, &readSet)) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            SOCKET newSock = accept(listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

            if (newSock != INVALID_SOCKET) {
                uint32_t assignedId = nextClientId++;

                // Send CONNECT_ACK
                PacketHeader ackHeader{PacketType::CONNECT_ACK, assignedId, sizeof(uint32_t)};
                std::vector<uint8_t> ackBuf(sizeof(ackHeader) + sizeof(uint32_t));
                std::memcpy(ackBuf.data(), &ackHeader, sizeof(ackHeader));
                std::memcpy(ackBuf.data() + sizeof(ackHeader), &assignedId, sizeof(uint32_t));
                send(newSock, reinterpret_cast<const char*>(ackBuf.data()), static_cast<int>(ackBuf.size()), 0);

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_serverClients.push_back({static_cast<uint64_t>(newSock), assignedId});
                    m_remotePlayers[assignedId] = RemotePlayer{assignedId, glm::vec3(0.0f), 0.0f, 0.0f};
                }

                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
                std::cout << "[Network] Client #" << assignedId << " connected from " << ip << std::endl;
            }
        }

        // Read from clients
        std::vector<uint32_t> disconnected;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto it = m_serverClients.begin(); it != m_serverClients.end();) {
                SOCKET sock = static_cast<SOCKET>(it->socket);
                if (FD_ISSET(sock, &readSet)) {
                    PacketHeader header;
                    int bytes = recv(sock, reinterpret_cast<char*>(&header), sizeof(header), MSG_WAITALL);

                    if (bytes <= 0) {
                        disconnected.push_back(it->id);
                        closesocket(sock);
                        it = m_serverClients.erase(it);
                        continue;
                    }

                    if (header.type == PacketType::PLAYER_TRANSFORM && header.dataSize == sizeof(PlayerTransformData)) {
                        PlayerTransformData tData;
                        recv(sock, reinterpret_cast<char*>(&tData), sizeof(tData), MSG_WAITALL);

                        m_remotePlayers[it->id] = RemotePlayer{it->id, glm::vec3(tData.x, tData.y, tData.z), tData.yaw, tData.pitch};

                        // Forward to all other clients
                        std::vector<uint8_t> forwardBuf(sizeof(header) + sizeof(tData));
                        std::memcpy(forwardBuf.data(), &header, sizeof(header));
                        std::memcpy(forwardBuf.data() + sizeof(header), &tData, sizeof(tData));

                        for (const auto& other : m_serverClients) {
                            if (other.id != it->id) {
                                sendRaw(other.socket, forwardBuf.data(), static_cast<int>(forwardBuf.size()));
                            }
                        }
                    } else if (header.type == PacketType::BLOCK_CHANGE && header.dataSize == sizeof(BlockChangeData)) {
                        BlockChangeData bData;
                        recv(sock, reinterpret_cast<char*>(&bData), sizeof(bData), MSG_WAITALL);

                        m_pendingBlockChanges.push_back({bData.x, bData.y, bData.z, static_cast<BlockType>(bData.blockType)});

                        // Forward to all other clients
                        std::vector<uint8_t> forwardBuf(sizeof(header) + sizeof(bData));
                        std::memcpy(forwardBuf.data(), &header, sizeof(header));
                        std::memcpy(forwardBuf.data() + sizeof(header), &bData, sizeof(bData));

                        for (const auto& other : m_serverClients) {
                            if (other.id != it->id) {
                                sendRaw(other.socket, forwardBuf.data(), static_cast<int>(forwardBuf.size()));
                            }
                        }
                    }
                }
                ++it;
            }

            for (uint32_t id : disconnected) {
                m_remotePlayers.erase(id);
                std::cout << "[Network] Client #" << id << " disconnected." << std::endl;
            }
        }
    }

    closesocket(listenSock);
    m_serverSocket = ~0ULL;
}

void NetworkManager::clientLoop(std::string host, uint16_t port) {
    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSock == INVALID_SOCKET) {
        std::cerr << "[Network] Failed to create client socket." << std::endl;
        m_connState = ConnectionState::FAILED;
        m_mode = NetworkMode::OFFLINE;
        m_running = false;
        return;
    }

    // Set 5-second socket timeout
#ifdef _WIN32
    DWORD timeout = 5000;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(clientSock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "[Network] Invalid server IP address: " << host << std::endl;
        closesocket(clientSock);
        m_connState = ConnectionState::FAILED;
        m_mode = NetworkMode::OFFLINE;
        m_running = false;
        return;
    }

    std::cout << "[Network] Connecting to server at " << host << ":" << port << "..." << std::endl;
    writeNetworkLog("CLIENT", "Attempting connection to " + host + ":" + std::to_string(port) + "...");

    if (connect(clientSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
#ifdef _WIN32
        int err = WSAGetLastError();
#else
        int err = 0;
#endif
        auto [desc, advice] = translateWinsockError(err);
        m_lastError = "ERR " + std::to_string(err) + ": " + desc;
        m_errorDiagnosis = advice;
        std::cerr << "[Network] Connection failed! Error: " << err << " - " << desc << std::endl;
        std::cerr << "[Network] Advice: " << advice << std::endl;
        writeNetworkLog("ERROR", "Failed to connect to " + host + ":" + std::to_string(port) + " (Error " + std::to_string(err) + ": " + desc + ")", advice);

        closesocket(clientSock);
        m_connState = ConnectionState::FAILED;
        m_mode = NetworkMode::OFFLINE;
        m_running = false;
        return;
    }

    m_clientSocket = static_cast<uint64_t>(clientSock);

    // Wait for CONNECT_ACK
    PacketHeader ackHeader{};
    int bytes = recv(clientSock, reinterpret_cast<char*>(&ackHeader), sizeof(ackHeader), MSG_WAITALL);
    if (bytes > 0 && ackHeader.type == PacketType::CONNECT_ACK) {
        uint32_t assignedId = 0;
        int idBytes = recv(clientSock, reinterpret_cast<char*>(&assignedId), sizeof(assignedId), MSG_WAITALL);
        if (idBytes > 0) {
            m_localId = assignedId;
            m_connState = ConnectionState::CONNECTED;
            std::cout << "[Network] Connected to server! Assigned Player ID: #" << m_localId << std::endl;
            writeNetworkLog("CLIENT", "Connected to " + host + ":" + std::to_string(port) + " as Player #" + std::to_string(assignedId));
        } else {
            std::cerr << "[Network] Failed to receive assigned player ID." << std::endl;
            m_connState = ConnectionState::FAILED;
            m_mode = NetworkMode::OFFLINE;
            closesocket(clientSock);
            m_clientSocket = ~0ULL;
            m_running = false;
            return;
        }
    } else {
        std::cerr << "[Network] Server failed to acknowledge connection." << std::endl;
        m_connState = ConnectionState::FAILED;
        m_mode = NetworkMode::OFFLINE;
        closesocket(clientSock);
        m_clientSocket = ~0ULL;
        m_running = false;
        return;
    }

    while (m_running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(clientSock, &readSet);

        timeval tv{0, 50000}; // 50ms
        int activity = select(static_cast<int>(clientSock + 1), &readSet, nullptr, nullptr, &tv);

        if (activity > 0 && FD_ISSET(clientSock, &readSet)) {
            PacketHeader header;
            int b = recv(clientSock, reinterpret_cast<char*>(&header), sizeof(header), MSG_WAITALL);
            if (b <= 0) {
                std::cout << "[Network] Server disconnected." << std::endl;
                m_connState = ConnectionState::OFFLINE;
                m_mode = NetworkMode::OFFLINE;
                break;
            }

            if (header.type == PacketType::PLAYER_TRANSFORM && header.dataSize == sizeof(PlayerTransformData)) {
                PlayerTransformData tData;
                recv(clientSock, reinterpret_cast<char*>(&tData), sizeof(tData), MSG_WAITALL);

                std::lock_guard<std::mutex> lock(m_mutex);
                m_remotePlayers[tData.id] = RemotePlayer{tData.id, glm::vec3(tData.x, tData.y, tData.z), tData.yaw, tData.pitch};
            } else if (header.type == PacketType::BLOCK_CHANGE && header.dataSize == sizeof(BlockChangeData)) {
                BlockChangeData bData;
                recv(clientSock, reinterpret_cast<char*>(&bData), sizeof(bData), MSG_WAITALL);

                std::lock_guard<std::mutex> lock(m_mutex);
                m_pendingBlockChanges.push_back({bData.x, bData.y, bData.z, static_cast<BlockType>(bData.blockType)});
            } else if (header.type == PacketType::PLAYER_DISCONNECT) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_remotePlayers.erase(header.senderId);
            }
        }
    }

    closesocket(clientSock);
    m_clientSocket = ~0ULL;
    m_running = false;
}

} // namespace voxel