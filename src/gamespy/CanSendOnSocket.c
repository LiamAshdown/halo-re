// CanSendOnSocket  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d370, size 91 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d370..0x61d3ca: select with only the socket in the write set and a zero
//   timeout: 1 when it is writable.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int CanSendOnSocket(SOCKET sock)
{
    fd_set write_set;
    struct timeval timeout;
    int result;

    write_set.fd_count = 1;
    write_set.fd_array[0] = sock;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    result = select(0x40, 0, &write_set, 0, &timeout);
    if (result == SOCKET_ERROR || result == 0) {
        return 0;
    }
    return 1;
}
