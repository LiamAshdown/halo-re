// ghiPostGetHasFilesContentLength  (GameSpy SDK in halo.exe; no C existed)
// address 0x6221c0, size 383 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6221c0..0x62233e: the multipart body length: fixed parts (set once: boundary
//   0x25, string part 0x54, file part 0x71, end 0x29) plus names, file names, content types and data lengths (a disk
//   file  from its posting state); an unknown field type gives 0.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

static int boundaryLength;       // 0x006a3290
static int filePartLength;       // 0x006a3294
static int stringPartLength;     // 0x006a3298
static int endLength;            // 0x006a329c

int ghiPostGetHasFilesContentLength(GHIConnection *connection)
{
    DArray data = connection->post->data;
    int total = 0;
    int count;
    int i;

    if (boundaryLength == 0) {
        boundaryLength = 0x25;
        stringPartLength = 0x54;
        filePartLength = 0x71;
        endLength = 0x29;
    }
    count = ArrayLength(data);
    for (i = 0; i < count; i++) {
        GHIPostData *field = (GHIPostData *)ArrayNth(data, i);

        if (field->type == 0) {
            total += (int)strlen(field->name) + field->data.string.len + stringPartLength;
        } else if (field->type == 1) {
            total += (int)strlen(field->data.fileDisk.contentType) + (int)strlen(field->data.fileDisk.reportFilename) +
                     (int)strlen(field->name) + filePartLength;
            total += ((GHIPostState *)ArrayNth(connection->postingStates, i))->len;
        } else if (field->type == 2) {
            total += field->data.fileMemory.len + (int)strlen(field->data.fileMemory.contentType) +
                     (int)strlen(field->data.fileMemory.reportFilename) + (int)strlen(field->name) + filePartLength;
        } else {
            return 0;
        }
    }
    return endLength + total;
}
