// weapon_play_trigger_tag_effect  (Ghidra: weapon_play_trigger_tag_effect, already named)
// address 0x4c47d0, size 198 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: types/objects.h object.flags (_object_no_collision_bit), object.parent_object
//   (0x11c); types/cache.h tag_instance.group_tag; the two literal fourccs match 'effe'
//   (effect) and 'snd!' (sound) reversed on x86, matching every "sound,effect" TagDependency
//   comment in types/tags.h.
// register convention: item index in ECX; the tag id to play in EDI (unaff_EDI); slot and
// sub_index are Ghidra-recognized stack parameters threaded through to the effect player.
// blam-cc: ECX -> item_index, EDI -> tag_id, stack -> (slot, sub_index)
// UNSURE: effect_new_on_object is called with a literal 0xffffffff where the tag id would be expected;
// preserved literally rather than "corrected" to tag_id. sound_start_at_object_marker and
// effect_try_and_get's exact roles in the sound branch are not established.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint32_t effect_new_on_object(datum_index object_index, int32_t tag_id, int32_t slot, int32_t sub_index,
    int32_t a5, int32_t a6); // 0x4507a0, outside this module, UNSURE signature
extern void *effect_try_and_get(int32_t index); // 0x450630
extern void sound_start_at_object_marker(void); // 0x543ce0, outside this module, UNSURE signature

// Plays whichever tag (sound or effect) is referenced by a tag id, at the item's owning object
// (or its holder, when the item has no-collision and is attached). Returns the resulting effect
// handle for an 'effe' tag; sound playback always reports -1.
uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, int32_t slot, int32_t sub_index)
{
    object *item_obj;
    tag_group group;

    if (tag_id == (datum_index)0xffffffff) {
        return 0xffffffff;
    }

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    if ((item_obj->flags & _object_no_collision_bit) != 0 && item_obj->parent_object != (datum_index)0xffffffff) {
        item_index = item_obj->parent_object;
    }
    if (item_obj->parent_object != (datum_index)0xffffffff) {
        object_try_and_get(item_obj->parent_object, _object_mask_unit); // result discarded, see weapon_set_state.c
    }

    group = tag_instances[(uint16_t)tag_id].group_tag;
    if (group == 0x65666665) { // 'effe' (effect), fourcc bytes reversed on x86
        return effect_new_on_object(item_index, -1, slot, sub_index, 0, 0);
    }
    if (group == 0x736e6421) { // 'snd!' (sound)
        effect_try_and_get(0);
        sound_start_at_object_marker();
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
