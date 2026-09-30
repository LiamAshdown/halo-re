// device_groups_dispose  (not a Ghidra function; data-pool callback)
// address 0x44b640, size 22 bytes
// name confidence: 0.7  rewrite confidence: 0.95
// evidence: stored in a callback table in halo.exe's data and never made a Ghidra function; written by
//   tools/gen_pool_callbacks.py because its bytes (objdump 0x44b640..0x44b656) match the pool dispose idiom exactly.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "fn_memory.h"

extern data_array *device_groups; // 0x0087abf0

extern void device_groups_initialize(void); // 0x44c220 (tail call)

void device_groups_dispose(void)
{
    device_groups->valid = 1;
    data_delete_all(device_groups);
    device_groups_initialize();
}
