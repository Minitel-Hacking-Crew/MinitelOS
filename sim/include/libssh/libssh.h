#pragma once
// Minimal libssh stub for native simulation — SSH is not functional in sim

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Types
typedef void* ssh_session;
typedef void* ssh_channel;

// Return codes
#define SSH_OK    0
#define SSH_ERROR (-1)
#define SSH_AGAIN (-2)

// Auth results
#define SSH_AUTH_SUCCESS 0
#define SSH_AUTH_ERROR   (-1)
#define SSH_AUTH_DENIED  1

// Log levels
#define SSH_LOG_NOLOG    0
#define SSH_LOG_WARNING  1
#define SSH_LOG_PROTOCOL 2
#define SSH_LOG_PACKET   3
#define SSH_LOG_FUNCTIONS 4

// Options
typedef enum {
    SSH_OPTIONS_HOST = 0,
    SSH_OPTIONS_USER,
    SSH_OPTIONS_LOG_VERBOSITY,
    SSH_OPTIONS_PORT,
    SSH_OPTIONS_TIMEOUT,
} ssh_options_e;

// Session
static inline ssh_session ssh_new(void) { return NULL; }
static inline void ssh_free(ssh_session s) { (void)s; }
static inline int ssh_connect(ssh_session s) { (void)s; return SSH_ERROR; }
static inline void ssh_disconnect(ssh_session s) { (void)s; }
static inline int ssh_options_set(ssh_session s, ssh_options_e t, const void* v) { (void)s;(void)t;(void)v; return SSH_ERROR; }
static inline int ssh_userauth_password(ssh_session s, const char* u, const char* p) { (void)s;(void)u;(void)p; return SSH_AUTH_ERROR; }
static inline void ssh_finalize(void) {}
static inline void libssh_begin(void) {}

// Channel
static inline ssh_channel ssh_channel_new(ssh_session s) { (void)s; return NULL; }
static inline void ssh_channel_free(ssh_channel c) { (void)c; }
static inline int ssh_channel_open_session(ssh_channel c) { (void)c; return SSH_ERROR; }
static inline void ssh_channel_close(ssh_channel c) { (void)c; }
static inline int ssh_channel_send_eof(ssh_channel c) { (void)c; return SSH_ERROR; }
static inline int ssh_channel_is_open(ssh_channel c) { (void)c; return 0; }
static inline int ssh_channel_is_eof(ssh_channel c) { (void)c; return 1; }
static inline int ssh_channel_read_nonblocking(ssh_channel c, void* buf, uint32_t n, int stderr_flag) {
    (void)c;(void)buf;(void)n;(void)stderr_flag; return 0;
}
static inline int ssh_channel_write(ssh_channel c, const void* buf, uint32_t n) {
    (void)c;(void)buf;(void)n; return SSH_ERROR;
}
static inline int ssh_channel_request_pty_size(ssh_channel c, const char* t, int cols, int rows) {
    (void)c;(void)t;(void)cols;(void)rows; return SSH_ERROR;
}
static inline int ssh_channel_request_shell(ssh_channel c) { (void)c; return SSH_ERROR; }

#ifdef __cplusplus
}
#endif
