// objects_initialize
// address 0x4f4ad0, size 212 bytes
// name confidence: 0.9 (CEA/PDB match via strings "object"/"objects"/"collideable object"/
//   "noncollideable object"; matches functions.md's summary)
// rewrite confidence: 0.75
// evidence: types/objects.h globals list (object_data 0x008603b0, object_memory_pool
//   0x006b8cb4, object_globals_pointer 0x006b8cbc, object_name_list 0x006b8cb8,
//   k_maximum_objects 0x800, object_globals size 0x98); callees widgets_initialize 0x4ff9d0,
//   object_type_definition_chain_build 0x4f3db0 (this batch), lights_initialize 0x4f0a20,
//   game_state_new/game_state_new_pool/cluster_partition_new (memory/cluster modules),
//   crc32_update (memory module).
// register convention: none (void), matches Ghidra's __cdecl void(void) signature.
// UNSURE: the two cluster_partition_new() calls take no visible arguments in the decompilation,
//   yet 0x008603d0 and 0x008603c0 (the collideable/noncollideable cluster-first tables) are in
//   this function's global-reference list; how those two globals get initialized is not visible
//   here and is not guessed at.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "structures.h"

extern data_array *object_data; // 0x008603b0
extern memory_pool *object_memory_pool; // 0x006b8cb4
extern object_globals *object_globals_pointer; // 0x006b8cbc
extern datum_index *object_name_list; // 0x006b8cb8

extern uint8_t *game_state_base; // 0x006e2dc8, UNSURE: foreign (memory/checksum) module global
extern int32_t game_state_cursor;     // 0x006e2dcc, UNSURE: running offset into that table
extern uint32_t game_state_crc;         // 0x006e2dd4, passed to crc32_update by address

extern void widgets_initialize(void); // 0x4ff9d0
extern void object_type_definition_chain_build(void); // 0x4f3db0, this batch
extern void lights_initialize(void); // 0x4f0a20
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count
extern memory_pool *game_state_new_pool(char *name, int32_t pool_size); // 0x538150, stack name, EBX pool_size
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length);
extern void cluster_partition_new(cluster_reference_group *out, char *name); // 0x551e30, blam-cc: ESI -> out, EDI -> name
extern cluster_reference_group collideable_cluster_group; // 0x008603d0
extern cluster_reference_group noncollideable_cluster_group; // 0x008603c0

void objects_initialize(void)
{
    uint8_t *globals_region;
    uint8_t *name_list_region;
    int32_t size;

    widgets_initialize();
    object_type_definition_chain_build();
    lights_initialize();
    object_data = game_state_new("object", k_maximum_objects, 0xc /* EBX at the original call */);
    // FIXED (objdump 0x4f4af7..0x4f4b06): EBX = 0x200000, the 2 MB object pool. The draft passed no size, so the pool
    //   (and every game-state allocation after it) got a garbage size.
    object_memory_pool = game_state_new_pool("objects", 0x200000);

    globals_region = game_state_base + game_state_cursor;
    game_state_cursor = game_state_cursor + 0x98;
    size = 0x98;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    name_list_region = game_state_base + game_state_cursor;
    game_state_cursor = game_state_cursor + 0x800;
    size = 0x800;
    object_globals_pointer = (object_globals *)globals_region;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    object_name_list = (datum_index *)name_list_region;

    // 0x4f4b81..0x4f4b9a: EDI the category name, ESI the group
    cluster_partition_new(&collideable_cluster_group, "collideable object");
    cluster_partition_new(&noncollideable_cluster_group, "noncollideable object");
}

#if 0
Original Ghidra decompilation (0x4f4ad0):

void __cdecl objects_initialize(void)

{
  int iVar1;
  int iVar2;
  undefined4 local_4;

  widgets_initialize();
  FUN_004f3db0();
  lights_initialize();
  DAT_008603b0 = game_state_new("object",0x800);
  DAT_006b8cb4 = game_state_new_pool("objects");
  iVar1 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x98;
  local_4 = 0x98;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  iVar2 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x800;
  local_4 = 0x800;
  DAT_006b8cbc = iVar1;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006b8cb8 = iVar2;
  cluster_partition_new();
  cluster_partition_new();
  return;
}
#endif
