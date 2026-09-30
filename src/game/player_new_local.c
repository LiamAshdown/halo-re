// player_new_local  (Ghidra: FUN_00473940; named per this rewrite)
// address 0x473940, size 412 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: types/game.h calls this "0x473940 (locally created player)" throughout its player
//   field commentary (as opposed to player_new_network, 0x473780, "the network constructor");
//   this function's one caller in the image, 0x4de8c0 (networking module), is the direct
//   counterpart of player_new_network's caller 0x4de870, differing only in which of the two
//   constructors it calls with the same machine/local-player-index pair.
//   out/phase4/game_functions.md's "full state initialization" (conf 0.4) is right about the
//   shape (this is the one that builds the three embedded network-update queues) but not the
//   trigger: the queue-building block is gated on local_player_index == -1, i.e. it runs for
//   every player that is NOT this machine's local player -- a remotely-represented player is
//   exactly the case that needs an incoming position/vehicle update history to reconcile
//   against, while the player actually being driven by this machine's own input does not.
//   objdump -d -M intel --start-address=0x473940 --stop-address=0x473ae0 bin/halo.exe confirms
//   every field offset, the datum_new_at_index_with_salt callee (distinguishing this from
//   player_new_network's plain datum_new_at_index) and that EAX carries the return value on
//   every path despite Ghidra's "void" signature, exactly as in player_new_network.
// register convention: EAX -> requested_handle (index and salt, for datum_new_at_index_with_salt;
//   -1 for "any free slot" via plain datum_new); stack -> machine_index, local_player_index,
//   identifier_record.
//   // blam-cc: EAX -> requested_handle
// reconciled: R35 player.unknown_15c (datum_index) -> int32 last_remote_update_id (-1 = none)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <wchar.h>
#include <string.h>

extern data_array *player_data;                // 0x0087a480
extern wchar_t empty_string;                   // 0x00660c34
extern datum_index machine_to_player[16];      // 0x006b1460

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array);
    // 0x4d03d0, blam-cc: EAX -> requested_handle, EDX -> array
extern void player_update_queue_create(player_update_queue *queue); // 0x479f40, this batch's
    // neighbor, blam-cc: ESI -> queue (matches src/game/update_server_dispose.c)


