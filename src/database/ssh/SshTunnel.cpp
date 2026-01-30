#include "ssh/SshTunnel.h"

#include <array>
#include <chrono>
#include <cerrno>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#    include <BaseTsd.h>
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    ifndef socklen_t
using socklen_t = int;
#    endif
using ssize_t = SSIZE_T;
#else
#    include <arpa/inet.h>
#    include <netdb.h>
#    include <sys/select.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif

#include <libssh2.h>

namespace database
{

namespace
{

class Libssh2Initializer
{
public:
    Libssh2Initializer()
    {
        if (libssh2_init(0) != 0)
            throw std::runtime_error("Failed to initialize libssh2");
    }

    ~Libssh2Initializer()
    {
        libssh2_exit();
    }
};

void EnsureLibssh2Initialized()
{
    static Libssh2Initializer initializer;
    (void)initializer;
}

#ifdef _WIN32

class WinsockInitializer
{
public:
    WinsockInitializer()
    {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
            throw std::runtime_error("Failed to initialize Winsock");
    }

    ~WinsockInitializer()
    {
        WSACleanup();
    }
};

void EnsureWinsockInitialized()
{
    static WinsockInitializer initializer;
    (void)initializer;
}

inline void CloseSocket(SocketType socket)
{
    if (socket != InvalidSocket)
        closesocket(socket);
}

int LastSocketError()
{
    return WSAGetLastError();
}

bool WasInterrupted(int error)
{
    return error == WSAEINTR;
}

int SelectSocket(SocketType socket, fd_set* readfds, timeval* timeout)
{
    (void)socket;
    return select(0, readfds, nullptr, nullptr, timeout);
}

#else

void EnsureWinsockInitialized() {}

inline void CloseSocket(SocketType socket)
{
    if (socket >= 0)
        close(socket);
}

int LastSocketError()
{
    return errno;
}

bool WasInterrupted(int error)
{
    return error == EINTR;
}

int SelectSocket(SocketType socket, fd_set* readfds, timeval* timeout)
{
    return select(static_cast<int>(socket + 1), readfds, nullptr, nullptr, timeout);
}

#endif

std::string DescribeAddressInfoError(int error)
{
#ifdef _WIN32
    const char* message = gai_strerrorA(error);
#else
    const char* message = gai_strerror(error);
#endif
    if (message)
        return std::string(message);
    return std::string("error ") + std::to_string(error);
}

SocketType CreateConnectedSocket(const std::string& host, std::uint16_t port)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    const auto portString = std::to_string(port);
    if (const int rc = getaddrinfo(host.c_str(), portString.c_str(), &hints, &result); rc != 0)
    {
        LOG_ERROR("getaddrinfo failed for SSH host: " + DescribeAddressInfoError(rc));
        return InvalidSocket;
    }

    SocketType connected = InvalidSocket;
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        SocketType sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock == InvalidSocket)
            continue;

        if (connect(sock, ptr->ai_addr, static_cast<socklen_t>(ptr->ai_addrlen)) == 0)
        {
            connected = sock;
            break;
        }

        CloseSocket(sock);
    }

    freeaddrinfo(result);

    if (connected == InvalidSocket)
        LOG_ERROR("Failed to connect to SSH host " + host + ":" + portString);

    return connected;
}

SocketType CreateLoopbackListener(std::uint16_t& outPort)
{
    SocketType sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == InvalidSocket)
    {
        LOG_ERROR("Failed to create local listener socket");
        return InvalidSocket;
    }

#ifdef _WIN32
    const char opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#else
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        LOG_ERROR("Failed to bind local listener socket");
        CloseSocket(sock);
        return InvalidSocket;
    }

    if (listen(sock, 1) != 0)
    {
        LOG_ERROR("Failed to listen on local tunnel socket");
        CloseSocket(sock);
        return InvalidSocket;
    }

    socklen_t len = sizeof(addr);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &len) != 0)
    {
        LOG_ERROR("getsockname failed for tunnel listener");
        CloseSocket(sock);
        return InvalidSocket;
    }

    outPort = ntohs(addr.sin_port);
    return sock;
}

std::string DescribeLastError(const std::string& context, LIBSSH2_SESSION* session)
{
    char* message = nullptr;
    int length = 0;
    int rc = libssh2_session_last_error(session, &message, &length, 0);
    if (rc == 0 && message)
        return context + ": " + std::string(message, static_cast<std::size_t>(length));
    return context;
}

} // namespace

SshTunnel::SshTunnel(const SSHConfig& config, std::string remoteHost, std::uint16_t remotePort)
    : config_(config), remoteHost_(std::move(remoteHost)), remotePort_(remotePort)
{
}

SshTunnel::~SshTunnel()
{
    Stop();
}

bool SshTunnel::Start()
{
    if (active_)
        return true;

    EnsureWinsockInitialized();
    EnsureLibssh2Initialized();

    if (!EstablishSession())
    {
        Stop();
        return false;
    }

    if (!CreateLocalListener())
    {
        Stop();
        return false;
    }

    running_ = true;
    forwardThread_ = std::thread(&SshTunnel::ForwardLoop, this);
    active_ = true;
    LOG_SQL("SSH tunnel established to " + remoteHost_ + ":" + std::to_string(remotePort_));
    return true;
}

void SshTunnel::Stop()
{
    running_ = false;

    if (listenSocket_ != InvalidSocket)
    {
        // Wake the accept() call so the thread can exit cleanly.
        SocketType wake = socket(AF_INET, SOCK_STREAM, 0);
        if (wake != InvalidSocket)
        {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = htons(localPort_);
            connect(wake, reinterpret_cast<sockaddr*>(&addr), static_cast<socklen_t>(sizeof(addr)));
            CloseSocket(wake);
        }
    }

    if (forwardThread_.joinable())
        forwardThread_.join();

    CloseListener();
    CloseSession();

    active_ = false;
}

