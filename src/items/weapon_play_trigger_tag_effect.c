// weapon_play_trigger_tag_effect  (Ghidra: weapon_play_trigger_tag_effect, already named)
// address 0x4c47d0, size 198 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: types/objects.h object.flags (_object_no_collision_bit), object.parent_object
//   (0x11c); types/cache.h tag_instance.group_tag; the two literal fourccs match 'effe'
//   (effect) and 'snd!' (sound) reversed on x86, matching every "sound,effect" TagDependency
//   comment in types/tags.h.
// blam-cc: ECX -> item_index, EDI -> tag_id, stack -> (scale_a, scale_b)
// VERIFIED 2026-09-27 against objdump 0x4c47d0..0x4c4895 and all six call sites: weapon_fire_trigger passes
// (trigger firing_rate or 1.0, heat fraction); weapon_ready (+0x348 ready_effect), fire_or_reload / begin_reload
// (magazine +0x44 reloading_effect), begin_chamber (+0x54 chambering_effect) and reload_recovery_finish
// (+0x390 overheat_detonation) pass (0, 0). The sound branch's early `push 0` (0x4c4848) is a fifth stack slot
// that 0x543ce0 never reads; 0x450630 takes only EDX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "effects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source);
    // 0x4507a0, blam-cc: EAX -> creator_object_index, ECX -> definition_index, stack -> the other six
extern void *effect_try_and_get(datum_index effect_index); // 0x450630, blam-cc: EDX
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward,
    datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint);
    // 0x543ce0, blam-cc: ESI -> object_index, ECX -> position, EAX -> forward, stack -> the rest
extern const real_point3d *global_zero_vector3d_pointer; // 0x006966f8
extern const real_vector3d *global_forward3d_pointer;   // 0x00696718

// Plays whichever tag (sound or effect) is referenced by a tag id, at the item's owning object
// (or its holder, when the item has no-collision and is attached). Returns the resulting effect
// handle for an 'effe' tag; sound playback always reports -1.
// REWRITTEN (from objdump 0x4c47d0..0x4c4894): the creator is the item's parent when that parent is a unit
//   (object_try_and_get mask 3), else -1; the effect or sound attaches to the item, or to its parent when the
//   item has no collision. The two stack arguments are raw float bits (a/b scale for an effect, the scale of a
//   sound). Effect: effect_new_on_object(EAX creator, ECX tag, stack: object, -1, a, b, 0, 0). Sound:
//   effect_try_and_get(EDX tag) (result unused) then sound_start_at_object_marker(ESI creator, ECX = the zero
//   point, EAX = the forward vector, stack: tag, -1, a, 0). Anything else returns -1.
uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, real scale_a, real scale_b)
{
    object *item_obj;
    tag_group group;
    datum_index attach_to = item_index;
    datum_index creator = k_datum_index_none;
    real a_scale = scale_a;
    real b_scale = scale_b;

    if (tag_id == (datum_index)0xffffffff) {
        return 0xffffffff;
    }
    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    if ((item_obj->flags & _object_no_collision_bit) != 0 && item_obj->parent_object != (datum_index)0xffffffff) {
        attach_to = item_obj->parent_object;
    }
    if (item_obj->parent_object != (datum_index)0xffffffff &&
        object_try_and_get(item_obj->parent_object, _object_mask_unit) != 0) {
        creator = item_obj->parent_object;
    }
    group = tag_instances[(uint16_t)tag_id].group_tag;
    if (group == 0x65666665) { // 'effe'
        return effect_new_on_object(creator, tag_id, attach_to, -1, a_scale, b_scale, 0, 0);
    }
    if (group == 0x736e6421) { // 'snd!'
        effect_try_and_get(tag_id);
        sound_start_at_object_marker(creator, (Point3D *)global_zero_vector3d_pointer,
            (Vector3D *)global_forward3d_pointer, tag_id, -1, a_scale, 0);
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4c47d0):

undefined4 weapon_play_trigger_tag_effect(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 uVar2;
  uint in_ECX;
  int unaff_EDI;

  if (unaff_EDI != -1) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    if (((*(byte *)(iVar1 + 0x10) & 1) != 0) && (*(uint *)(iVar1 + 0x11c) != 0xffffffff)) {
      in_ECX = *(uint *)(iVar1 + 0x11c);
    }
    if (*(int *)(iVar1 + 0x11c) != -1) {
      object_try_and_get(3);
    }
    iVar1 = *(int *)((short)unaff_EDI * 0x20 + DAT_0087bc14);
    if (iVar1 == 0x65666665) {
      uVar2 = FUN_004507a0(in_ECX,0xffffffff,param_1,param_2,0,0);
      return uVar2;
    }
    if (iVar1 == 0x736e6421) {
      particle_system_try_and_get(0);
      FUN_00543ce0();
    }
  }
  return 0xffffffff;
}
#endif
