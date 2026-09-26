// antenna_delete  (not a Ghidra function; an object widget type callback)
// address 0x4fac80, size 14 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object widget type table (records of 0x28 from 0x0069c010: fourcc, flag, initialize, dispose,
//   clear_disposing_flag, reset, new, delete, update, render) slot 0x69c054 = delete entry of 'ant!'. Only reachable
//   through that table. First-boot track: placing a campaign level's objects (weapons carry widgets).
// objdump 0x4fac80..0x4fac8e: datum_delete(antenna_data, index), no -1 check.
// blam-cc: stack -> antenna_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern void datum_delete(data_array *array, datum_index index); // 0x4d0510, blam-cc: EAX -> array, EDX -> index
extern data_array *antenna_data; // 0x008603ac

void antenna_delete(datum_index antenna_index)
{
    datum_delete(antenna_data, antenna_index);
}
