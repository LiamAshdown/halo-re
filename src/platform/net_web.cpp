/*
 * The browser's sockets: a virtual LAN behind the page's server (tools/serve_web.py, /net).
 *
 * Browsers cannot open UDP sockets, so the BSD socket calls the network code and GameSpy make (socket, bind, sendto,
 * recvfrom, select, ...) are defined here in place of the C library's. Every page holds one WebSocket to the server,
 * which gives it a virtual address (10.66.x.y) and routes datagrams between the pages connected to it, broadcasts
 * included, so a game one player hosts shows up in the others' LAN list. Nothing is sent outside that network.
 *
 * Incoming datagrams are queued here as they arrive (on the page thread), so recvfrom and select answer at once as on
 * a real non-blocking socket; only a blocking call with nothing queued waits. TCP is not available: connect fails.
 *
 * Messages (little endian; addresses as the four network-order bytes):
 *   page -> server   1 bind    u32 socket, u16 port (0: any)
 *                    3 sendto  u32 socket, u8[4] address, u16 port, payload
 *                    6 close   u32 socket
 *   server -> page 100 hello   u8[4] this page's address
 *                  101 data    u32 socket, u8[4] from address, u16 from port, payload
 *                  102 bound   u32 socket, u16 port (0: the port is taken)
 */
#if defined(__EMSCRIPTEN__)

#include <arpa/inet.h>
#include <emscripten.h>
#include <emscripten/threading.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

namespace {

constexpr int k_first_socket = 700;  // socket numbers 700..999: clear of real descriptors, inside FD_SETSIZE
constexpr int k_socket_count = 300;
constexpr uint8_t k_bind = 1;
constexpr uint8_t k_sendto = 3;
constexpr uint8_t k_close = 6;
constexpr uint8_t k_hello = 100;
constexpr uint8_t k_data = 101;
constexpr uint8_t k_bound = 102;

struct datagram {
    uint32_t address;  // network order
    uint16_t port;
    std::vector<uint8_t> payload;
};

struct net_socket {
    bool used;
    bool udp;
    bool nonblocking;
    bool bind_pending;
    uint16_t port;  // 0 until bound
    std::deque<datagram> inbox;
};

std::mutex g_lock;
std::condition_variable g_changed;
net_socket g_sockets[k_socket_count];
uint32_t g_address;  // this page's virtual address, network order; 0 until the server says hello
bool g_connected;
bool g_lost;

net_socket *find(int s)
{
    return s >= k_first_socket && s < k_first_socket + k_socket_count && g_sockets[s - k_first_socket].used ? &g_sockets[s - k_first_socket]
                                                                                                         : nullptr;
}

void put16(std::vector<uint8_t> &out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
}

void put32(std::vector<uint8_t> &out, uint32_t value)
{
    put16(out, static_cast<uint16_t>(value));
    put16(out, static_cast<uint16_t>(value >> 16));
}

/** Sends a message to the server (page thread, which owns the WebSocket and opens it on first use); frees it. */
EM_JS(void, net_send_js, (const uint8_t *message, int length), {
    var net = Module.haloNet;
    if (!net) {
        var url = (location.protocol == 'https:' ? 'wss://' : 'ws://') + location.host + '/net';
        var socket = new WebSocket(url);
        net = Module.haloNet = { socket: socket, queue: [] };
        socket.binaryType = 'arraybuffer';
        socket.onopen = function() {
            net.queue.forEach(function(m) { socket.send(m); });
            net.queue = null;
        };
        socket.onmessage = function(event) {
            var bytes = new Uint8Array(event.data);
            var at = _malloc(bytes.length);
            HEAPU8.set(bytes, at);
            _halo_net_deliver(at, bytes.length);
            _free(at);
        };
        socket.onclose = function() { _halo_net_lost(); };
    }
    var copy = HEAPU8.slice(message, message + length);
    _free(message);
    if (net.queue) {
        net.queue.push(copy);
    } else if (net.socket.readyState == 1) {
        net.socket.send(copy);
    }
});

void post(const std::vector<uint8_t> &message)
{
    uint8_t *copy = static_cast<uint8_t *>(malloc(message.size()));

    memcpy(copy, message.data(), message.size());
    emscripten_async_run_in_main_runtime_thread(EM_FUNC_SIG_VII, net_send_js, copy, static_cast<int>(message.size()));
}

/** Waits (game thread) until ready() or the timeout; the lock is held. */
template <typename Ready>
bool wait_for(std::unique_lock<std::mutex> &held, int milliseconds, Ready ready)
{
    return g_changed.wait_for(held, std::chrono::milliseconds(milliseconds), ready);
}

/** Makes sure the page is on the network and knows its address; the lock is held. */
bool join_network(std::unique_lock<std::mutex> &held)
{
    if (!g_connected) {
        g_connected = true;
        held.unlock();
        post({0});  // opens the WebSocket; the server ignores the empty message
        held.lock();
    }
    return wait_for(held, 5000, [] { return g_address != 0 || g_lost; }) && g_address != 0;
}

uint32_t read32(const uint8_t *at)
{
    return static_cast<uint32_t>(at[0]) | static_cast<uint32_t>(at[1]) << 8 | static_cast<uint32_t>(at[2]) << 16 |
           static_cast<uint32_t>(at[3]) << 24;
}

bool readable(const net_socket &socket)
{
    return !socket.inbox.empty();
}

}  // namespace

