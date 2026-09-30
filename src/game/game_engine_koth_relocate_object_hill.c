// game_engine_koth_relocate_object_hill  (Ghidra: FUN_0046c1a0; named per its summary)
// address 0x46c1a0, size 143 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("In team play, finds a new hill location for the given
//   object index, plays a related sound when few hills have been used, and clears the object's
//   'needs relocation' flag"); game_engine_variant::ball_count aliased 0x006f1d18;
//   game_engine_koth_find_marker_position (0x46beb0, this batch); ctf_flag_object_clear_carrier
//   (0x4666c0, already committed, blam-cc EBX -> flag_object_index, EDI -> position -- both
//   elided here and modeled as forwarded, per the same pattern used throughout this batch's CTF
//   helpers).
// register convention: object handle in in_EAX; flag_object_index/position forwarded straight
//   through to ctf_flag_object_clear_carrier.
//   // blam-cc: EAX -> object_index, EBX -> forwarded_flag_object_index, EDI ->
//   //   forwarded_position
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "fn_game.h"

extern int16_t network_game_mode;        // 0x00719720
extern data_array *object_data;       // 0x008603b0
extern game_variant game_engine_variant; // 0x006f1c88 (ball_count aliased 0x006f1d18)


// FIXED 2026-09-28: 0x46c1b1 copies EAX into EBX and 0x46c20f points EDI at the position 0x46beb0 found (type
//   filter: the object's +0xb8, 0x46c1cb) before ctf_flag_object_clear_carrier; nothing is forwarded.
// blam-cc: EAX -> object_index
// While hosting, finds a new (discarded) hill position, plays a sound if fewer than 3 hills have
// been used so far, forwards flag_object_index/position to ctf_flag_object_clear_carrier, and
// clears an equipment-runtime bit on the object.
void game_engine_koth_relocate_object_hill(uint32_t object_index)
{
    if (network_game_mode == 2) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        real_point3d discarded_position;

        game_engine_koth_find_marker_position(&discarded_position, ((object *)obj)->owner_team);

        if (game_engine_variant.ball_count < 3) {
            game_engine_queue_multiplayer_sound(0x1e, 0xffffffff, 1); // 0x46c1fd..0x46c207
        }
        ctf_flag_object_clear_carrier(object_index, &discarded_position);
        *(uint32_t *)((uint8_t *)obj + 0x22c) &= 0xffffffbf;
    }
}

#if 0
Original Ghidra decompilation (0x46c1a0), from tools/pack.py 0x46c1a0:

void FUN_0046c1a0(void)

{
  int iVar1;
  uint in_EAX;
  undefined1 local_c [12];

  if (DAT_00719720 == 2) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    FUN_0046beb0(local_c);
    if (DAT_006f1d18 < 3) {
      game_engine_queue_multiplayer_sound(1);
    }
    ctf_flag_object_clear_carrier();
    *(uint *)(iVar1 + 0x22c) = *(uint *)(iVar1 + 0x22c) & 0xffffffbf;
  }
  return;
}
#endif
