// ov_close_thunk  (not a Ghidra function; an Ogg Vorbis memory-stream callback)
// address 0x544dc0, size 22 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: sound_ogg_stream_open 0x544eb0 hands 0x544dc0 to ov_open_callbacks as the close member of its
//   ov_callbacks (read/seek/close/tell order); only reachable through that pointer. First-boot track:
//   the UI map's streamed sounds run it.
// objdump 0x544dc0..0x544dd5: -1 without a file; otherwise drops the data pointer (+4) and returns 0.
// blam-cc: stack -> file (cdecl)

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t ov_close_thunk(sound_ogg_memory_file *file)
{
    if (file == 0) {
        return -1;
    }
    file->data = 0;
    return 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
