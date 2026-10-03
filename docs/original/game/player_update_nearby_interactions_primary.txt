// player_update_nearby_interactions_primary  (Ghidra: FUN_00478400; renamed per
// out/phase4/game_functions.md: "Scans nearby objects around a unit and routes each to the
// appropriate interaction check (vehicle flip/steal, vehicle boarding, or assassination).")
// address 0x478400, size 212 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x478400..0x4784d3 (sphere search + the type jump table 0x4784d4: 1 vehicle, 2/3 boarding, 8 assassination).)
// evidence: out/phase4/game_functions.md; types/tags.h ObjectType (type 1 = vehicle); types/
//   objects.h object::location_leaf_index (0x098), object::bounding_center/bounding_radius
//   (0x0a0/0x0ac), object::type (0x0b4); the established object_find_in_sphere signature
//   (src/game/game_engine_location_blocked_by_vehicle.c); player_check_vehicle_interaction (this
//   batch's renamed FUN_00478600) and player_check_assassination_opportunity (this batch,
//   already named) are reused for cases 1 and 8; case 2/3 goes to player_check_vehicle_boarding_interaction (this batch's
//   sibling player_check_vehicle_boarding_interaction).
// register convention: a player index in EDI (unaff_EDI); no stack parameters.
//   // blam-cc: EDI -> player_index
// UNSURE: case 8 corresponds to ObjectType 8 (objecttype_device_control per types/tags.h), not
//   a biped, despite the dispatched handler's name; preserved exactly as decompiled rather than
//   reinterpreted.

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
extern void player_check_vehicle_boarding_interaction(uint32_t player_index, uint32_t candidate_object); // 0x4788a0, this
    // module. CORRECTED by review: Ghidra elides both arguments, but 0x4788a0's own entry reads
    // [esp+0x8] (its second stack argument) and the call site is "push ecx ; push edi ; call",
    // exactly like its two siblings below.
extern void player_check_assassination_opportunity(uint32_t player_index,
    uint32_t candidate_object); // 0x478770, this module. CORRECTED by review for the same
    // reason: 0x478770 reads [esp+0x4] as a player handle (and 0x8 as the candidate).

// blam-cc: EDI -> player_index
// Finds up to 16 objects near `player_index`'s unit's bounding sphere (only when that unit has
// no parent) and, for each, dispatches on its object_type: vehicle (1) to
// player_check_vehicle_interaction, weapon/equipment (2 or 3) to player_check_vehicle_boarding_interaction, and type 8 to
// player_check_assassination_opportunity.
void player_update_nearby_interactions_primary(uint32_t player_index)
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
                    player_check_vehicle_boarding_interaction(player_index, candidates[i]);
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
Original Ghidra decompilation (0x478400), from tools/pack.py 0x478400:

void FUN_00478400(void)

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
        FUN_004788a0();
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
