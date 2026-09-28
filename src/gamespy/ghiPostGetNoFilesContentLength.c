// ghiPostGetNoFilesContentLength  (GameSpy SDK in halo.exe; no C existed)
// address 0x622150, size 108 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622150..0x6221bb: EAX connection: the url-encoded body length: per field name,
//   "=", the value with 2 more per extended character, "&" between.
// blam-cc: EAX -> connection

#include "gamespy.h"

#include "ghttp.h"

int ghiPostGetNoFilesContentLength(GHIConnection *connection)
{
    DArray data = connection->post->data;
    int count = ArrayLength(data);
    int total = 0;
    int i;

    if (count == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        GHIPostData *field = (GHIPostData *)ArrayNth(data, i);

        total += (int)strlen(field->name) + field->data.string.len + field->data.string.extendedChars * 2 + 1;
    }
    return total + count - 1;
}
