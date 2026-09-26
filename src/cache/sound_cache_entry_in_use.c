// sound_cache_entry_in_use  (not a Ghidra function; the "pc sound" cache callback)
// address 0x444060, size 54 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence: sound_cache_new 0x443ca0 passes 0x444060 to cache_new; the cache calls it cdecl with an entry handle. Only reachable as that
//   pointer; first-boot track: reached once textures started streaming for the UI map.
// objdump 0x444060..0x444095: in use while the page is still loading (+2 == 0), locked (+5) or playing (+6).
// blam-cc: stack -> handle (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"

extern data_array *sound_cache_entries; // 0x006ac528

uint8_t sound_cache_entry_in_use(datum_index handle)
{
    uint8_t *entry = (uint8_t *)sound_cache_entries->data + (handle & 0xffff) * 0x10;
    return entry[2] == 0 || entry[5] != 0 || entry[6] != 0;
}
