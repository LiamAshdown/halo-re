// ghttpGetEx  (GameSpy SDK in halo.exe; no C existed)
// address 0x61bd80, size 357 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61bd80..0x61bee4: (URL, headers, buffer, buffer size, post, throttle, blocking,
//   progress, completed, param): -1 for no URL, a negative size or a buffer without a size; starts ghttp when needed;
//   a get connection with copies of the URL and headers, the caller  buffer (fixed) or a 2 KB growable one, and the
//   post state. Blocking: processed every 10 ms until done (0); else the request index. -1 (connection freed) on
//   failure.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern int ghiReferenceCount;             // 0x006a2e6c

int ghttpGetEx(const char *URL, const char *headers, char *buffer, int bufferSize, GHIPost *post, int throttle,
    int blocking, ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void *param)
{
    GHIConnection *connection;
    int ok;

    if (URL == 0 || *URL == 0 || bufferSize < 0) {
        return -1;
    }
    if (buffer != 0 && bufferSize == 0) {
        return -1;
    }
    if (ghiReferenceCount == 0) {
        ghttpStartup();
    }
    connection = ghiNewConnection();
    if (connection == 0) {
        return -1;
    }
    connection->type = 0;
    connection->URL = _strdup(URL);
    if (connection->URL == 0) {
        goto fail;
    }
    if (headers != 0 && *headers != 0) {
        connection->sendHeaders = _strdup(headers);
        if (connection->sendHeaders == 0) {
            goto fail;
        }
    }
    connection->progressCallback = progressCallback;
    connection->throttle = throttle;
    connection->post = post;
    connection->blocking = blocking;
    connection->completedCallback = completedCallback;
    connection->callbackParam = param;
    connection->userBufferSupplied = buffer != 0;
    if (connection->userBufferSupplied) {
        ok = ghiInitFixedBuffer(connection, &connection->getFileBuffer, buffer, bufferSize);
    } else {
        ok = ghiInitBuffer(connection, &connection->getFileBuffer, 0x800, 0x800);
    }
    if (!ok || (post != 0 && !ghiPostInitState(connection))) {
        goto fail;
    }
    if (blocking != 0) {
        while (!ghiProcessConnection(connection)) {
            msleep(10);
        }
        return 0;
    }
    return connection->request;

fail:
    ghiFreeConnection(connection);
    return -1;
}
