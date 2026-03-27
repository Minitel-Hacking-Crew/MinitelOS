#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
// Utilise CommonCrypto (macOS) pour un MD5 correct et certifié
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>

static inline void mbedtls_md5(const unsigned char* input, size_t ilen,
                                unsigned char output[16]) {
    CC_MD5(input, (CC_LONG)ilen, output);
}

#else
// Implémentation portable pour autres plateformes

typedef struct {
    uint32_t lo, hi;
    uint32_t a, b, c, d;
    uint8_t  buffer[64];
    uint32_t block[16];
} _MD5_CTX;

void _md5_init(_MD5_CTX* ctx);
void _md5_update(_MD5_CTX* ctx, const uint8_t* data, size_t size);
void _md5_final(uint8_t* result, _MD5_CTX* ctx);

static inline void mbedtls_md5(const unsigned char* input, size_t ilen,
                                unsigned char output[16]) {
    _MD5_CTX ctx;
    _md5_init(&ctx);
    _md5_update(&ctx, input, ilen);
    _md5_final(output, &ctx);
}

#endif // __APPLE__

#ifdef __cplusplus
}
#endif
