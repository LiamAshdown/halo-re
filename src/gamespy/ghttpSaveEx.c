// ghttpSaveEx  (GameSpy SDK in halo.exe; no C existed)
// address 0x61bef0, size 289 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61bef0..0x61c010: (URL, file name, headers, post, throttle, blocking, progress,
//   completed, param): like ghttpGetEx but a save connection writing the file opened "wb" (after the post state).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern int ghiReferenceCount;             // 0x006a2e6c

int ghttpSaveEx(const char *URL, const char *filename, const char *headers, GHIPost *post, int throttle, int blocking,
    ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void *param)
{
    GHIConnection *connection;

    if (URL == 0 || *URL == 0 || filename == 0 || *filename == 0) {
        return -1;
    }
    if (ghiReferenceCount == 0) {
        ghttpStartup();
    }
    connection = ghiNewConnection();
    if (connection == 0) {
        return -1;
    }
    connection->type = 1;
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
    connection->completedCallback = completedCallback;
    connection->post = post;
    connection->blocking = blocking;
    connection->callbackParam = param;
    connection->throttle = throttle;
    if (post != 0 && !ghiPostInitState(connection)) {
        goto fail;
    }
    connection->saveFile = fopen(filename, "wb");
    if (connection->saveFile == 0) {
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
