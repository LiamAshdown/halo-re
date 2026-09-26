// flag_delete  (not a Ghidra function; an object widget type callback)
// address 0x4fb970, size 14 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object widget type table (records of 0x28 from 0x0069c010: fourcc, flag, initialize, dispose,
//   clear_disposing_flag, reset, new, delete, update, render) slot 0x69c02c = delete entry of 'flag'. Only reachable
//   through that table. First-boot track: placing a campaign level's objects (weapons carry widgets).
// objdump 0x4fb970..0x4fb97e: datum_delete(flag_data, index), no -1 check.
// blam-cc: stack -> flag_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern void datum_delete(data_array *array, datum_index index); // 0x4d0510, blam-cc: EAX -> array, EDX -> index
extern data_array *flag_data; // 0x008603a8

void flag_delete(datum_index flag_index)
{
    datum_delete(flag_data, flag_index);
}
