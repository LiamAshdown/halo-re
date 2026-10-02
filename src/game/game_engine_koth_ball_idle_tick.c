// game_engine_koth_ball_idle_tick  (Ghidra: FUN_0046c5d0; named per its summary)
// address 0x46c5d0, size 382 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Checks whether a hill has gone unclaimed for too
//   long and, if so, announces it via the kill feed and relocates it; otherwise records the
//   current tick as the hill's last-active time"); the CEA/PDB string hint ("ball_blue",
//   oddball_engine_weapon_update) suggests this logic is shared with (or mirrors) Oddball's
//   ball-idle handling. types/objects.h object.flags (_object_needs_cluster_update_bit 0x800,
//   _object_at_rest_bit 0x20, _object_changed_bit 0x4000000), parent_object (0x11c);
//   item_data.held_game_time (0x204); game_engine_koth_relocate_object_hill.c (0x46c1a0, this
//   batch); a second 16-entry per-team-slot table at 0x006b124c, immediately after
//   king_hill_occupant_table (0x006b120c, this batch); 0x006883a0 is an idle-timeout constant.
// register convention: object handle in param_1; object pointer in param_2 (both ordinary
//   stack parameters per Ghidra's own signature).
// UNSURE: weapon_must_be_readied's identity (a validity gate); the relocate call (game_engine_koth_
//   relocate_object_hill) needs two more forwarded parameters this function's body never
//   touches, modeled as "no specific flag/position" like its own sibling call sites.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c
extern int16_t network_game_mode;    // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88 (unknown_8c aliased 0x006f1d14)
extern uint32_t king_hill_occupant_table[16];    // 0x006b120c, this batch
extern int32_t king_hill_occupant_last_tick[16]; // 0x006b124c
extern int32_t king_hill_idle_timeout;           // 0x006883a0

extern uint8_t item_get_effective_position(datum_index object_index, real_point3d *out_position); // 0x4bd740
extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position,
    float height_offset, datum_index player_filter, int16_t team_filter); // 0x462260
extern void game_engine_koth_relocate_object_hill(uint32_t object_index); // 0x46c1a0, blam-cc: EAX object_index
extern uint8_t weapon_must_be_readied(void); // 0x4c2ea0, UNSURE exact identity
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30

void game_engine_koth_ball_idle_tick(uint32_t object_handle, object *obj)
{
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);
    real_point3d position;
    object_header *hdr;
    int32_t tick;

    if (item_get_effective_position((datum_index)object_handle, &position) != 1) {
        return;
    }

    hdr = (object_header *)object_data->data + (object_handle & 0xffff);
    if ((hdr->flags & 0x08) != 0) { // UNSURE: header flags bit 0x08 (_object_header_delete_pending_bit)
        return;
    }

    custom_waypoint_register((datum_index)0xffffffff, (int16_t)0, &position, 0.0f,
        (datum_index)0xffffffff, (int16_t)0xffffffff); // UNSURE forwarded owner/slot, see header

    tick = game_time->game_time;
    if ((uint32_t)(game_time->game_time - item->held_game_time) > 0x4b0) {
        if (network_game_mode != 2) {
            return;
        }
        if (weapon_must_be_readied() == 0 || (obj->flags >> 0xb & 1) == 0 || obj->parent_object != (datum_index)0xffffffff) {
            goto check_relocation;
        }
        if ((*(uint8_t *)((uint8_t *)obj + 0x22c) & 0x40) != 0) {
            data_iterator iter;
            void *element;
            iter.data = object_data; // UNSURE: iterator source not resolved
            iter.next_index = 0;
            iter.index = (datum_index)0xffffffff;
            iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
            element = data_iterator_next(&iter);
            while (element != 0) {
                chimera__kill_feed((datum_index)0xffffffff, 0x26, (uint32_t)0xffffffff, 1, 0);
                element = data_iterator_next(&iter);
            }
        }
        game_engine_koth_relocate_object_hill(object_handle);
    }
    tick = game_time->game_time;
    if (network_game_mode != 2) {
        return;
    }
check_relocation:
    if (game_engine_variant.engine.oddball.ball_type < 1 || game_engine_variant.engine.oddball.ball_type > 2) {
        int16_t team = ((object *)obj)->owner_team;
        if (king_hill_occupant_last_tick[team] == -1 ||
            king_hill_occupant_last_tick[team] + king_hill_idle_timeout < tick) {
            if (obj->parent_object == (datum_index)0xffffffff && (obj->flags & _object_at_rest_bit) != 0) {
                obj->flags |= _object_changed_bit;
            }
            king_hill_occupant_last_tick[team] = tick;
        }
    }
}

#if 0
Original Ghidra decompilation (0x46c5d0), from tools/pack.py 0x46c5d0:

void FUN_0046c5d0(uint param_1,int param_2)

{
  char cVar1;
  int iVar2;

  cVar1 = item_get_effective_position();
  if (cVar1 != '\x01') {
    return;
  }
  if ((*(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + (param_1 & 0xffff) * 0xc) & 8) != 0) {
    return;
  }
  FUN_00462260(0,0xffffffff,0xffffffff);
  iVar2 = DAT_006f1d6c;
  if (0x4b0 < (uint)(*(int *)(DAT_006f1d6c + 0xc) - *(int *)(param_2 + 0x204))) {
    if (DAT_00719720 != 2) {
      return;
    }
    cVar1 = FUN_004c2ea0();
    if (((cVar1 == '\0') || ((*(uint *)(param_2 + 0x10) >> 0xb & 1) == 0)) ||
       (*(int *)(param_2 + 0x11c) != -1)) goto LAB_0046c6f7;
    if ((*(byte *)(param_2 + 0x22c) & 0x40) != 0) {
      iVar2 = data_iterator_next();
      while (iVar2 != 0) {
        chimera__kill_feed(0xffffffff,0x26,0xffffffff,1);
        iVar2 = data_iterator_next();
      }
    }
    FUN_0046c1a0();
  }
  iVar2 = DAT_006f1d6c;
  if (DAT_00719720 != 2) {
    return;
  }
LAB_0046c6f7:
  if ((DAT_006f1d14 < 1) || (2 < DAT_006f1d14)) {
    if (((&DAT_006b124c)[*(short *)(param_2 + 0xb8)] == -1) ||
       ((&DAT_006b124c)[*(short *)(param_2 + 0xb8)] + DAT_006883a0 < *(int *)(iVar2 + 0xc))) {
      if ((*(int *)(param_2 + 0x11c) == -1) && ((*(uint *)(param_2 + 0x10) & 0x20) != 0)) {
        *(uint *)(param_2 + 0x10) = *(uint *)(param_2 + 0x10) | 0x4000000;
      }
      (&DAT_006b124c)[*(short *)(param_2 + 0xb8)] = *(undefined4 *)(iVar2 + 0xc);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
