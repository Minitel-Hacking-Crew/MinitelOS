#include "sshClient.h"

SSHClient::SSHClient() {}

bool SSHClient::begin(const char *host, const char *username, const char *password)
{
    libssh_begin();
    SSHStatus status = start_session(host, username, password);
    if (status != SSHStatus::OK)
        return false;
    if (!open_channel())
        return false;
    if (SSH_OK != interactive_shell_session())
        return false;
    return true;
}

bool SSHClient::available()
{
    if (!ssh_channel_is_open(_channel) || ssh_channel_is_eof(_channel))
    {
        return false;
    }
    return true;
}

int SSHClient::receive()
{
    memset(_readBuffer, '\0', sizeof(_readBuffer));
    return ssh_channel_read_nonblocking(_channel, _readBuffer, sizeof(_readBuffer), 0);
}

int SSHClient::flushReceiving()
{
    int nbyte;
    int tbyte = 0;
    uint8_t count = 0;
    do
    {
        nbyte = ssh_channel_read_nonblocking(_channel, _readBuffer, sizeof(_readBuffer), 0);
        tbyte += nbyte;
        if (nbyte == 0)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            count++;
        }
    } while (count < 3);
    return tbyte;
}

char SSHClient::readIndex(int index)
{
    return _readBuffer[index];
}

int SSHClient::send(void *buffer, uint32_t len)
{
    int rc = ssh_channel_write(_channel, buffer, len);
    if (rc == SSH_ERROR)
        return -1;
    return rc;
}

void SSHClient::end()
{
    close_channel();
    close_session();
}

SSHClient::SSHStatus SSHClient::connect_ssh(const char *host, const char *user, const char *password, const int verbosity)
{
    _session = ssh_new();

    if (_session == NULL)
    {
        return SSHStatus::GENERAL_ERROR;
    }

    if (ssh_options_set(_session, SSH_OPTIONS_USER, user) < 0)
    {
        ssh_free(_session);
        return SSHStatus::GENERAL_ERROR;
    }

    if (ssh_options_set(_session, SSH_OPTIONS_HOST, host) < 0)
    {
        ssh_free(_session);
        return SSHStatus::GENERAL_ERROR;
    }

    ssh_options_set(_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

    if (ssh_connect(_session))
    {
        ssh_disconnect(_session);
        ssh_free(_session);
        return SSHStatus::GENERAL_ERROR;
    }

    // Authenticate ourselves
    if (ssh_userauth_password(_session, NULL, password) != SSH_AUTH_SUCCESS)
    {
        ssh_disconnect(_session);
        ssh_free(_session);
        return SSHStatus::AUTHENTICATION_ERROR;
    }
    return SSHStatus::OK;
}

bool SSHClient::open_channel()
{
    _channel = ssh_channel_new(_session);
    if (_channel == NULL)
    {
        return false;
    }
    const int ret = ssh_channel_open_session(_channel);
    if (ret != SSH_OK)
    {
        return false;
    }
    return true;
}

void SSHClient::close_channel()
{
    if (_channel != NULL)
    {
        ssh_channel_close(_channel);
        ssh_channel_send_eof(_channel);
        ssh_channel_free(_channel);
    }
}

int SSHClient::interactive_shell_session()
{
    int ret;

    ret = ssh_channel_request_pty_size(_channel, "vt100", 80, 24);
    if (ret != SSH_OK)
    {
        return ret;
    }

    ret = ssh_channel_request_shell(_channel);
    if (ret != SSH_OK)
    {
        return ret;
    }

    return ret;
}

SSHClient::SSHStatus SSHClient::start_session(const char *host, const char *user, const char *password)
{
    SSHStatus status = connect_ssh(host, user, password, SSH_LOG_NOLOG);
    if (status != SSHStatus::OK)
    {
        ssh_finalize();
    }
    return status;
}

void SSHClient::close_session()
{
    if (_session != NULL)
    {
        ssh_disconnect(_session);
        ssh_free(_session);
    }
}
