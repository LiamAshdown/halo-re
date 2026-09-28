// ghiPostGetContentType  (GameSpy SDK in halo.exe; no C existed)
// address 0x622120, size 38 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622120..0x622145: "" without a post; multipart/form-data with the boundary, or
//   application/x-www-form-urlencoded.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

const char *ghiPostGetContentType(GHIConnection *connection)
{
    if (connection->post == 0) {
        return "";
    }
    if (connection->post->useMultipart != 0) {
        return "multipart/form-data; boundary=Qr4G823s23d---<<><><<<>--7d118e0536";
    }
    return "application/x-www-form-urlencoded";
}
