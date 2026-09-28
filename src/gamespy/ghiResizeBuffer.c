// ghiResizeBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x622ab0, size 49 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622ab0..0x622ae0: EDI buffer, EAX increment: grows the data by the increment
//   (realloc); 0 for no buffer, a non-positive increment or no memory.
// blam-cc: EDI -> buffer, EAX -> sizeIncrement

#include "gamespy.h"

#include "ghttp.h"

int ghiResizeBuffer(GHIBuffer *buffer, int sizeIncrement)
{
    char *data;
    int size;

    if (buffer == 0 || sizeIncrement <= 0) {
        return 0;
    }
    size = buffer->size + sizeIncrement;
    data = (char *)realloc(buffer->data, size);
    if (data == 0) {
        return 0;
    }
    buffer->data = data;
    buffer->size = size;
    return 1;
}
