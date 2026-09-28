// ghiHandleStatus  (GameSpy SDK in halo.exe; no C existed)
// address 0x61bb60, size 102 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61bb60..0x61bbc5: ESI connection: an HTTP error status becomes the result --
//   401 unauthorized (9), 403 forbidden (10), 404/410 file not found (11), other 4xx server error (8), 5xx server
//   error 12.
// blam-cc: ESI -> connection

#include "gamespy.h"

#include "ghttp.h"

void ghiHandleStatus(GHIConnection *connection)
{
    switch (connection->statusCode / 100) {
    case 4:
        switch (connection->statusCode) {
        case 401:
            connection->result = 9;
            break;
        case 403:
            connection->result = 10;
            break;
        case 404:
        case 410:
            connection->result = 11;
            break;
        default:
            connection->result = 8;
            break;
        }
        break;
    case 5:
        connection->result = 12;
        break;
    }
}
