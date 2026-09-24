// camera_observer_collect_target_candidates  (Ghidra: FUN_0045a0e0; renamed per
// symbols/review_queue.txt)
// address 0x45a0e0, size 401 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: types/objects.h object (bounding_radius 0xac == puVar1[0x2b], type 0xb4 ==
//   puVar1[0x2d], flags 0x10 == puVar1[4], vitality_flags 0x106 == puVar1[0x106/4... byte
//   access, _object_health_frozen_bit 0x4, next_object 0x114 == puVar1[0x45], first_child_object
//   0x118 == puVar1[0x46]); types/objects.h object_type_mask (_object_mask_unit 0x3,
//   _object_mask_biped 0x1); src/math/vector3d_projection_band_test.c (established signature);
//   types/tags.h Item.item_flags at tag+0x17c (Object tag base is 0x17c bytes). Recurses over a
//   per-cluster sibling list (next_object) and, for each biped/vehicle, either scores it
//   directly (bipeds only) via camera_observer_target_score, or walks into its children
//   (first_child_object) so a vehicle's passengers are still considered.
// register convention: all eleven of Ghidra's recognized parameters are genuine __cdecl stack
//   arguments (the caller pushes 11 dwords and cleans 0x2c); none of them is a register.
//   // blam-cc: stack -> the whole prototype below
//
// CORRECTED (phase 4 review) against
//   objdump -d -M intel --start-address=0x45a0e0 --stop-address=0x45a190 bin/halo.exe
// and the caller push sequence at 0x45a066..0x45a09f. Three things the previous version got
// wrong:
//   1. Ghidra param_9 (the OBSERVER team) was dropped as "dead" -- it is dead only in Ghidra
//      C output because teams_are_enemies (0x45bd50) takes both teams in registers. The
//      previous version therefore called teams_are_enemies(0, candidate_team), hard-coding the
//      observer onto team 0. It is restored here as observer_team and forwarded in the
//      recursive call, exactly as Ghidra own decompile forwards param_9.
//   2. vector3d_projection_band_test point_b is "lea edx,[ebx+0xa0]", i.e. the candidate
//      bounding_center (types/objects.h object +0x0a0), NOT its position (+0x05c).
//   3. params 5/6/7 are the caller max_distance / sin(max_angle) / cos(max_angle) locals, not
//      cone fields; renamed accordingly. The axis in EAX is param_4, the observer facing.
// UNSURE: vector3d_projection_band_test axis/point_a/point_b are the register arguments
// EAX/ECX/EDX; the disassembly shows eax = param_4, ecx = param_3, edx = &object[0xa0], which is
// what is written below, but the parameter NAMES of that function come from src/math.
// UNSURE: candidate_team is reconstructed via the candidate controlling_player -> player
// team, since teams_are_enemies is called with zero visible arguments and no per-object team
// field exists in types/units.h; not independently verified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *object_data;     // 0x008603b0
extern data_array *player_data;     // 0x0087a480
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t vector3d_projection_band_test(real_vector3d *axis, real_point3d *point_a, real_point3d *point_b,
    real param_1, real param_2, real param_3, real param_4); // 0x4cef90, math module
extern uint32_t camera_observer_target_score(observer_target_cone *cone, datum_index object,
    observer_target_candidate *out, real_point3d *reference_position); // this batch, 0x459b10
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // this batch, 0x45bd50

