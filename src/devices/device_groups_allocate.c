// device_groups_allocate  (not a Ghidra function; data-pool callback)
// address 0x44b620, size 31 bytes
// name confidence: 0.7  rewrite confidence: 0.95
// evidence: stored in a callback table in halo.exe's data and never made a Ghidra function; written by
//   tools/gen_pool_callbacks.py because its bytes (objdump 0x44b620..0x44b63f) match the pool initialize idiom exactly.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *device_groups; // 0x0087abf0
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count

void device_groups_allocate(void)
{
    device_groups = game_state_new((char *)"device groups", 0x400, 0x8);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
