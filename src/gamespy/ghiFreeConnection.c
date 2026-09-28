// ghiFreeConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x620950, size 259 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620950..0x620a52: for a live connection: under the lock frees its strings,
//   closes its save file and socket (shutdown both ways), frees its buffers and posting state, frees an auto-free
//   post, and marks it unused; 1, else 0.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern GHIConnection **ghiConnections;   // 0x006a3284
extern int ghiConnectionsLen;             // 0x006a327c
extern int ghiNumConnections;             // 0x006a3280

int ghiFreeConnection(GHIConnection *connection)
{
    if (connection == 0 || connection->inUse == 0 || connection->request < 0 ||
        connection->request >= ghiConnectionsLen) {
        return 0;
    }
    ghiLock();
    free(connection->URL);
    free(connection->serverAddress);
    free(connection->requestPath);
    free(connection->sendHeaders);
    free(connection->redirectURL);
    if (connection->saveFile != 0) {
        fclose(connection->saveFile);
    }
    if (connection->socket != INVALID_SOCKET) {
        shutdown(connection->socket, 2);
        closesocket(connection->socket);
    }
    ghiFreeBuffer(&connection->sendBuffer);
    ghiFreeBuffer(&connection->recvBuffer);
    ghiFreeBuffer(&connection->getFileBuffer);
    if (connection->postingStates != 0) {
        ghiPostCleanupState(connection);
    }
    if (connection->post != 0 && ghiIsPostAutoFree(connection->post)) {
        ghttpFreePost(connection->post);
        connection->post = 0;
    }
    ghiNumConnections--;
    connection->inUse = 0;
    ghiUnlock();
    return 1;
}
