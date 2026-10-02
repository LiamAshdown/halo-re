// player_update_nearby_interactions_secondary  (Ghidra: FUN_00478500; renamed per
// out/phase4/game_functions.md: "A second nearby-interaction scan that uses a lighter-weight
// vehicle-boarding check than the primary variant." -- and player_check_vehicle_boarding_interaction_lightweight's own summary names
// this exact function as its caller.)
// address 0x478500, size 212 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: identical shape to the sibling player_update_nearby_interactions_primary.c (this
// batch), differing only in the case-2/3 handler (player_check_vehicle_boarding_interaction_lightweight instead of FUN_004788a0).
// register convention: a player index in EDI (unaff_EDI); no stack parameters.
//   // blam-cc: EDI -> player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0

extern int16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location,
    real_point3d *center, float radius, datum_index *out_objects, int16_t max_output); // 0x4f6fe0
extern void player_check_vehicle_interaction(uint32_t player_index, uint32_t candidate_object); // this batch, 0x478600
extern void player_check_vehicle_boarding_interaction_lightweight(uint32_t player_index, uint32_t candidate_object); // 0x478c40, this
    // module. CORRECTED by review: the call site at 0x4785b5 is "push ecx ; push edi ; call",
    // the same two arguments its two siblings get.
extern void player_check_assassination_opportunity(uint32_t player_index,
    uint32_t candidate_object); // 0x478770, this module; same correction (0x4785be).

// blam-cc: EDI -> player_index
// As player_update_nearby_interactions_primary, but routes object_type 2/3 candidates to the
// lightweight player_check_vehicle_boarding_interaction_lightweight instead of FUN_004788a0.
void player_update_nearby_interactions_secondary(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    datum_index unit_index = p->unit;

    if (unit_index == (datum_index)0xffffffff) {
        return;
    }

    {
        object *unit = (object *)((object_header *)object_data->data)[unit_index & 0xffff].data;

        if (unit->parent_object == (datum_index)0xffffffff) {
            datum_index candidates[16];
            int16_t count = object_find_in_sphere(0, 0x11f, &unit->location_leaf_index,
                &unit->bounding_center, unit->bounding_radius, candidates, 0x10);
            int16_t i;

            for (i = 0; i < count; i++) {
                object *candidate = (object *)((object_header *)object_data->data)[candidates[i] & 0xffff].data;
                switch (candidate->type) {
                case 1:
                    player_check_vehicle_interaction(player_index, candidates[i]);
                    break;
                case 2:
                case 3:
                    player_check_vehicle_boarding_interaction_lightweight(player_index, candidates[i]);
                    break;
                case 8:
                    player_check_assassination_opportunity(player_index, candidates[i]);
                    break;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x478500), from tools/pack.py 0x478500:

void FUN_00478500(void)

{
  int iVar1;
  ushort uVar2;
  uint uVar3;
  uint *puVar4;
  uint unaff_EDI;
  uint local_40 [16];

  uVar3 = *(uint *)((unaff_EDI & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34);
  if (((uVar3 != 0xffffffff) &&
      (iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc),
      *(int *)(iVar1 + 0x11c) == -1)) &&
     (uVar2 = object_find_in_sphere
                        (0,0x11f,iVar1 + 0x98,iVar1 + 0xa0,*(undefined4 *)(iVar1 + 0xac),local_40),
     0 < (short)uVar2)) {
    puVar4 = (uint *)&stack0xffffffbc;
    uVar3 = (uint)uVar2;
    do {
      puVar4 = puVar4 + 1;
      switch(*(undefined2 *)
              (*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*puVar4 & 0xffff) * 0xc) + 0xb4)) {
      case 1:
        FUN_00478600();
        break;
      case 2:
      case 3:
        FUN_00478c40();
        break;
      case 8:
        player_check_assassination_opportunity();
      }
      uVar3 = uVar3 - 1;
    } while (uVar3 != 0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
