// ghiAppendDataToBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x622be0, size 231 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622be0..0x622cc6: appends (0 length: strlen) keeping a NUL after, growing by
//   the increment while needed; a fixed buffer overflowing fails the connection with buffer overflow (2), no memory
//   with out of memory (1).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiAppendDataToBuffer(GHIBuffer *buffer, const char *data, int dataLen)
{
    int newLen;

    if (buffer == 0 || data == 0 || dataLen < 0) {
        return 0;
    }
    if (dataLen == 0) {
        dataLen = (int)strlen(data);
    }
    newLen = buffer->len + dataLen;
    while (newLen >= buffer->size) {
        char *grown;
        int size;

        if (buffer->fixed != 0) {
            buffer->connection->completed = 1;
            buffer->connection->result = 2;
            return 0;
        }
        if (buffer->sizeIncrement <= 0) {
            goto out_of_memory;
        }
        size = buffer->size + buffer->sizeIncrement;
        grown = (char *)realloc(buffer->data, size);
        if (grown == 0) {
            goto out_of_memory;
        }
        buffer->data = grown;
        buffer->size = size;
    }
    memcpy(buffer->data + buffer->len, data, dataLen);
    buffer->len = newLen;
    buffer->data[newLen] = 0;
    return 1;

out_of_memory:
    buffer->connection->completed = 1;
    buffer->connection->result = 1;
    return 0;
}
