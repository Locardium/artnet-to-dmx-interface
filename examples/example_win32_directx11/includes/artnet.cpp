#include "artnet.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <array>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#include "globals.h"

namespace ArtNet {
    static constexpr size_t ARTNET_MAX_LEN = 530; // Header (18 bytes) + hasta 512 bytes datos

#pragma pack(push,1)
    struct ArtDmxHeader {
        char     ID[8];
        uint16_t OpCode;
        uint16_t ProtVer;  
        uint8_t  Sequence; 
        uint8_t  Physical;
        uint16_t Universe;  
        uint16_t Length;
    };
#pragma pack(pop)
    static SOCKET        g_sock = INVALID_SOCKET;
    static string   selectedIp = "";
    static bool          receiving = false;

    bool setup_socket(const char* localIp);
    void cleanup_socket();
    bool parse_artdmx(const uint8_t* packet, int bytes, vector<uint8_t>& dmxBuffer, uint16_t& outUniverse);

    void selectInterface(const string& ip) {
        selectedIp = ip;
    }

    bool startReceiving() {
        if (receiving) return true;

        const char* ip = nullptr;
        if (!selectedIp.empty()) ip = selectedIp.c_str();
        if (!setup_socket(ip)) return false;

        receiving = true;

        return true;
    }

    void stopReceiving() {
        if (!receiving) return;
        cleanup_socket();
        receiving = false;
    }

    bool isReceiving() {
        if (receiving) return true;

        return false;
    }

    vector<uint8_t> getDMX(uint16_t universe) {
        if (!receiving || g_sock == INVALID_SOCKET) {
            static vector<uint8_t> emptyBuf(512, 0);
            return emptyBuf;
        }

        static vector<uint8_t> lastDmx(512, 0);
        universe -= 1;

        array<uint8_t, ARTNET_MAX_LEN> packet;
        sockaddr_in from{};
        int fromLen = sizeof(from);

        while (true) {
            int bytes = recvfrom(
                g_sock,
                reinterpret_cast<char*>(packet.data()),
                static_cast<int>(packet.size()),
                0,
                (sockaddr*)&from,
                &fromLen
            );
            if (bytes <= 0) break;

            vector<uint8_t> tmp;
            uint16_t pktUniv;
            if (!parse_artdmx(packet.data(), bytes, tmp, pktUniv)) {
                continue;
            }
            if (pktUniv != universe) {
                continue;
            }

            lastDmx = tmp;
            lastDmx.resize(512, 0);
        }

        return lastDmx;
    }

    // Private functions
    inline bool setup_socket(const char* localIp) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            cerr << "[ArtNet] WSAStartup failed: " << WSAGetLastError() << "\n";
            return false;
        }

        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            cerr << "[ArtNet] Could not create UDP socket: " << WSAGetLastError() << "\n";
            WSACleanup();
            return false;
        }

        BOOL reuse = TRUE;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) {
            cerr << "[ArtNet] setsockopt SO_REUSEADDR failed: " << WSAGetLastError() << "\n";
            closesocket(sock);
            WSACleanup();
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ARTNET_PORT);
        if (localIp && strlen(localIp) > 0) {
            inet_pton(AF_INET, localIp, &addr.sin_addr);
        }
        else {
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        }

        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            cerr << "[ArtNet] Error to bind port" << ARTNET_PORT << ": " << WSAGetLastError() << "\n";
            closesocket(sock);
            WSACleanup();
            return false;
        }

        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);

        g_sock = sock;
        return true;
    }

    inline void cleanup_socket() {
        if (g_sock != INVALID_SOCKET) {
            closesocket(g_sock);
            g_sock = INVALID_SOCKET;
        }
        WSACleanup();
    }

    inline bool parse_artdmx(const uint8_t* packet, int bytes, vector<uint8_t>& dmxBuffer, uint16_t& outUniverse) {
        if (bytes < static_cast<int>(sizeof(ArtDmxHeader))) {
            return false;
        }
        ArtDmxHeader hdr;
        memcpy(&hdr, packet, sizeof(ArtDmxHeader));

        if (memcmp(hdr.ID, "Art-Net\0", 8) != 0) {
            return false;
        }
  
        if (hdr.OpCode != 0x5000) {
            return false;
        }
   
        outUniverse = hdr.Universe;

        uint16_t length = ntohs(hdr.Length);
        if (length > 512) length = 512;

        dmxBuffer.resize(length);
        memcpy(dmxBuffer.data(), packet + sizeof(ArtDmxHeader), length);
        return true;
    }

}
