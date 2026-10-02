// equipment_pickup_play_sound  (Ghidra: FUN_004bbb50; renamed per types/items.h, which already
// spells this address "equipment_pickup_play_sound (0x4bbb50)")
// address 0x4bbb50, size 114 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: types/items.h item_flags._item_unknown_40_bit (0x40; item_data.flags is the uint32
//   at object 0x1f4, so Ghidra's `puVar1 + 0x7d` is exactly that word) and
//   out/phase4/items_types_notes.md "equipment (mask 0x008, Equipment tag 0x31c)";
//   types/tags.h Equipment.pickup_sound is the TagDependency at 0x310, so its .tag_id is the
//   int32 at 0x31c that this function tests against -1; types/objects.h object.definition_tag
//   (0x000); types/cache.h tag_instance (0x20 stride, tag data pointer at +0x14).
//   Call sites confirm the "pickup" reading: 0x479a40 (item consumed by a unit), 0x56d080
//   (powerup applied then object_delete) and 0x56d1a0 (item attached to a new holder, which
//   then calls item_set_holder) all guard the call on a real player/unit being involved and
//   then delete the item.
// register convention: object index in EAX (in_EAX), the same bare-EAX object accessor
//   convention every other item accessor in this module uses.
// UNSURE: FUN_00549af0 is the sound module's "play sound by tag id" entry point (0x549af0,
//   15 callers, calls sound_permutation_pick_random / sound_cache_touch). Its second argument
//   is a small sound-parameter block that no header in types/ describes yet; this function
//   initializes exactly three fields of it (an int16 0 at +0x00 and two floats 1.0 at +0x04
//   and +0x08) and leaves the rest of the frame untouched, so it is kept here as a raw byte
//   block rather than inventing a `sound_parameters` type inside types/items.h.
// UNSURE: item_flags bit 0x40 has no setter and no reader anywhere in the export (see
//   out/phase4/items_types_notes.md "Unresolved offsets"), so the clear is preserved
//   literally without a name for what it means.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// 0x549af0, sound module; see file header for the parameter block
extern uint32_t sound_play_new(uint32_t sound_tag_id, void *parameters, uint32_t owner_index,
                             int32_t extra_size, void *extra, uint32_t extra_count,
                             uint32_t allow_deferred);

// Clears item_flags bit 0x40 on an item that is being picked up, then plays the Equipment tag's
// pickup_sound if the tag has one.
void equipment_pickup_play_sound(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj;
    item_data *item;
    Equipment *tag;
    int32_t pickup_sound_tag_id;
    uint8_t parameters[16];

    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    tag = (Equipment *)tag_instances[obj->definition_tag & 0xffff].data;

    item = (item_data *)((uint8_t *)obj + k_item_data_offset);
    item->flags &= ~(uint32_t)_item_unknown_40_bit;

    pickup_sound_tag_id = *(int32_t *)&tag->pickup_sound.tag_id;
    if (pickup_sound_tag_id != -1) {
        ((sound_location *)parameters)->type = 0;         // the 16-byte head of a sound_location
        ((sound_location *)parameters)->scale = 1.0f;
        ((sound_location *)parameters)->gain = 1.0f;
        sound_play_new((uint32_t)pickup_sound_tag_id, parameters, 0xffffffff, 0, 0, 0, 0);
    }
}

#if 0
Original Ghidra decompilation (0x4bbb50):

void FUN_004bbb50(void)

{
  uint *puVar1;
  int iVar2;
  uint in_EAX;
  undefined2 local_40 [2];
  undefined4 local_3c;
  undefined4 local_38;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  puVar1 = puVar1 + 0x7d;
  *puVar1 = *puVar1 & 0xffffffbf;
  iVar2 = *(int *)(iVar2 + 0x31c);
  if (iVar2 != -1) {
    local_40[0] = 0;
    local_3c = 0x3f800000;
    local_38 = 0x3f800000;
    FUN_00549af0(iVar2,local_40,0xffffffff,0,0,0,0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
