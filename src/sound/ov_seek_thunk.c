// ov_seek_thunk  (not a Ghidra function; an Ogg Vorbis memory-stream callback)
// address 0x544da0, size 29 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: sound_ogg_stream_open 0x544eb0 hands 0x544da0 to ov_open_callbacks as the seek member of its
//   ov_callbacks (read/seek/close/tell order); only reachable through that pointer. First-boot track:
//   the UI map's streamed sounds run it.
// objdump 0x544da0..0x544dbc: forwards to sound_ogg_seek_callback 0x544e00 with ESI = file, EAX = whence and
//   the 64-bit offset's two halves on the stack; returns its result.
// blam-cc: stack -> file, offset_low, offset_high, whence (cdecl)

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t sound_ogg_seek_callback(sound_ogg_memory_file *file, uint32_t offset_low, int32_t offset_high,
    int32_t whence); // 0x544e00, blam-cc: ESI -> file, EAX -> whence, stack -> (offset_low, offset_high)

int32_t ov_seek_thunk(sound_ogg_memory_file *file, uint32_t offset_low, int32_t offset_high, int32_t whence)
{
    return sound_ogg_seek_callback(file, offset_low, offset_high, whence);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