extern "C" {

/** A message from the server (page thread). */
EMSCRIPTEN_KEEPALIVE void halo_net_deliver(const uint8_t *message, int length)
{
    std::lock_guard<std::mutex> held(g_lock);

    if (length >= 5 && message[0] == k_hello) {
        memcpy(&g_address, message + 1, 4);
    } else if (length >= 11 && message[0] == k_data) {
        if (net_socket *socket = find(static_cast<int>(read32(message + 1)))) {
            datagram packet;

            memcpy(&packet.address, message + 5, 4);
            packet.port = static_cast<uint16_t>(message[9] | message[10] << 8);
            packet.payload.assign(message + 11, message + length);
            if (socket->inbox.size() < 256) {
                socket->inbox.push_back(std::move(packet));
            }
        }
    } else if (length >= 7 && message[0] == k_bound) {
        if (net_socket *socket = find(static_cast<int>(read32(message + 1)))) {
            socket->port = static_cast<uint16_t>(message[5] | message[6] << 8);
            socket->bind_pending = false;
        }
    }
    g_changed.notify_all();
}

/** The WebSocket closed (page thread): nothing more will arrive. */
EMSCRIPTEN_KEEPALIVE void halo_net_lost()
{
    std::lock_guard<std::mutex> held(g_lock);

    g_lost = true;
    g_changed.notify_all();
}

int socket(int domain, int type, int protocol)
{
    std::unique_lock<std::mutex> held(g_lock);

    (void)protocol;
    if (domain != AF_INET) {
        errno = EAFNOSUPPORT;
        return -1;
    }
    if (!join_network(held)) {
        errno = ENETDOWN;
        return -1;
    }
    for (int i = 0; i < k_socket_count; i++) {
        if (!g_sockets[i].used) {
            g_sockets[i] = net_socket{};
            g_sockets[i].used = true;
            g_sockets[i].udp = (type & 0xf) == SOCK_DGRAM;
            return k_first_socket + i;
        }
    }
    errno = EMFILE;
    return -1;
}

int bind(int s, const struct sockaddr *address, socklen_t length)
{
    std::unique_lock<std::mutex> held(g_lock);
    net_socket *socket = find(s);
    std::vector<uint8_t> message{k_bind};
    uint16_t port;

    if (socket == nullptr || length < sizeof(sockaddr_in)) {
        errno = EBADF;
        return -1;
    }
    port = ntohs(reinterpret_cast<const sockaddr_in *>(address)->sin_port);
    put32(message, static_cast<uint32_t>(s));
    put16(message, port);
    socket->bind_pending = true;
    held.unlock();
    post(message);
    held.lock();
    socket = find(s);
    if (!wait_for(held, 5000, [&] { return socket == nullptr || !socket->bind_pending || g_lost; }) || socket == nullptr || socket->port == 0) {
        errno = EADDRINUSE;
        return -1;
    }
    return 0;
}

ssize_t sendto(int s, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t length)
{
    std::unique_lock<std::mutex> held(g_lock);
    net_socket *socket = find(s);
    const sockaddr_in *target = reinterpret_cast<const sockaddr_in *>(to);
    std::vector<uint8_t> message{k_sendto};

    (void)flags;
    if (socket == nullptr || !socket->udp || to == nullptr || length < sizeof(sockaddr_in)) {
        errno = socket == nullptr ? EBADF : EDESTADDRREQ;
        return -1;
    }
    if (g_lost) {
        errno = ENETDOWN;
        return -1;
    }
    // loopback stays in the page and arrives from 127.0.0.1, as on a real machine: the host's own client connects to
    // 127.0.0.1 and GameSpy matches replies by source address, so a reply from the virtual address would be refused
    if ((ntohl(target->sin_addr.s_addr) >> 24) == 127 && socket->port != 0) {
        // Web diagnostic: loopback traffic per 5 s (delivered / dropped), to see whether the host's own client stalls.
        static int delivered, dropped;
        static auto window = std::chrono::steady_clock::now();
        bool sent = false;

        for (net_socket &other : g_sockets) {
            if (other.used && other.udp && other.port == ntohs(target->sin_port) && other.inbox.size() < 256) {
                other.inbox.push_back({htonl(INADDR_LOOPBACK), socket->port,
                                       std::vector<uint8_t>(static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + size)});
                g_changed.notify_all();
                sent = true;
                break;
            }
        }
        (sent ? delivered : dropped)++;
        if (std::chrono::steady_clock::now() - window > std::chrono::seconds(5)) {
            fprintf(stderr, "web: loopback %d delivered, %d dropped in 5 s\n", delivered, dropped);
            delivered = dropped = 0;
            window = std::chrono::steady_clock::now();
        }
        return static_cast<ssize_t>(size);
    }
    put32(message, static_cast<uint32_t>(s));
    message.insert(message.end(), reinterpret_cast<const uint8_t *>(&target->sin_addr), reinterpret_cast<const uint8_t *>(&target->sin_addr) + 4);
    put16(message, ntohs(target->sin_port));
    message.insert(message.end(), static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + size);
    held.unlock();
    post(message);
    return static_cast<ssize_t>(size);
}

ssize_t send(int s, const void *data, size_t size, int flags)
{
    (void)data;
    (void)size;
    (void)flags;
    errno = find(s) != nullptr ? ENOTCONN : EBADF;
    return -1;
}

ssize_t recvfrom(int s, void *buffer, size_t size, int flags, struct sockaddr *from, socklen_t *length)
{
    std::unique_lock<std::mutex> held(g_lock);
    net_socket *socket = find(s);
    datagram packet;

    if (socket == nullptr || !socket->udp) {
        errno = socket == nullptr ? EBADF : ENOTCONN;
        return -1;
    }
    if (!readable(*socket)) {
        if (socket->nonblocking || g_lost) {
            errno = EWOULDBLOCK;
            return -1;
        }
        g_changed.wait(held, [&] { return find(s) == nullptr || readable(*find(s)) || g_lost; });
        socket = find(s);
        if (socket == nullptr || !readable(*socket)) {
            errno = ENETDOWN;
            return -1;
        }
    }
    packet = socket->inbox.front();
    if ((flags & MSG_PEEK) == 0) {
        socket->inbox.pop_front();
    }
    if (from != nullptr && length != nullptr && *length >= sizeof(sockaddr_in)) {
        sockaddr_in *out = reinterpret_cast<sockaddr_in *>(from);

        memset(out, 0, sizeof(*out));
        out->sin_family = AF_INET;
        memcpy(&out->sin_addr, &packet.address, 4);
        out->sin_port = htons(packet.port);
        *length = sizeof(sockaddr_in);
    }
    memcpy(buffer, packet.payload.data(), packet.payload.size() < size ? packet.payload.size() : size);
    if (packet.payload.size() > size) {
        errno = EMSGSIZE;  // Winsock drops the rest and reports WSAEMSGSIZE
        return -1;
    }
    return static_cast<ssize_t>(packet.payload.size());
}

ssize_t recv(int s, void *buffer, size_t size, int flags)
{
    return recvfrom(s, buffer, size, flags, nullptr, nullptr);
}

int connect(int s, const struct sockaddr *address, socklen_t length)
{
    (void)address;
    (void)length;
    errno = find(s) != nullptr ? ECONNREFUSED : EBADF;  // only the virtual LAN's datagrams are available
    return -1;
}

int listen(int s, int backlog)
{
    (void)backlog;
    errno = find(s) != nullptr ? EOPNOTSUPP : EBADF;
    return -1;
}

int accept(int s, struct sockaddr *address, socklen_t *length)
{
    (void)address;
    (void)length;
    errno = find(s) != nullptr ? EOPNOTSUPP : EBADF;
    return -1;
}

int shutdown(int s, int how)
{
    (void)how;
    return find(s) != nullptr ? 0 : (errno = EBADF, -1);
}

int setsockopt(int s, int level, int name, const void *value, socklen_t length)
{
    (void)level;
    (void)name;
    (void)value;
    (void)length;
    return find(s) != nullptr ? 0 : (errno = EBADF, -1);  // broadcast is always allowed on the virtual LAN
}

int getsockopt(int s, int level, int name, void *value, socklen_t *length)
{
    if (find(s) == nullptr) {
        errno = EBADF;
        return -1;
    }
    if (value != nullptr && length != nullptr && *length >= sizeof(int)) {
        *static_cast<int *>(value) = 0;
        *length = sizeof(int);
    }
    (void)level;
    (void)name;
    return 0;
}

int getsockname(int s, struct sockaddr *address, socklen_t *length)
{
    std::lock_guard<std::mutex> held(g_lock);
    net_socket *socket = find(s);

    if (socket == nullptr || address == nullptr || length == nullptr || *length < sizeof(sockaddr_in)) {
        errno = EBADF;
        return -1;
    }
    sockaddr_in *out = reinterpret_cast<sockaddr_in *>(address);

    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;
    memcpy(&out->sin_addr, &g_address, 4);
    out->sin_port = htons(socket->port);
    *length = sizeof(sockaddr_in);
    return 0;
}

int select(int count, fd_set *read_set, fd_set *write_set, fd_set *error_set, struct timeval *timeout)
{
    std::unique_lock<std::mutex> held(g_lock);
    fd_set reads;
    fd_set writes;
    int ready = 0;
    auto scan = [&] {
        ready = 0;
        FD_ZERO(&reads);
        FD_ZERO(&writes);
        for (int s = k_first_socket; s < count && s < k_first_socket + k_socket_count; s++) {
            net_socket *socket = find(s);

            if (socket == nullptr) {
                continue;
            }
            if (read_set != nullptr && FD_ISSET(s, read_set) && readable(*socket)) {
                FD_SET(s, &reads);
                ready++;
            }
            if (write_set != nullptr && FD_ISSET(s, write_set) && socket->udp) {
                FD_SET(s, &writes);
                ready++;
            }
        }
        return ready > 0;
    };

    if (!scan() && (timeout == nullptr || timeout->tv_sec > 0 || timeout->tv_usec > 0)) {
        if (timeout == nullptr) {
            g_changed.wait(held, scan);
        } else {
            wait_for(held, static_cast<int>(timeout->tv_sec * 1000 + timeout->tv_usec / 1000), scan);
        }
    }
    if (read_set != nullptr) {
        *read_set = reads;
    }
    if (write_set != nullptr) {
        *write_set = writes;
    }
    if (error_set != nullptr) {
        FD_ZERO(error_set);
    }
    return ready;
}

/** closesocket (types/win32.h). */
int halo_net_close(int s)
{
    std::unique_lock<std::mutex> held(g_lock);
    net_socket *socket = find(s);
    std::vector<uint8_t> message{k_close};

    if (socket == nullptr) {
        held.unlock();
        return close(s);
    }
    *socket = net_socket{};
    put32(message, static_cast<uint32_t>(s));
    held.unlock();
    post(message);
    return 0;
}

/** ioctlsocket (types/win32.h): FIONBIO and FIONREAD. */
int halo_net_ioctl(int s, long command, unsigned long *argument)
{
    std::lock_guard<std::mutex> held(g_lock);
    net_socket *socket = find(s);

    if (socket == nullptr) {
        errno = EBADF;
        return -1;
    }
    if (command == static_cast<long>(FIONBIO)) {
        socket->nonblocking = *argument != 0;
    } else if (command == static_cast<long>(FIONREAD)) {
        *argument = socket->inbox.empty() ? 0 : socket->inbox.front().payload.size();
    }
    return 0;
}

int gethostname(char *name, size_t size)
{
    strncpy(name, "halo-web", size);
    return 0;
}

/** Dotted addresses, and this page's own name, which is its virtual address; no other names exist on the LAN. */
struct hostent *gethostbyname(const char *name)
{
    static uint32_t address;
    static char *addresses[2] = {reinterpret_cast<char *>(&address), nullptr};
    static char *aliases[1] = {nullptr};
    static char host_name[64];
    static struct hostent entry;
    std::unique_lock<std::mutex> held(g_lock);

    if (inet_pton(AF_INET, name, &address) != 1) {
        if (strcmp(name, "halo-web") != 0 && strcmp(name, "localhost") != 0) {
            h_errno = HOST_NOT_FOUND;
            return nullptr;
        }
        if (!join_network(held)) {
            h_errno = TRY_AGAIN;
            return nullptr;
        }
        address = g_address;
    }
    strncpy(host_name, name, sizeof(host_name) - 1);
    entry.h_name = host_name;
    entry.h_aliases = aliases;
    entry.h_addrtype = AF_INET;
    entry.h_length = 4;
    entry.h_addr_list = addresses;
    return &entry;
}

}  // extern "C"

#endif
