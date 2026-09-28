// CanReceiveOnSocket  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d310, size 91 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d310..0x61d36a: a zero-timeout select for readability (nfds 0x40, WSOCK32
//   #18); 1 when readable.
// blam-cc: cdecl

#include "gamespy.h"

int CanReceiveOnSocket(SOCKET sock)
{
    fd_set read_set;
    struct timeval timeout;
    int result;

    read_set.fd_array[0] = sock;
    read_set.fd_count = 1;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    result = select(0x40, &read_set, 0, 0, &timeout);
    return result != SOCKET_ERROR && result != 0;
}
