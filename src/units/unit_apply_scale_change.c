// unit_apply_scale_change  (Ghidra: unit_apply_scale_change)
// address 0x562030, size 326 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.2
// evidence: types/objects.h object.body_vitality/shield_vitality (0xe0/0xe4), object.flags
//   (0x10, _object_unknown_20000_bit), object.vitality_flags (0x106, _object_health_frozen_bit),
//   object.animation_index/animation_frame (0xd0/0xd2), Object.animation_graph (tag+0x44) ->
//   the same +0x78 flat ModelAnimationsAnimation array used by
//   object_animation_get_frames_remaining.c, frame_count at ModelAnimationsAnimation+0x22;
//   types/units.h unit_data.grenade_counts (0x31e), .equipment_object_index (0x318),
//   .animation_state (0x2a3), .flags (0x204, _unit_flag_unknown_200, cited on that enumerator
//   for exactly this address), .unknown_41c (0x41c, "game tick stamp taken by 0x562030").
// register convention: unit index in EAX, a {scale, flags} record in ECX.
//   // blam-cc: in_EAX -> unit_index, in_ECX -> request (float scale at +0x0, flags at +0x4)
// UNSURE: the `(uint)*floatPtr` cast Ghidra emits when writing into body_vitality is a known
//   decompiler artifact for a plain 4-byte copy into a field it did not know was a float here
//   (compare object_apply_body_damage.c and similar files in src/objects); reproduced as a
//   direct float assignment, not a truncating conversion. `unit_scale_request`'s shape (a float
//   then a flags dword) is inferred purely from the two dereferences Ghidra shows.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern hs_game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/hs.h)

extern void object_set_shield_depleted_flag(void);              // 0x4edb10, UNSURE: no traced args  // real signature (object_set_shield_depleted_flag.c): void object_set_shield_depleted_flag(uint32_t object_index); Ghidra recovered 0 of 1 args at this call site
extern void object_delete(uint32_t object_index);                // 0x4f5bd0, UNSURE signature
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0, index in a register
extern void unit_update_stance_and_jump(void);                                  // 0x566de0, UNSURE: no traced args (unit seat overlay c)  // real signature (unit_update_stance_and_jump.c): void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction, uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle, int16_t weapon_class_index, int32_t fire_trigger_event, uint8_t require_still); Ghidra recovered 0 of 10 args at this call site
extern void unit_drop_inventory_weapons_except_current(void);                                  // 0x56d360, UNSURE: no traced args  // real signature (unit_drop_inventory_weapons_except_current.c): void unit_drop_inventory_weapons_except_current(uint32_t unit_index); Ghidra recovered 0 of 1 args at this call site

// unit_scale_request is declared in types/units.h (shape still UNSURE -- see the header).

void unit_apply_scale_change(uint32_t unit_index, unit_scale_request *request) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    if (request->scale > 0.0f) {
        obj->body_vitality = request->scale; // UNSURE: see file header
    }
    if ((request->flags & 1) != 0) {
        unit_update_stance_and_jump();
        if (unit->animation_state == 0x19) {
            unit_drop_inventory_weapons_except_current();
            unit->grenade_counts[0] = 0; // clears the 0x31e int16 pair as one write
            unit->grenade_counts[1] = 0;
            if (unit->equipment_object_index != (datum_index)-1) {
                object_delete(unit_index); // UNSURE: probably takes the equipment handle, not unit_index
                unit->equipment_object_index = (datum_index)-1;
            }
            // Same "+0x78 flat animation array" chain as object_animation_get_frames_remaining.c,
            // but resolved via the tag's own animation_graph instead of the runtime object's.
            void *graph = tag_instances[obj_tag->animation_graph.tag_id.index].data;
            uint8_t *animations = *(uint8_t **)((uint8_t *)graph + 0x78);
            ModelAnimationsAnimation *anim =
                (ModelAnimationsAnimation *)(animations + obj->animation_index * 0xb4);
            int32_t remaining = (int32_t)anim->frame_count - 4;
            obj->vitality_flags = obj->vitality_flags | _object_health_frozen_bit;
            unit->flags = unit->flags | _unit_flag_unknown_200;
            obj->animation_frame = (int16_t)((remaining < 0) ? 0 : remaining);
            obj->flags = obj->flags | _object_unknown_20000_bit;
            unit->unknown_41c = game_time->current_tick;
            obj->body_vitality = 0.0f;
            obj->shield_vitality = 0.0f;
            object_set_shield_depleted_flag();
            object_recalculate_bounding_radius_recursive(unit_index); // UNSURE: register-carried
        }
    }
}

#if 0
Original Ghidra decompilation (0x562030):

void FUN_00562030(void)

{
  uint *puVar1;
  uint in_EAX;
  int iVar2;
  float *in_ECX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (0.0 < *in_ECX) {
    puVar1[0x38] = (uint)*in_ECX;
  }
  if (((uint)in_ECX[1] & 1) != 0) {
    FUN_00566de0();
    if (*(char *)((int)puVar1 + 0x2a3) == '\x19') {
      FUN_0056d360();
      *(undefined2 *)((int)puVar1 + 0x31e) = 0;
      if (puVar1[0xc6] != 0xffffffff) {
        object_delete();
        puVar1[0xc6] = 0xffffffff;
      }
      iVar2 = *(short *)((short)puVar1[0x34] * 0xb4 +
                         *(int *)(*(int *)((*(uint *)(iVar2 + 0x44) & 0xffff) * 0x20 + 0x14 +
                                          DAT_0087bc14) + 0x78) + 0x22) + -4;
      *(byte *)((int)puVar1 + 0x106) = *(byte *)((int)puVar1 + 0x106) | 4;
      puVar1[0x81] = puVar1[0x81] | 0x200;
      *(ushort *)((int)puVar1 + 0xd2) = (ushort)iVar2 & (iVar2 < 0) - 1;
      puVar1[4] = puVar1[4] | 0x20000;
      puVar1[0x107] = *(uint *)(DAT_006f1d6c + 0xc);
      puVar1[0x38] = 0;
      puVar1[0x39] = 0;
      object_set_shield_depleted_flag();
      object_recalculate_bounding_radius_recursive();
    }
  }
  return;
}
#endif
