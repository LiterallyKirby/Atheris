#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include <string>
#include "ChunkManager.h"
#include "Player.h"
#include "Constants.h"

using socket_t = int;


class Server {
public:
    Server(uint16_t port);
    ~Server();

    void start();
    void stop();

    // allow server to know player's camera/player on server-side (optional)
    void setPlayer(Player* p) { player = p; }

private:
    void run();
    void handleUdpRequest(socket_t sock);
    bool handleTcpRequest(socket_t clientSock);

    socket_t createNonBlockingSocket(int type, int protocol);

    uint16_t port;
    std::atomic<bool> running;
    std::thread serverThread;

    std::atomic<socket_t> udpSocket;
    std::atomic<socket_t> tcpSocket;

    std::unique_ptr<ChunkManager> chunkManager;
    Player* player = nullptr;
};