// Walks the sibling list starting at `start_object`, scoring every biped that passes the
// frustum/team/tag-flag filters into `out` (up to `capacity` entries) and recursing into every
// unit's children (so a vehicle's passengers are still visited even though the vehicle itself is
// never scored). Returns the number of entries written.
uint16_t camera_observer_collect_target_candidates(observer_target_cone *cone, datum_index start_object,
                                                    real_point3d *observer_position, real_vector3d *facing,
                                                    real max_distance, real sin_max_angle, real cos_max_angle,
                                                    datum_index exclude_object, int16_t observer_team,
                                                    int16_t capacity, observer_target_candidate *out)
    // blam-cc: stack -> cone, start_object, observer_position, facing, max_distance,
    //          sin_max_angle, cos_max_angle, exclude_object, observer_team, capacity, out
{
    datum_index object_index;
    object *obj;
    uint32_t type_bit;
    uint16_t count;
    observer_target_candidate temp;
    datum_index candidate_team_player;
    int16_t candidate_team;
    Item *tag;

    count = 0;
    object_index = start_object;
    do {
        obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        type_bit = 1u << (obj->type & 0x1f);
        if ((type_bit & _object_mask_unit) != 0 && (obj->flags & 1) == 0 &&
            *(real *)((uint8_t *)obj + 0x37c) < 1.0f) { // UNSURE: obj->flags bit 0 not named; +0x37c is
                                                         // past the common object header (0x1f4), likely a
                                                         // type-specific shield/camo fraction (TYPES-GAP)
            if (vector3d_projection_band_test(facing, observer_position, &obj->bounding_center,
                                               obj->bounding_radius, max_distance,
                                               sin_max_angle, cos_max_angle) != 0) {
                if ((type_bit & _object_mask_biped) != 0 && (obj->vitality_flags & _object_health_frozen_bit) == 0 &&
                    object_index != exclude_object) {
                    candidate_team_player = ((unit_data *)((uint8_t *)obj + k_unit_data_offset))->controlling_player;
                    candidate_team = -1;
                    if (candidate_team_player != k_datum_index_none) {
                        candidate_team = (int16_t)((player *)((uint8_t *)player_data->data + (candidate_team_player & 0xffff) * sizeof(player)))->team; // UNSURE: reused as player*, see note
                    }
                    if (teams_are_enemies(observer_team, candidate_team) != 0) {
                        tag = (Item *)tag_instances[obj->definition_tag & 0xffff].data;
                        if ((tag->item_flags & 0x200000) == 0) { // UNSURE: unnamed ItemFlags bit 21
                            if (camera_observer_target_score(cone, object_index, &temp, observer_position) != 0 &&
                                count < (uint16_t)capacity) {
                                out[count] = temp;
                                count = count + 1;
                            }
                        }
                    }
                }
                if (obj->first_child_object != k_datum_index_none && count < (uint16_t)capacity) {
                    count = count + camera_observer_collect_target_candidates(
                        cone, obj->first_child_object, observer_position, facing,
                        max_distance, sin_max_angle, cos_max_angle, exclude_object,
                        observer_team, (int16_t)(capacity - count), out + count);
                }
            }
        }
        object_index = obj->next_object;
    } while (object_index != k_datum_index_none && count < (uint16_t)capacity);
    return count;
}

#if 0
Original Ghidra decompilation (0x45a0e0), from tools/pack.py 0x45a0e0:

uint FUN_0045a0e0(undefined4 param_1,uint param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6,undefined4 param_7,uint param_8,
                 undefined4 param_9,int param_10,int param_11)

{
  uint *puVar1;
  char cVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 local_38 [14];

  uVar5 = 0;
  do {
    puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
    uVar3 = 1 << ((byte)puVar1[0x2d] & 0x1f);
    if ((((uVar3 & 3) != 0) && ((puVar1[4] & 1) == 0)) && ((float)puVar1[0xdf] < 1.0)) {
      cVar2 = vector3d_projection_band_test(puVar1[0x2b],param_5,param_6,param_7);
      if (cVar2 != '\0') {
        if ((((uVar3 & 1) != 0) && ((*(byte *)((int)puVar1 + 0x106) & 4) == 0)) &&
           (param_2 != param_8)) {
          cVar2 = FUN_0045bd50();
          if ((cVar2 != '\0') &&
             ((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) &
              0x200000) == 0)) {
            cVar2 = FUN_00459b10(param_1,param_3);
            if ((cVar2 != '\0') && ((short)uVar5 < (short)param_10)) {
              puVar6 = local_38;
              puVar7 = (undefined4 *)((short)uVar5 * 0x38 + param_11);
              for (iVar4 = 0xe; iVar4 != 0; iVar4 = iVar4 + -1) {
                *puVar7 = *puVar6;
                puVar6 = puVar6 + 1;
                puVar7 = puVar7 + 1;
              }
              uVar5 = uVar5 + 1;
            }
          }
        }
        if (puVar1[0x46] != 0xffffffff) {
          if ((short)uVar5 < (short)param_10) {
            iVar4 = FUN_0045a0e0(param_1,puVar1[0x46],param_3,param_4,param_5,param_6,param_7,
                                 param_8,param_9,param_10 - uVar5,(short)uVar5 * 0x38 + param_11);
            uVar5 = uVar5 + iVar4;
          }
        }
      }
    }
    param_2 = puVar1[0x45];
  } while ((param_2 != 0xffffffff) && ((short)uVar5 < (short)param_10));
  return uVar5 & 0xffff;
}
#endif
