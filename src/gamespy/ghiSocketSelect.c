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
        readSet.fd_array[0] = socket;
        readSet.fd_count = 1;
        readSetPointer = &readSet;
    }
    if (writeFlag != 0) {
        writeSet.fd_array[0] = socket;
        writeSet.fd_count = 1;
        writeSetPointer = &writeSet;
    }
    if (exceptFlag != 0) {
        exceptSet.fd_array[0] = socket;
        exceptSet.fd_count = 1;
        exceptSetPointer = &exceptSet;
    }
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    result = select(0x40, readSetPointer, writeSetPointer, exceptSetPointer, &timeout);
    if (result == SOCKET_ERROR) {
        return 0;
    }
    if (readFlag != 0) {
        *readFlag = result > 0 && __WSAFDIsSet(socket, readSetPointer) ? 1 : 0;
    }
    if (writeFlag != 0) {
        *writeFlag = result > 0 && __WSAFDIsSet(socket, writeSetPointer) ? 1 : 0;
    }
    if (exceptFlag != 0) {
        *exceptFlag = result > 0 && __WSAFDIsSet(socket, exceptSetPointer) ? 1 : 0;
    }
    return 1;
}
