// unit_begin_throw_grenade  (Ghidra: unit_begin_throw_grenade, already named)
// address 0x56e080, size 472 bytes, name confidence 0.5, rewrite confidence 0.6
// functions.md: "Initiates the unit's grenade-throw sequence: validates the current mode,
// records timing/aim data, and starts the throw animation state machine."
// evidence: types/units.h unit_data.current_weapon_index (0x2f2), .weapons[4] (0x2f8),
//   .current_grenade_index (0x31c), .grenade_counts[2] (0x31e), .animation_state (0x2a3),
//   .overlay_animation_command (0x2a4), .overlays[0] (0x2aa), .throwing_grenade_state (0x28d),
//   .throwing_grenade_counter (0x28e), .throwing_grenade_duration (0x290); types/objects.h
//   object.animation_index (0xd0), .animation_frame (0xd2); ModelAnimationsAnimation
//   (0xb4 stride, animations reflexive pointer at ModelAnimations+0x78, established in
//   unit_update_stance_and_jump.c); .key_frame_index at +0x34 (confirmed via offsetof).
// blam-cc: EDI -> unit_index, stack -> direction (a real_vector2d pointer or null; the draft read it as a flag).
// UNSURE: weapon_prevents_grenade_throwing/weapon_reset_triggers/unit_invalidate_local_player_zoom_level/effect_new_on_object's real roles are not recovered;
// the biped-extension byte at object+0x505 is out of this module's struct coverage.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "effects.h"
#include "fn_math.h"
#include "fn_items.h"
#include "fn_interface.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern Globals *global_globals;


extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source);
    // 0x4507a0, blam-cc: EAX -> creator_object_index, ECX -> definition_index, stack -> the other six
extern void unit_invalidate_local_player_zoom_level(uint32_t unit_index);                             // 0x4726f0, UNSURE signature


extern void weapon_reset_triggers(datum_index weapon_index);                        // 0x4c4b50, UNSURE signature
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern void unit_set_throw_aim_direction(uint32_t object_index, const real_vector2d *direction_xy); // 0x5704d0, EAX, ECX

uint8_t unit_begin_throw_grenade(uint32_t unit_index, const real_vector2d *direction) // blam-cc: EDI, stack
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;

    datum_index current_weapon = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    int16_t grenade_type = unit->current_grenade_index;
    if ((grenade_type == -1) || (unit->grenade_counts[grenade_type] <= 0)) {
        return 0;
    }

    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return 0;
    default:
        if (weapon_prevents_grenade_throwing(current_weapon) /* 0x56e11f: ECX = the weapon */ != 0) {
            return 0;
        }
        if (current_weapon != k_datum_index_none) {
            weapon_reset_triggers(current_weapon);
        }
        object *biped_check = object_try_and_get(unit_index, _object_mask_biped);
        if (biped_check != (object *)0) {
            *((uint8_t *)biped_check + 0x505) = 0; // UNSURE: biped-extension field, out of scope
        }
        unit->overlay_animation_command = 0;
        unit->overlays[0].animation_index = -1;

        if (unit_try_set_animation_state(unit_index, 0x21) == 0) { // UNSURE: state value assumed = throwing_grenade
            return 0;
        }

        unit->throwing_grenade_state = 1;
        unit->throwing_grenade_counter = 0;
        uint8_t *graph = (uint8_t *)tag_instances[unit_tag->base.animation_graph.tag_id.index & 0xffff].data;
        ModelAnimationsAnimation *animations = (ModelAnimationsAnimation *)(*(uint8_t **)&((ModelAnimations *)graph)->animations.pointer);
        unit->throwing_grenade_duration = (animations[unit_obj->animation_index].key_frame_index - unit_obj->animation_frame) + 1;

        // 0x56e1b9: a given direction aims the throw; without one, the unit's aiming vector (+0x23c) flattened to
        // 2D and normalized does, when it has any length.
        if (direction != 0) {
            unit_set_throw_aim_direction(unit_index, direction);
        } else {
            real_vector2d aim;

            aim.i = *(float *)((uint8_t *)unit_obj + 0x23c);
            aim.j = *(float *)((uint8_t *)unit_obj + 0x240);
            if (0.0f < vector2d_normalize_with_length(&aim)) {
                unit_set_throw_aim_direction(unit_index, &aim);
            }
        }
        weapon_action_notify_for_unit(unit_index, 0x11);
        unit_invalidate_local_player_zoom_level(unit_index);
        uint8_t *grenade_table_entry = ((uint8_t *)global_globals->grenades.pointer) + (int8_t)grenade_type * 0x44;
        if (*(int32_t *)(grenade_table_entry + 0x10) != -1) {
            // 0x56e22f..0x56e23c: EAX = the unit, ECX = the grenade entry's +0x10 effect, stack: unit, -1, 0..
            effect_new_on_object(unit_index, *(datum_index *)(grenade_table_entry + 0x10), unit_index, -1,
                0.0f, 0.0f, 0, 0);
        }
        return 1;
    }
}

#if 0
Original Ghidra decompilation (0x56e080):

undefined4 unit_begin_throw_grenade(int param_1)

{
  uint *puVar1;
  int iVar2;
  char cVar3;
  int iVar4;
  short sVar5;
  uint uVar6;
  uint unaff_EDI;
  float10 fVar7;

  iVar4 = (unaff_EDI & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar4);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar6 = 0xffffffff;
  if (*(short *)((int)puVar1 + 0x2f2) != -1) {
    uVar6 = puVar1[*(short *)((int)puVar1 + 0x2f2) + 0xbe];
  }
  iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar4);
  sVar5 = (short)*(char *)(iVar4 + 0x31c);
  if ((sVar5 != -1) && ('\0' < *(char *)(sVar5 + 0x31e + iVar4))) {
    switch(*(undefined1 *)((int)puVar1 + 0x2a3)) {
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x27:
    case 0x29:
      break;
    default:
      cVar3 = FUN_004c2f30();
      if (cVar3 == '\0') {
        if (uVar6 != 0xffffffff) {
          FUN_004c4b50(uVar6);
        }
        iVar4 = object_try_and_get(1);
        if (iVar4 != 0) {
          *(undefined1 *)(iVar4 + 0x505) = 0;
        }
        *(undefined1 *)(puVar1 + 0xa9) = 0;
        *(undefined2 *)((int)puVar1 + 0x2aa) = 0xffff;
        cVar3 = unit_try_set_animation_state();
        iVar4 = DAT_0087bc14;
        if (cVar3 != '\0') {
          *(undefined1 *)((int)puVar1 + 0x28d) = 1;
          *(undefined2 *)((int)puVar1 + 0x28e) = 0;
          *(short *)(puVar1 + 0xa4) =
               (*(short *)(*(int *)(*(int *)((*(uint *)(iVar2 + 0x44) & 0xffff) * 0x20 + 0x14 +
                                            iVar4) + 0x78) + 0x34 + (short)puVar1[0x34] * 0xb4) -
               *(short *)((int)puVar1 + 0xd2)) + 1;
          if ((param_1 != 0) ||
             (fVar7 = (float10)vector2d_normalize_with_length(), (float10)0.0 < fVar7)) {
            FUN_005704d0();
          }
          FUN_00492730(0x11);
          FUN_004726f0();
          if (*(int *)((char)puVar1[199] * 0x44 + *(int *)(DAT_00746fa0 + 300) + 0x10) != -1) {
            FUN_004507a0();
          }
          return 1;
        }
      }
    }
  }
  return 0;
}
#endif
