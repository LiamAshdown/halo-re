// ov_read_thunk  (not a Ghidra function; an Ogg Vorbis memory-stream callback)
// address 0x544d50, size 79 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: sound_ogg_stream_open 0x544eb0 hands 0x544d50 to ov_open_callbacks as the read member of its
//   ov_callbacks (read/seek/close/tell order); only reachable through that pointer. First-boot track:
//   the UI map's streamed sounds run it.
// objdump 0x544d50..0x544d9e: size * count bytes, or 0 without a destination or once end_of_file is set. A
//   request past the end is cut to what is left and sets end_of_file. memcpy (0x6236f0) from data + position,
//   then position advances; returns the byte count.
// blam-cc: stack -> destination, size, count, file (cdecl)

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern void *memcpy(void *destination, const void *source, uint32_t size); // 0x6236f0

uint32_t ov_read_thunk(void *destination, uint32_t size, uint32_t count, sound_ogg_memory_file *file)
{
    uint32_t bytes = size * count;
    uint32_t remaining;

    if (destination == 0 || file->end_of_file) {
        return 0;
    }
    remaining = (uint32_t)(file->size - file->position);
    if (bytes > remaining) {
        bytes = remaining;
        file->end_of_file = 1;
    }
    memcpy(destination, (uint8_t *)file->data + file->position, bytes);
    file->position = file->position + bytes;
    return bytes;
}
