// game_engine_reset_respawns_and_cleanup_bipeds  (Ghidra: FUN_00467e60; named per this rewrite --
// sharpens out/phase4/game_functions.md's guess: "In multiplayer, resets a per-entry duration
// field across a data table and then sweeps all game objects, deleting stray biped/weapon-type
// garbage.")
// address 0x467e60, size 267 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x467e60
//   --stop-address=0x467f70), which fills in everything Ghidra's decompile drops:
//     - the player loop uses a local data_iterator passed in EDI (src/memory/data_iterator_next
//       .c's real convention, not the "()" Ghidra shows), same construction (and same dead
//       "iter"-XOR dword) as game_engine_reset_all_unit_grenade_counts.c;
//     - player_kill_and_release_unit (0x476250, not in this batch) is called with the player handle in EBX
//       (loaded from the iterator's own `index` field right before the call) and the clamped
//       respawn time on the stack;
//     - the object sweep iterates with object_iterator::type_mask == 0x001
//       (types/objects.h `_object_mask_biped`), matching object_iterator_next's own documented
//       (stack) convention; object::network_role (+0x004, "object_delete dispatches on 0 versus
//       3") is the "+4 type field" functions.md's summary refers to -- it is not an object type
//       at all, so this rewrite corrects "biped/weapon-type garbage" to "role 0 or 3 bipeds".
//   game_variant::respawn_time (0x006f1cd0) and player::respawn_timer (+0x2c) are already named
//   in types/game.h.
// register convention: no parameters.
// UNSURE: player_kill_and_release_unit's real name/signature/behavior; why role 0 also calls
//   object_delete_unparented before object_delete_recursive while role 3 only calls the latter.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include <stdint.h>

extern int16_t network_game_mode;               // 0x00719720
extern data_array *player_data;                 // 0x0087a480
extern game_variant game_engine_variant;        // 0x006f1c88
extern data_array *object_data;              // 0x008603b0

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern object *object_iterator_next(object_iterator *iterator);   // 0x4f6f20
extern void object_delete_unparented(datum_index object_index);      // 0x4f5aa0, blam-cc: EDI
extern void object_delete_recursive(datum_index object_index, uint8_t recurse_siblings); // 0x4f59d0

extern void player_kill_and_release_unit(int32_t respawn_time); // 0x476250, blam-cc: EBX -> player_handle, stack -> respawn_time; UNSURE

// While hosting a multiplayer game, forwards every player (with its clamped respawn_time, >= 90
// ticks) to player_kill_and_release_unit and resets their respawn_timer to 0; then, unconditionally, sweeps
// every live biped object and deletes any whose network_role is 0 (also via
// object_delete_unparented first) or 3 (via object_delete_recursive).
void game_engine_reset_respawns_and_cleanup_bipeds(void)
{
    if (network_game_mode == 2) {
        data_iterator player_iter;
        uint32_t unused_checksum; // UNSURE: written, never read back
        player *p;

        player_iter.data = player_data;
        player_iter.next_index = 0;
        player_iter.index = (datum_index)0xffffffff;
        player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
        unused_checksum = (uint32_t)player_data ^ 0x69746572;

        p = (player *)data_iterator_next(&player_iter);
        while (p != (player *)0) {
            int32_t respawn_time = game_engine_variant.respawn_time;
            if (respawn_time < 0x5a) {
                respawn_time = 0x5a;
            }
            player_kill_and_release_unit(respawn_time); // blam-cc: EBX = player_iter.index, set by the caller
            p->respawn_timer = 0;
            p = (player *)data_iterator_next(&player_iter);
        }
        (void)unused_checksum;
    }

    {
        object_iterator obj_iter;
        object *obj;

        obj_iter.type_mask = 0x001; // _object_mask_biped
        obj_iter.flags_mask = 0;
        obj_iter.unknown_05 = 0;
        obj_iter.index = 0;
        obj_iter.handle = (datum_index)0xffffffff;

        obj = object_iterator_next(&obj_iter);
        while (obj != (object *)0) {
            if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
                if (obj->network_role == 0) {
                    object_delete_unparented(obj_iter.handle);
                    object_delete_recursive(obj_iter.handle, 0);
                } else if (obj->network_role == 3) {
                    object_delete_recursive(obj_iter.handle, 0);
                }
            }
            obj = object_iterator_next(&obj_iter);
        }
    }
}

#if 0
Original Ghidra decompilation (0x467e60), from tools/pack.py 0x467e60:

void FUN_00467e60(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint local_14;
  ushort local_10;
  undefined2 local_e;
  uint local_c;
  uint local_8;

  if (DAT_00719720 == 2) {
    local_14 = DAT_0087a480;
    local_8 = DAT_0087a480 ^ 0x69746572;
    local_10 = 0;
    local_c = 0xffffffff;
    iVar2 = data_iterator_next();
    while (iVar2 != 0) {
      iVar3 = DAT_006f1cd0;
      if (DAT_006f1cd0 < 0x5b) {
        iVar3 = 0x5a;
      }
      FUN_00476250(iVar3);
      *(undefined4 *)(iVar2 + 0x2c) = 0;
      iVar2 = data_iterator_next();
    }
  }
  local_8 = 0x86868686;
  local_14 = 1;
  local_10 = local_10 & 0xff00;
  local_e = 0;
  local_c = 0xffffffff;
  iVar2 = object_iterator_next(&local_14);
  uVar1 = local_c;
  do {
    if (iVar2 == 0) {
      return;
    }
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
    local_c = uVar1;
    if ((*(byte *)(iVar2 + 0x106) & 4) != 0) {
      iVar2 = *(int *)(iVar2 + 4);
      if (iVar2 == 0) {
        object_delete_unparented();
      }
      else if (iVar2 != 3) goto LAB_00467f53;
      object_delete_recursive(uVar1,0);
    }
LAB_00467f53:
    iVar2 = object_iterator_next(&local_14);
    uVar1 = local_c;
  } while( true );
}
#endif
