// object_new_from_scenario_name  (Ghidra: FUN_004f7370; formerly object_refresh_local_player_render_cache)
// address 0x4f7370, size 72 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence: objdump 0x4f7370..0x4f73b7: the scenario's object-name block (+0x208, 0x24 bytes per name) gives the
//   named object's type (+0x20) and placement index (+0x22); the type definition's placement block (+0x0a), size
//   (+0x0e) and palette (+0x0c) locate the placement, and object_new_from_scenario_placement 0x4f9b70 creates it
//   (EDI placement, stack palette); its result is returned. (The old version passed a NULL placement.)
// blam-cc: CX -> name_index
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *global_scenario; // 0x00746f8c
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern datum_index object_new_from_scenario_placement(uint8_t *placement, TagReflexive *palette); // 0x4f9b70,
    // blam-cc: EDI -> placement, stack -> palette

datum_index object_new_from_scenario_name(int16_t name_index)
{
    uint8_t *name = *(uint8_t **)(global_scenario + 0x208) + name_index * 0x24; // the names block's pointer field
    object_type_definition *definition = object_type_definitions[*(int16_t *)(name + 0x20)];
    TagReflexive *placements = (TagReflexive *)(global_scenario + definition->scenario_placement_offset);
    TagReflexive *palette = (TagReflexive *)(global_scenario + definition->scenario_palette_offset);
    uint8_t *placement = (uint8_t *)placements->pointer +
                         definition->scenario_placement_size * *(int16_t *)(name + 0x22);

    return object_new_from_scenario_placement(placement, palette);
}

#if 0
Original Ghidra decompilation (0x4f7370):

void FUN_004f7370(void)

{
  short in_CX;

  FUN_004f9b70(*(short *)((&PTR_PTR_0069bfdc)
                          [*(short *)(*(int *)(DAT_00746f8c + 0x208) + in_CX * 0x24 + 0x20)] + 0xc)
               + DAT_00746f8c);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
