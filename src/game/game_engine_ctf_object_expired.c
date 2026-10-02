// game_engine_ctf_object_expired  (not a Ghidra function; the ctf game engine definition's +0x44 slot (object_expired); no C existed, so that
//   stored pointer trapped as unlisted_469960)
// address 0x469960, size 36 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469960..0x469983: the expiring flag object's +0xc0 dword becomes -1.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

void game_engine_ctf_object_expired(datum_index object_index)
{
    uint8_t *object = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;

    *(int32_t *)&((struct object *)object)->owner_linkage = -1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