std::string SshTunnel::GetLocalHost() const
{
    return "127.0.0.1";
}

std::uint16_t SshTunnel::GetLocalPort() const
{
    return localPort_;
}

bool SshTunnel::EstablishSession()
{
    sshSocket_ = CreateConnectedSocket(config_.host, config_.port);
    if (sshSocket_ == InvalidSocket)
        return false;

    session_ = libssh2_session_init();
    if (!session_)
    {
        LOG_ERROR("Failed to create libssh2 session");
        return false;
    }

    libssh2_session_set_blocking(session_, 1);

    if (const int rc = libssh2_session_handshake(session_, static_cast<libssh2_socket_t>(sshSocket_)); rc != 0)
    {
        LOG_ERROR("SSH handshake failed: " + std::to_string(rc));
        CloseSession();
        return false;
    }

    libssh2_keepalive_config(session_, 1, 60);

    if (!config_.privateKeyFile.empty())
    {
        const char* passphrase = config_.passphrase.empty() ? nullptr : config_.passphrase.c_str();
        const int rc = libssh2_userauth_publickey_fromfile(session_, config_.username.c_str(), nullptr, config_.privateKeyFile.c_str(), passphrase);
        if (rc != 0)
        {
            LOG_ERROR(DescribeLastError("SSH public-key authentication failed", session_));
            CloseSession();
            return false;
        }
    }
    else if (!config_.password.empty())
    {
        const int rc = libssh2_userauth_password(session_, config_.username.c_str(), config_.password.c_str());
        if (rc != 0)
        {
            LOG_ERROR(DescribeLastError("SSH password authentication failed", session_));
            CloseSession();
            return false;
        }
    }
    else
    {
        LOG_ERROR("SSH configuration missing authentication data");
        CloseSession();
        return false;
    }

    return true;
}

bool SshTunnel::CreateLocalListener()
{
    listenSocket_ = CreateLoopbackListener(localPort_);
    return listenSocket_ != InvalidSocket;
}

void SshTunnel::ForwardLoop()
{
    while (running_)
    {
        sockaddr_storage addr{};
        socklen_t len = sizeof(addr);
        SocketType client = accept(listenSocket_, reinterpret_cast<sockaddr*>(&addr), &len);
        if (client == InvalidSocket)
        {
            if (!running_)
                break;
            if (WasInterrupted(LastSocketError()))
                continue;

            LOG_ERROR("SSH tunnel accept failed");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        LIBSSH2_CHANNEL* channel = libssh2_channel_direct_tcpip_ex(session_, remoteHost_.c_str(), remotePort_, "127.0.0.1", 0);
        if (!channel)
        {
            LOG_ERROR(DescribeLastError("Failed to open SSH direct-tcpip channel", session_));
            CloseSocket(client);
            continue;
        }

        ForwardConnection(client, channel);

        libssh2_channel_close(channel);
        libssh2_channel_free(channel);
        CloseSocket(client);
    }
}

void SshTunnel::ForwardConnection(SocketType clientSocket, LIBSSH2_CHANNEL* channel)
{
    constexpr std::size_t BufferSize = 4096;
    std::array<char, BufferSize> buffer{};

    while (running_)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);

        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 200'000; // 200 ms

        int rc = SelectSocket(clientSocket, &readfds, &timeout);
        if (rc < 0)
        {
            if (WasInterrupted(LastSocketError()))
                continue;
            LOG_ERROR("SSH tunnel select failed");
            break;
        }

        if (rc > 0 && FD_ISSET(clientSocket, &readfds))
        {
            const ssize_t received = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
            if (received <= 0)
                break;

            ssize_t writtenTotal = 0;
            while (writtenTotal < received)
            {
                const ssize_t written = libssh2_channel_write(channel, buffer.data() + writtenTotal, received - writtenTotal);
                if (written == LIBSSH2_ERROR_EAGAIN)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    continue;
                }
                if (written <= 0)
                    return;
                writtenTotal += written;
            }
        }

        while (running_)
        {
            const ssize_t available = libssh2_channel_read(channel, buffer.data(), buffer.size());
            if (available == LIBSSH2_ERROR_EAGAIN)
                break;
            if (available <= 0)
            {
                if (available == 0 && libssh2_channel_eof(channel))
                    return;
                if (available < 0)
                    LOG_ERROR("SSH channel read failed with code " + std::to_string(available));
                return;
            }

            ssize_t sentTotal = 0;
            while (sentTotal < available)
            {
                const ssize_t sent = send(clientSocket, buffer.data() + sentTotal, static_cast<int>(available - sentTotal), 0);
                if (sent <= 0)
                    return;
                sentTotal += sent;
            }

            if (available < static_cast<ssize_t>(buffer.size()))
                break;
        }
    }
}

void SshTunnel::CloseSession()
{
    if (session_)
    {
        libssh2_session_disconnect(session_, "Normal Shutdown");
        libssh2_session_free(session_);
        session_ = nullptr;
    }

    if (sshSocket_ != InvalidSocket)
    {
        CloseSocket(sshSocket_);
        sshSocket_ = InvalidSocket;
    }
}

void SshTunnel::CloseListener()
{
    if (listenSocket_ != InvalidSocket)
    {
        CloseSocket(listenSocket_);
        listenSocket_ = InvalidSocket;
    }
    localPort_ = 0;
}

} // namespace database

