// ghiSocketSelect  (GameSpy SDK in halo.exe; no C existed)
// address 0x621d70, size 344 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x621d70..0x621ec7: a zero-timeout select on the socket for the requested flags;
//   0 on error, else each requested flag says whether its set fired.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiSocketSelect(SOCKET socket, int *readFlag, int *writeFlag, int *exceptFlag)
{
    fd_set readSet;
    fd_set writeSet;
    fd_set exceptSet;
    fd_set *readSetPointer = 0;
    fd_set *writeSetPointer = 0;
    fd_set *exceptSetPointer = 0;
    struct timeval timeout;
    int result;

    if (readFlag != 0) {
        FD_ZERO(&readSet);
        FD_SET(socket, &readSet);
        readSetPointer = &readSet;
    }
    if (writeFlag != 0) {
        FD_ZERO(&writeSet);
        FD_SET(socket, &writeSet);
        writeSetPointer = &writeSet;
    }
    if (exceptFlag != 0) {
        FD_ZERO(&exceptSet);
        FD_SET(socket, &exceptSet);
        exceptSetPointer = &exceptSet;
    }
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    result = select(FD_SETSIZE, readSetPointer, writeSetPointer, exceptSetPointer, &timeout);
    if (result == SOCKET_ERROR) {
        return 0;
    }
    if (readFlag != 0) {
        *readFlag = result > 0 && FD_ISSET(socket, readSetPointer) ? 1 : 0;
    }
    if (writeFlag != 0) {
        *writeFlag = result > 0 && FD_ISSET(socket, writeSetPointer) ? 1 : 0;
    }
    if (exceptFlag != 0) {
        *exceptFlag = result > 0 && FD_ISSET(socket, exceptSetPointer) ? 1 : 0;
    }
    return 1;
}