// CORRECTED (phase 4 review, against objdump): Ghidra shows this as void; EAX carries the new
// player's datum_index (or k_datum_index_none) on every return path, matching both callers.
//
// Creates a player datum (at a specific index+salt if requested_handle is not -1, otherwise the
// next free one via datum_new) and initializes the same reduced field set player_new_network
// does, plus -- only when local_player_index is -1, i.e. this player will be driven by updates
// from elsewhere rather than this machine's own input -- builds the three embedded network
// update queues (player_update_queue_create at +0x120, position_update_queue_create at +0x170,
// vehicle_update_queue_create at +0x1d0) and zeroes every field between and around them. Copies
// the caller's identifier record's name (defaulting to the empty string) and, on success, the
// whole 32-byte identifier record a second time into +0x48. Finally registers the result (even
// on failure) as machine_index's entry in machine_to_player, unless that slot is already
// claimed.
datum_index player_new_local(datum_index requested_handle, uint32_t machine_index,
                              int16_t local_player_index, uint16_t *identifier_record)
    // blam-cc: EAX -> requested_handle, stack -> machine_index, local_player_index, identifier_record
{
    datum_index result;
    player *p;
    wchar_t *name_source;

    if (requested_handle == (datum_index)-1) {
        result = datum_new(player_data);
    } else {
        result = datum_new_at_index_with_salt(requested_handle, player_data);
    }

    if (result != (datum_index)-1) {
        p = (player *)((uint8_t *)player_data->data + (result & 0xffff) * sizeof(player));

        name_source = &empty_string;
        if (identifier_record != (uint16_t *)0) {
            name_source = (wchar_t *)identifier_record;
        }
        wcsncpy((wchar_t *)p->name, name_source, 11);
        p->name[11] = 0;

        p->ping_ms = 0;
        p->medal_streak_count = 0;
        p->medal_streak_timer = 0;
        p->local_player_index = local_player_index;
        p->unit = (datum_index)-1;
        p->previous_unit = (datum_index)-1;
        p->unknown_1c = -1;
        p->bsp_cluster = -1;
        p->observer_target = (datum_index)-1;
        p->speed = 1.0f;
        p->team = 1;

        p->interaction_type = 0;
        p->interaction_object = (datum_index)-1;

        p->removal_tick = (datum_index)-1;
        p->marked_for_deletion = 0;
        p->unknown_ec = (datum_index)-1;
        p->unknown_e8 = -1; // UNSURE: unknown_e8 is int32_t; player_new_network sets it to 0

        if (local_player_index == -1) {
            p->last_remote_update_id = -1;
            p->unknown_160 = (datum_index)-1;
            player_update_queue_create(&p->update_history);
            // The 0xc-dword run types/game.h already documents as "the local constructor
            // zeroes": unknown_f0, unknown_f4, unknown_f8, unknown_fc[8], unknown_104,
            // unknown_108, pad_109[3], unknown_10c, unknown_110, unknown_114, unknown_118,
            // unknown_11c.
            memset((uint8_t *)p + 0xf0, 0, 0x30);

            p->unknown_164 = 0;
            p->unknown_168 = 0;
            p->unknown_16c = 0;
            position_update_queue_create(&p->position_updates);

            p->unknown_188 = 0;
            p->unknown_18c = (datum_index)-1;
            // Zeroes the whole unknown_190 tail (0x40 bytes, exactly its declared size).
            memset((uint8_t *)p + 0x190, 0, 0x40);
            vehicle_update_queue_create(&p->vehicle_updates);

            p->unknown_1e8 = 0;
            p->unknown_1ec = 0;
            p->unknown_1f0 = 0;
            p->unknown_1f4 = 0;
            p->unknown_1f8 = 0;
        }

        if (identifier_record != (uint16_t *)0) {
            memcpy(&p->identifier_name, identifier_record, 32);
        }
    }

    if (machine_to_player[machine_index & 0xffff] == (datum_index)-1) {
        machine_to_player[machine_index & 0xffff] = result;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x473940), from tools/pack.py 0x473940:

void FUN_00473940(uint param_1,short param_2,wchar_t *param_3)

{
  int iVar1;
  int in_EAX;
  uint uVar2;
  wchar_t *_Source;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined8 uVar7;

  if (in_EAX == -1) {
    uVar7 = datum_new();
  }
  else {
    uVar7 = datum_new_at_index_with_salt();
  }
  uVar2 = (uint)uVar7;
  iVar3 = 0;
  if (uVar2 != 0xffffffff) {
    iVar5 = (uVar2 & 0xffff) * 0x200;
    iVar4 = *(int *)((int)((ulonglong)uVar7 >> 0x20) + 0x34) + iVar5;
    _Source = L"";
    if (param_3 != (wchar_t *)0x0) {
      _Source = param_3;
    }
    _wcsncpy((wchar_t *)(iVar4 + 4),_Source,0xb);
    iVar1 = DAT_0087a480;
    *(undefined2 *)(iVar4 + 0x1a) = 0;
    *(undefined4 *)(iVar4 + 0xdc) = 0;
    *(undefined4 *)(iVar4 + 0xe0) = 0;
    *(undefined4 *)(iVar4 + 0xe4) = 0;
    *(short *)(iVar4 + 2) = param_2;
    *(undefined4 *)(iVar4 + 0x34) = 0xffffffff;
    *(undefined4 *)(iVar4 + 0x38) = 0xffffffff;
    *(undefined4 *)(iVar4 + 0x1c) = 0xffffffff;
    *(undefined2 *)(iVar4 + 0x3c) = 0xffff;
    *(undefined4 *)(iVar4 + 0x40) = 0xffffffff;
    *(undefined4 *)(iVar4 + 0x6c) = 0x3f800000;
    *(undefined4 *)(iVar4 + 0x20) = 1;
    iVar5 = *(int *)(iVar1 + 0x34) + iVar5;
    *(undefined2 *)(iVar5 + 0x28) = 0;
    *(undefined4 *)(iVar5 + 0x24) = 0xffffffff;
    *(undefined4 *)(iVar4 + 0xd0) = 0xffffffff;
    *(undefined1 *)(iVar4 + 0xd5) = 0;
    *(undefined4 *)(iVar4 + 0xec) = 0xffffffff;
    *(undefined4 *)(iVar4 + 0xe8) = 0xffffffff;
    if (param_2 == -1) {
      *(undefined4 *)(iVar4 + 0x15c) = 0xffffffff;
      *(undefined4 *)(iVar4 + 0x160) = 0xffffffff;
      FUN_00479f40();
      puVar6 = (undefined4 *)(iVar4 + 0xf0);
      for (iVar5 = 0xc; iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
      *(undefined4 *)(iVar4 + 0x164) = 0;
      *(undefined4 *)(iVar4 + 0x168) = 0;
      *(undefined4 *)(iVar4 + 0x16c) = 0;
      position_update_queue_create();
      *(undefined4 *)(iVar4 + 0x188) = 0;
      *(undefined4 *)(iVar4 + 0x18c) = 0xffffffff;
      puVar6 = (undefined4 *)(iVar4 + 400);
      for (iVar5 = 0x10; iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
      vehicle_update_queue_create();
      *(undefined4 *)(iVar4 + 0x1e8) = 0;
      *(undefined4 *)(iVar4 + 0x1ec) = 0;
      *(undefined4 *)(iVar4 + 0x1f0) = 0;
      *(undefined4 *)(iVar4 + 500) = 0;
      *(undefined4 *)(iVar4 + 0x1f8) = 0;
    }
    if (param_3 != (wchar_t *)0x0) {
      puVar6 = (undefined4 *)(iVar4 + 0x48);
      for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
        *puVar6 = *(undefined4 *)param_3;
        param_3 = param_3 + 2;
        puVar6 = puVar6 + 1;
      }
    }
  }
  do {
    if ((&DAT_006b1460)[(param_1 & 0xffff) + iVar3] == -1) {
      (&DAT_006b1460)[(param_1 & 0xffff) + iVar3] = uVar2;
      return;
    }
    iVar3 = iVar3 + 1;
  } while (iVar3 < 1);
  return;
}
#endif
