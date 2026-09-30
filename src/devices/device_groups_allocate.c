// device_groups_allocate  (not a Ghidra function; data-pool callback)
// address 0x44b620, size 31 bytes
// name confidence: 0.7  rewrite confidence: 0.95
// evidence: stored in a callback table in halo.exe's data and never made a Ghidra function; written by
//   tools/gen_pool_callbacks.py because its bytes (objdump 0x44b620..0x44b63f) match the pool initialize idiom exactly.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "fn_saved_games.h"

extern data_array *device_groups; // 0x0087abf0


void device_groups_allocate(void)
{
    device_groups = game_state_new("device groups", 0x400, 0x8);
}
