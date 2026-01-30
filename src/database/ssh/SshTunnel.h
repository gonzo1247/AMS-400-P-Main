#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include <libssh2.h>

#include "SettingsManager.h"

#ifdef _WIN32
#    ifndef _WINSOCKAPI_
#        include <winsock2.h>
#    endif
#else
#    include <sys/types.h>
#endif


namespace database
{

#ifdef _WIN32
using SocketType = SOCKET;
constexpr SocketType InvalidSocket = INVALID_SOCKET;
#else
using SocketType = int;
constexpr SocketType InvalidSocket = -1;
#endif

class SshTunnel
{
public:
    SshTunnel(const SSHConfig& config, std::string remoteHost, std::uint16_t remotePort);
    ~SshTunnel();

    SshTunnel(const SshTunnel&) = delete;
    SshTunnel& operator=(const SshTunnel&) = delete;

    bool Start();
    void Stop();

    std::string GetLocalHost() const;
    std::uint16_t GetLocalPort() const;
    bool IsActive() const { return active_; }

private:
    bool EstablishSession();
    bool CreateLocalListener();
    void ForwardLoop();
    void ForwardConnection(SocketType clientSocket, LIBSSH2_CHANNEL* channel);
    void CloseSession();
    void CloseListener();

    SSHConfig config_;
    std::string remoteHost_;
    std::uint16_t remotePort_;

    SocketType sshSocket_ = InvalidSocket;
    SocketType listenSocket_ = InvalidSocket;
    std::uint16_t localPort_ = 0;

    LIBSSH2_SESSION* session_ = nullptr;

    std::thread forwardThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> active_{false};
};

} // namespace database

