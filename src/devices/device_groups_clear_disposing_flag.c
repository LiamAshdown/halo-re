// device_groups_clear_disposing_flag  (not a Ghidra function; data-pool callback)
// address 0x44b660, size 10 bytes
// name confidence: 0.7  rewrite confidence: 0.95
// evidence: stored in a callback table in halo.exe's data and never made a Ghidra function; written by
//   tools/gen_pool_callbacks.py because its bytes (objdump 0x44b660..0x44b66a) match the clear-disposing-flag idiom exactly.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"

extern data_array *device_groups; // 0x0087abf0

void device_groups_clear_disposing_flag(void)
{
    device_groups->valid = 0;
}
