#pragma once
/* win32.h -- the Windows API, from the Windows SDK headers.

   Every source file that calls Windows includes this first, instead of declaring the functions it uses itself: the
   prototypes (return type, parameters, __stdcall) then come from the SDK and cannot drift. The standalone link binds
   the calls to the SDK import libraries (kernel32, user32, gdi32, advapi32, ws2_32, winmm, version, ole32, oleaut32,
   shell32). WIN32_LEAN_AND_MEAN keeps the rarely used parts out; the few this code does use are included explicitly;
   NOMINMAX keeps the min / max macros out. winsock2.h has to come before windows.h.

   Off Windows (Emscripten, Linux) only the sockets are portable: this then gives the BSD sockets under the Winsock
   names the network code and GameSpy use. Files that need the rest of Windows are Windows-only. */
#ifndef HALO_WIN32_H
#define HALO_WIN32_H

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <objbase.h>
#include <oleauto.h>

#else

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

typedef int SOCKET;
typedef unsigned long u_long;
typedef struct hostent HOSTENT;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

#define WSAEWOULDBLOCK 10035
#define WSAEINPROGRESS 10036
#define WSAEMSGSIZE 10040
#define WSAECONNRESET 10054

typedef struct WSAData {
    unsigned short wVersion;
    unsigned short wHighVersion;
    char szDescription[257];
    char szSystemStatus[129];
    unsigned short iMaxSockets;
    unsigned short iMaxUdpDg;
    char *lpVendorInfo;
} WSADATA;

static inline int WSAStartup(unsigned short version, WSADATA *data)
{
    data->wVersion = version;
    data->wHighVersion = version;
    return 0;
}

static inline int WSACleanup(void)
{
    return 0;
}

/** errno in Winsock terms, for the cases the network code tells apart. */
static inline int WSAGetLastError(void)
{
    switch (errno) {
    case EWOULDBLOCK: return WSAEWOULDBLOCK;
    case EINPROGRESS: return WSAEINPROGRESS;
    case EMSGSIZE: return WSAEMSGSIZE;
    case ECONNRESET: case ECONNREFUSED: return WSAECONNRESET;
    default: return errno;
    }
}

static inline int closesocket(SOCKET s)
{
    return close(s);
}

static inline int ioctlsocket(SOCKET s, long command, u_long *argument)
{
    int value = (int)*argument;

    return ioctl(s, (unsigned long)command, &value);
}

#endif

#endif
