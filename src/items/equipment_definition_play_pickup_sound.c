// equipment_definition_play_pickup_sound  (Ghidra: FUN_004bbbd0; renamed this pass)
// address 0x4bbbd0, size 83 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: the body is equipment_pickup_play_sound (0x4bbb50) minus the item_data.flags clear,
//   and it takes the tag id directly instead of resolving one from an object -- see
//   symbols/review_queue.txt 0x4bbbd0 "reads the function index directly from the tag
//   definition (in_EAX treated as a tag id, not an object id)".
//   The argument is proved by the single call site inside weapon_transfer_ammunition
//   (0x4c2610): objdump -d -M intel bin/halo.exe shows
//       4c27b9: mov eax, DWORD PTR [edx+0x18]
//       4c27bc: call 0x4bbbd0
//   where edx walks the 0x1c-stride WeaponMagazine.magazine_objects block, and
//   types/tags.h WeaponMagazineObject puts `TagDependency equipment` at +0x0c, i.e. its
//   .tag_id at +0x18. So the incoming value is an *Equipment* tag id, which makes tag+0x31c
//   exactly Equipment.pickup_sound.tag_id (types/tags.h: Item is 0x308, then powerup_type
//   0x308, grenade_type 0x30a, powerup_time 0x30c, pickup_sound TagDependency 0x310).
//   types/cache.h tag_instance (0x20 stride, tag data pointer at +0x14).
// register convention: Equipment tag id in EAX (in_EAX).
// UNSURE: as in equipment_pickup_play_sound, the second argument to the sound module's
//   FUN_00549af0 (0x549af0) is a parameter block no header in types/ describes yet; the
//   function initializes an int16 0 at +0x00 and two floats 1.0 at +0x04 and +0x08 and leaves
//   the rest of the frame untouched, so it stays a raw byte block here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "sound.h"

extern tag_instance *tag_instances; // 0x0087bc14

// 0x549af0, sound module; see src/items/equipment_pickup_play_sound.c for the parameter block
extern uint32_t sound_play_new(uint32_t sound_tag_id, void *parameters, uint32_t owner_index,
                             int32_t extra_size, void *extra, uint32_t extra_count,
                             uint32_t allow_deferred);

// Plays the pickup_sound of an Equipment tag named by tag id, with no live object involved.
void equipment_definition_play_pickup_sound(uint32_t equipment_tag_id) // blam-cc: EAX -> equipment_tag_id
{
    Equipment *tag;
    int32_t pickup_sound_tag_id;
    uint8_t parameters[16];

    tag = (Equipment *)tag_instances[equipment_tag_id & 0xffff].data;
    pickup_sound_tag_id = *(int32_t *)&tag->pickup_sound.tag_id;

    if (pickup_sound_tag_id != -1) {
        ((sound_location *)parameters)->type = 0;         // the 16-byte head of a sound_location
        ((sound_location *)parameters)->scale = 1.0f;
        ((sound_location *)parameters)->gain = 1.0f;
        sound_play_new((uint32_t)pickup_sound_tag_id, parameters, 0xffffffff, 0, 0, 0, 0);
    }
}

#if 0
Original Ghidra decompilation (0x4bbbd0):

void FUN_004bbbd0(void)

{
  int iVar1;
  uint in_EAX;
  undefined2 local_40 [2];
  undefined4 local_3c;
  undefined4 local_38;

  iVar1 = *(int *)(*(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x31c);
  if (iVar1 != -1) {
    local_40[0] = 0;
    local_3c = 0x3f800000;
    local_38 = 0x3f800000;
    FUN_00549af0(iVar1,local_40,0xffffffff,0,0,0,0);
  }
  return;
}
#endif
