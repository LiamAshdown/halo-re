// ghiParseURL  (GameSpy SDK in halo.exe; no C existed)
// address 0x620e30, size 212 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620e30..0x620f03: an "http://" URL: the host (copied), the port after ":" (0
//   fails; default 80) and the path from the first "/" ("/" when none) with spaces turned into "+".
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiParseURL(GHIConnection *connection)
{
    char *url;
    char *end;
    char save;
    char *path;
    char *space;

    if (connection == 0 || connection->URL == 0 || strncmp(connection->URL, "http://", 7) != 0) {
        return 0;
    }
    url = connection->URL + 7;
    end = url + strcspn(url, ":/");
    save = *end;
    *end = 0;
    connection->serverAddress = _strdup(url);
    if (connection->serverAddress == 0) {
        return 0;
    }
    *end = save;
    path = end;
    if (*path == ':') {
        path++;
        connection->serverPort = (unsigned short)atoi(path);
        if (connection->serverPort == 0) {
            return 0;
        }
        do {
            path++;
            if (*path == 0) {
                path = "/";
                goto have_path;
            }
        } while (*path != '/');
    } else {
        connection->serverPort = 80;
    }
    if (*path == 0) {
        path = "/";
    }
have_path:
    connection->requestPath = _strdup(path);
    for (space = strchr(connection->requestPath, ' '); space != 0; space = strchr(connection->requestPath, ' ')) {
        *space = '+';
    }
    return connection->requestPath != 0;
}
