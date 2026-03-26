#pragma once
#include "Arduino.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#define WIFI_STA     1
#define WIFI_AP      2
#define WIFI_AP_STA  3
#define WIFI_OFF     0

#define WL_CONNECTED      3
#define WL_DISCONNECTED   6
#define WL_CONNECT_FAILED 4
#define WIFI_AUTH_OPEN    0
#define WIFI_AUTH_WPA2_PSK 4

class IPAddress {
    uint32_t _addr = 0;
public:
    IPAddress() {}
    IPAddress(uint32_t a) : _addr(a) {}
    String toString() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
            (_addr) & 0xFF, (_addr >> 8) & 0xFF,
            (_addr >> 16) & 0xFF, (_addr >> 24) & 0xFF);
        return String(buf);
    }
};

// Fake networks shown during WiFi scan
struct FakeAP { const char *ssid; int rssi; int auth; };
static const FakeAP _fake_aps[] = {
    { "Livebox-MinitelNet",  -42, WIFI_AUTH_WPA2_PSK },
    { "FreeWifi",            -67, WIFI_AUTH_OPEN      },
    { "SFR_HOME",            -78, WIFI_AUTH_WPA2_PSK },
};
static const int _fake_ap_count = 3;

static IPAddress _get_local_ip() {
    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1) return IPAddress(0x0100007F);
    for (auto *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        if (strcmp(ifa->ifa_name, "lo0") == 0) continue;
        auto *sin = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
        uint32_t addr = sin->sin_addr.s_addr;  // déjà en bon ordre pour IPAddress::toString()
        freeifaddrs(ifaddr);
        return IPAddress(addr);
    }
    freeifaddrs(ifaddr);
    return IPAddress(0x0100007F);
}

class WiFiClass {
    // Dans le simulateur, la machine hôte a toujours internet
    bool _connected = true;
    String _ssid = "SimulatorNet";
public:
    void mode(int) {}
    void disconnect(bool wifioff = false) { _connected = false; }

    void begin(const char* ssid, const char* pass) {
        _ssid = ssid;
        // On macOS le réseau hôte est toujours disponible — on réussit toujours
        _connected = true;
    }

    int status() const { return _connected ? WL_CONNECTED : WL_DISCONNECTED; }

    String SSID()           const { return _connected ? _ssid : String(""); }
    IPAddress localIP()     const { return _connected ? _get_local_ip() : IPAddress(0); }
    IPAddress gatewayIP()   const { return IPAddress(0x0101A8C0); }
    IPAddress subnetMask()  const { return IPAddress(0x00FFFFFF); }
    IPAddress dnsIP()       const { return IPAddress(0x08080808); }
    String macAddress()     const { return String("DE:AD:BE:EF:00:01"); }
    int RSSI()              const { return _connected ? -48 : 0; }

    // DNS resolution via POSIX getaddrinfo
    int hostByName(const char* host, IPAddress &result) {
        struct addrinfo hints = {}, *res;
        hints.ai_family = AF_INET;
        if (getaddrinfo(host, nullptr, &hints, &res) != 0) return 0;
        uint32_t addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
        result = IPAddress(addr);
        freeaddrinfo(res);
        return 1;
    }

    // Scan : retourne les faux APs
    int scanNetworks()      { return _fake_ap_count; }
    void scanDelete()       {}
    String SSID(int i)      { return (i >= 0 && i < _fake_ap_count) ? String(_fake_aps[i].ssid) : String(""); }
    int RSSI(int i)         { return (i >= 0 && i < _fake_ap_count) ? _fake_aps[i].rssi : 0; }
    int encryptionType(int i){ return (i >= 0 && i < _fake_ap_count) ? _fake_aps[i].auth : WIFI_AUTH_OPEN; }
};

extern WiFiClass WiFi;
