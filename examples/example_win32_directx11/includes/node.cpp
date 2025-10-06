#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>

#include "payload.h"

#pragma comment(lib, "Ws2_32.lib")

namespace {
    std::atomic<bool> running{ false };
    std::thread senderThread;
}

namespace node {

    bool start() {
        if (running.load()) {
            std::cerr << "[node] Already running.\n";
            return false;
        }

        running.store(true);
        senderThread = std::thread([]() {
            const char* dstIp = "236.4.0.0";
            const unsigned short dstPort = 29999;

            WSADATA wsa;
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
                std::cerr << "[node] WSAStartup failed.\n";
                running.store(false);
                return;
            }

            SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock == INVALID_SOCKET) {
                std::cerr << "[node] Socket error: " << WSAGetLastError() << "\n";
                WSACleanup();
                running.store(false);
                return;
            }

            sockaddr_in dest{};
            dest.sin_family = AF_INET;
            dest.sin_port = htons(dstPort);
            if (InetPtonA(AF_INET, dstIp, &dest.sin_addr) != 1) {
                std::cerr << "[node] Invalid IP.\n";
                closesocket(sock);
                WSACleanup();
                running.store(false);
                return;
            }

            std::cout << "[node] Sending both payloads to " << dstIp << ":" << dstPort << "...\n";

            while (running.load()) {
                // node 0
                int sent0 = sendto(sock, reinterpret_cast<const char*>(dmx_payload_node0),
                    static_cast<int>(dmx_payload_node0_len), 0,
                    reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
                if (sent0 == SOCKET_ERROR)
                    std::cerr << "[node] sendto node0 error: " << WSAGetLastError() << "\n";

                // node 1
                int sent1 = sendto(sock, reinterpret_cast<const char*>(dmx_payload_node1),
                    static_cast<int>(dmx_payload_node1_len), 0,
                    reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
                if (sent1 == SOCKET_ERROR)
                    std::cerr << "[node] sendto node1 error: " << WSAGetLastError() << "\n";

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            closesocket(sock);
            WSACleanup();
            running.store(false);
            std::cout << "[node] Send stopped.\n";
            });

        return true;
    }

    void stop() {
        if (!running.load()) return;
        running.store(false);
        if (senderThread.joinable())
            senderThread.join();
    }

    bool isRunning() {
        return running.load();
    }
} 
