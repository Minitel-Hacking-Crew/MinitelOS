#pragma once
#include "Arduino.h"
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>

class ESP32PingClass {
public:
    // Simule un ping via tentative de connexion TCP port 80 (ou 443)
    // Si l'hôte répond sur TCP, il est "joignable". Fallback: résolution DNS.
    bool ping(const char* host, uint8_t count = 1) {
        struct addrinfo hints{}, *res = nullptr;
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(host, "80", &hints, &res) != 0 || !res)
            return false;

        // Essai connexion TCP non-bloquante (timeout 2s)
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) { freeaddrinfo(res); return false; }

        fcntl(sock, F_SETFL, O_NONBLOCK);
        connect(sock, res->ai_addr, res->ai_addrlen);

        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(sock, &wfds);
        struct timeval tv{ 2, 0 };
        bool ok = (select(sock + 1, nullptr, &wfds, nullptr, &tv) > 0);

        close(sock);
        freeaddrinfo(res);
        return ok;
    }
};

extern ESP32PingClass Ping;
