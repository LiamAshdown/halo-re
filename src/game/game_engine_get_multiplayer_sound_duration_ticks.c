// game_engine_get_multiplayer_sound_duration_ticks  (Ghidra:
//   game_engine_get_multiplayer_sound_duration_ticks, already named)
// address 0x46bde0, size 87 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: types/tags.h GlobalsMultiplayerInformation::sounds (TagReflexive, count/pointer at
//   +0x5c/+0x60, GlobalsSound stride 0x10, TagDependency at +0x0c); types/cache.h tag_instance
//   (stride 0x20, data at +0x14); Sound tag's own duration-ish field at +0x84 (frames?),
//   converted here to ticks by *30/1000 (30 Hz from a millisecond-ish unit).
// register convention: sound index in in_EAX.
//   // blam-cc: EAX -> sound_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "fn_game.h"

extern Globals *global_globals;     // 0x00746fa0
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EAX -> sound_index
// Returns the playback duration, in 30 Hz game ticks, of GlobalsMultiplayerInformation's
// announcer sound `sound_index`, or 0 if the index is out of range or has no tag.
int32_t game_engine_get_multiplayer_sound_duration_ticks(int32_t sound_index)
{
    GlobalsMultiplayerInformation *mp_info =
        (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
    uint8_t *sound;
    uint32_t tag_id;

    if (mp_info == (GlobalsMultiplayerInformation *)0 || sound_index >= (int32_t)mp_info->sounds.count) {
        return 0;
    }
    sound = (uint8_t *)mp_info->sounds.pointer + sound_index * 0x10;
    if (sound == (uint8_t *)0) {
        return 0;
    }
    tag_id = *(uint32_t *)(sound + 0xc);
    if (tag_id == 0xffffffff) {
        return 0;
    }
    return (*(int32_t *)((uint8_t *)tag_instances[tag_id & 0xffff].data + 0x84) * 30) / 1000;
}

#if 0
Original Ghidra decompilation (0x46bde0), from tools/pack.py 0x46bde0:

int game_engine_get_multiplayer_sound_duration_ticks(void)

{
  uint uVar1;
  int in_EAX;
  int iVar2;

  iVar2 = *(int *)(DAT_00746fa0 + 0x168);
  if ((((iVar2 != 0) && (in_EAX < *(int *)(iVar2 + 0x5c))) &&
      (iVar2 = in_EAX * 0x10 + *(int *)(iVar2 + 0x60), iVar2 != 0)) &&
     (uVar1 = *(uint *)(iVar2 + 0xc), uVar1 != 0xffffffff)) {
    return (*(int *)(*(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x84) * 0x1e) / 1000;
  }
  return 0;
}
#endif
