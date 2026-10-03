// lights_initialize
// address 0x4f0a20, size 115 bytes
// name confidence: 0.7 (out/phase4/objects_functions.md, corroborated by the "lights"/"light"
// strings)
// rewrite confidence: 0.45
// evidence: types/objects.h globals list (light_data 0x00860b14, lights_enabled 0x0071cfb8);
// types/memory.h data_array (used by game_state_new's return, size 0x380 == k_maximum_lights).
// UNSURE: DAT_006e2dc8/DAT_006e2dcc/DAT_006e2dd4 are a game-state checksum registration table
// (registering a fresh 4-byte checksummed region and folding its size into a running crc32);
// they belong to the memory/cache module, not this one, and are kept as raw externs.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "structures.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *light_data;  // 0x00860b14
extern uint8_t *lights_enabled; // 0x0071cfb8

extern uint8_t *game_state_base;  // 0x006e2dc8, UNSURE: checksum region base, memory/cache module
extern int32_t game_state_cursor;  // 0x006e2dcc, UNSURE: checksum region cursor, memory/cache module
extern uint32_t game_state_crc;  // 0x006e2dd4, UNSURE: running crc32 accumulator, memory/cache module

extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // memory module, 0x4d02d0
extern void cluster_partition_new(cluster_reference_group *out, char *name); // 0x551e30, blam-cc: ESI -> out, EDI -> name
extern cluster_reference_group light_cluster_first; // 0x00860b20 (light_cluster_first, then the two pools)

void lights_initialize(void)
{
    data_array *new_light_data = game_state_new((char *)"lights", k_maximum_lights, 0x7c /* EBX at the original call */);
    uint8_t *checksum_slot = game_state_base + game_state_cursor;
    uint32_t size_marker = 4;

    game_state_cursor = game_state_cursor + 4;
    light_data = new_light_data;
    crc32_update(&game_state_crc, (uint8_t *)&size_marker, 4);
    lights_enabled = checksum_slot;
    *checksum_slot = 1;

    if (new_light_data != 0) {
        cluster_partition_new(&light_cluster_first, (char *)"light"); // 0x4f0a7f: EDI "light" 0x0066e6bc, ESI 0x00860b20
    }
}

#if 0
Original Ghidra decompilation (0x4f0a20):

void lights_initialize(void)

{
  undefined1 *puVar1;
  int iVar2;
  undefined4 local_4;

  iVar2 = game_state_new("lights",0x380);
  puVar1 = (undefined1 *)(DAT_006e2dcc + DAT_006e2dc8);
  DAT_006e2dcc = DAT_006e2dcc + 4;
  local_4 = 4;
  DAT_00860b14 = iVar2;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_0071cfb8 = puVar1;
  *puVar1 = 1;
  if (iVar2 != 0) {
    cluster_partition_new();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
