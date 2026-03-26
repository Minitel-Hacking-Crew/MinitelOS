#pragma once
#include "Arduino.h"
#include "WiFiClientSecure.h"
#include <string>

// HTTPClient bridgé sur libcurl pour le simulateur natif
class HTTPClient {
    std::string _url;
    std::string _response;
    std::string _postData;
    struct curl_slist *_headers = nullptr;
    int _lastCode = -1;

    static size_t _write_cb(void *data, size_t size, size_t nmemb, void *userp);
    int _perform(bool isPost);
public:
    HTTPClient() {}
    ~HTTPClient();

    bool begin(WiFiClientSecure &, const String &url);
    bool begin(const String &url);
    void addHeader(const String &name, const String &value);
    int  GET();
    int  POST(const String &data);
    String getString();
    void end();
};
