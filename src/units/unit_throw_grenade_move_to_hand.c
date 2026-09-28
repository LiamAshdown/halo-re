// unit_throw_grenade_move_to_hand  (Ghidra: unit_throw_grenade_move_to_hand, already named)
// address 0x56e280, size 446 bytes, name confidence 0.65, rewrite confidence 0.85 (placement block REWRITTEN from objdump; gate verified)
// functions.md: "Spawns the grenade projectile object for a throw and attaches it to the unit's
// left-hand marker, advancing the throw-state machine."
// evidence: types/units.h unit_data.controlling_player (0x218), .actor_index (0x1f4),
//   .current_grenade_index (0x31c), .grenade_counts[2] (0x31e), .throwing_grenade_state (0x28d),
//   .throwing_grenade_projectile (0x294); types/objects.h object.network_role (0x04),
//   object_marker (node_index at +0x00); the cea-pdb "left hand" string match confirms the name.
// blam-cc: param_1 -> unit_index.
// UNSURE: the network-prediction gate at the top (controlling_player/DAT_0087abc2/
// current_game_engine/DAT_0087aa00/DAT_006f1cc0) is reproduced literally without
// naming every bit; object_new_with_datum_role_control's, object_placement_data_initialize's
// and object_attach_to_object's exact field layouts are only partially recovered (local_f8,
// local_70 and the angular-velocity locals are kept as raw byte buffers).
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern Globals *global_globals;
extern uint8_t weapon_bottomless_clip; // 0x0087abc2
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint32_t game_engine_unknown_aa00;    // 0x0087aa00
extern uint32_t motion_sensor_override_value;         // 0x006f1cc0
extern int16_t network_game_mode;         // 0x00719720 (a WORD; 0x719722 is the screenshot counter)

extern real vector3d_normalize_with_length(real_vector3d *v);                      // 0x401990
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir);    // 0x4cd670, UNSURE signature
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag, datum_index role); // 0x4f53a0, UNSURE signature
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0, UNSURE signature
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t param_4); // 0x4f6080
extern void object_attach_to_object(uint32_t parent_index, uint32_t child_index, int16_t marker_index); // 0x4f6440, UNSURE signature

void unit_throw_grenade_move_to_hand(uint32_t unit_index)
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    int8_t grenade_type = unit->current_grenade_index;
    uint8_t *grenade_table = (uint8_t *)global_globals->grenades.pointer;

    if (((unit->controlling_player == k_datum_index_none) ||
         ((weapon_bottomless_clip == 0) &&
          ((current_game_engine == 0) || ((game_engine_unknown_aa00 & 4) != 0) ||
           ((motion_sensor_override_value >> 2 & 1) == 0)))) &&
        (unit->actor_index == k_datum_index_none) &&
        ((unit_obj->network_role == 3) || (unit_obj->network_role == 0))) {
        unit->grenade_counts[grenade_type] -= 1;
    }

    if ((network_game_mode != 2) && (network_game_mode != 0)) {
        unit->throwing_grenade_projectile = k_datum_index_none;
        unit->throwing_grenade_state = 2;
        return;
    }

    // REWRITTEN from objdump 0x56e344..0x56e3d6: the placement (a full 0x88-byte object_placement_data) gets
    //   position = the world "left hand" marker position (marker +0x60), forward = the unit's aiming vector
    //   (+0x23c) and up = normalize(perpendicular(forward)). The draft wrote a perpendicular into +0x20 and
    //   left the position at the unit origin.
    object_marker hand_marker;
    object_get_node_local_transform(unit_index, "left hand", &hand_marker, 1);

    object_placement_data placement;
    object_placement_data_initialize(&placement, *(datum_index *)(grenade_table + grenade_type * 0x44 + 0x40),
                                     unit_index);
    placement.flags |= 2;
    placement.forward = *(real_vector3d *)((uint8_t *)unit_obj + 0x23c);
    vector3d_build_perpendicular(&placement.up, &placement.forward);
    vector3d_normalize_with_length(&placement.up);
    placement.position = *(real_point3d *)((uint8_t *)&hand_marker + 0x60); // node_transform.position

    uint32_t projectile_index = object_new_with_datum_role_control(&placement, 3);
    if (projectile_index != 0xffffffff) {
        object_attach_to_object(unit_index, projectile_index, hand_marker.node_index);
        unit->throwing_grenade_projectile = projectile_index;
        unit->throwing_grenade_state = 2;
        object *projectile_obj = ((object_header *)object_data->data)[projectile_index & 0xffff].data;
        *((uint8_t *)projectile_obj + 0x278) = 1; // UNSURE: projectile-specific "held in hand" flag
        return;
    }
    unit->throwing_grenade_state = 3;
    return;
}

#if 0
Original Ghidra decompilation (0x56e280):

void unit_throw_grenade_move_to_hand(uint param_1)

{
  char *pcVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  undefined1 local_f8 [4];
  uint local_f4;
  undefined4 local_e0;
  undefined4 local_dc;
  undefined4 local_d8;
  undefined4 local_c4;
  undefined4 local_c0;
  undefined4 local_bc;
  undefined4 local_70 [24];
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;

  iVar6 = (param_1 & 0xffff) * 0xc;
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  cVar2 = *(char *)(iVar3 + 0x31c);
  iVar4 = *(int *)(DAT_00746fa0 + 300);
  if ((((*(int *)(iVar3 + 0x218) == -1) ||
       ((DAT_0087abc2 == '\0' &&
        (((DAT_006f1d20 == 0 || ((DAT_0087aa00 & 4) != 0)) || ((DAT_006f1cc0 >> 2 & 1) == 0)))))) &&
      (*(int *)(iVar3 + 500) == -1)) && ((*(int *)(iVar3 + 4) == 3 || (*(int *)(iVar3 + 4) == 0))))
  {
    pcVar1 = (char *)(cVar2 + 0x31e + iVar3);
    *pcVar1 = *pcVar1 + -1;
  }
  if ((DAT_00719720 != 2) && (DAT_00719720 != 0)) {
    *(undefined4 *)(iVar3 + 0x294) = 0xffffffff;
    *(undefined1 *)(iVar3 + 0x28d) = 2;
    return;
  }
  object_get_node_local_transform(param_1,"left hand",local_70,1);
  object_placement_data_initialize(*(undefined4 *)(cVar2 * 0x44 + iVar4 + 0x40),param_1);
  local_f4 = local_f4 | 2;
  iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  local_c4 = *(undefined4 *)(iVar4 + 0x23c);
  local_c0 = *(undefined4 *)(iVar4 + 0x240);
  local_bc = *(undefined4 *)(iVar4 + 0x244);
  vector3d_build_perpendicular();
  vector3d_normalize_with_length();
  local_e0 = local_10;
  local_dc = local_c;
  local_d8 = local_8;
  uVar5 = object_new_with_datum_role_control(local_f8,3);
  if (uVar5 != 0xffffffff) {
    object_attach_to_object(param_1,uVar5,local_70[0]);
    iVar4 = DAT_008603b0;
    *(uint *)(iVar3 + 0x294) = uVar5;
    *(undefined1 *)(iVar3 + 0x28d) = 2;
    *(undefined1 *)(*(int *)(*(int *)(iVar4 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc) + 0x278) = 1;
    return;
  }
  *(undefined1 *)(iVar3 + 0x28d) = 3;
  return;
}
#endif
