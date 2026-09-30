// player_new_network  (Ghidra: FUN_00473780; named per this rewrite)
// address 0x473780, size 438 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: types/game.h player field list is built from this function together with
//   player_new_local (0x473940, this batch): the field-by-field commentary explicitly calls
//   this one "the network constructor" at two offsets (unknown_f4 "the network constructor
//   writes -1 here", unknown_104 "network constructor writes -1"), which pins the name; the
//   other caller in the image, 0x4de870 (networking module), creates a player straight out of
//   a received-over-the-wire machine/local-player-index pair, which matches. The second caller,
//   0x4c8800 (main module), uses it for the single local player too, always with machine index
//   0 -- this constructor is shared by both paths, it just always keys the new player into
//   machine_to_player by machine index, where player_new_local does not.
//   out/phase4/game_functions.md's "reduced field-initialization set" (conf 0.3) undersells it;
//   it initializes as many fields as player_new_local, just a slightly different set (it does
//   not build the three embedded queue headers with player_update_queue_create /
//   position_update_queue_create / vehicle_update_queue_create -- it only stamps a handful of
//   their fields with sentinel values, exactly transcribed below).
//   objdump -d -M intel --start-address=0x473780 --stop-address=0x473940 bin/halo.exe confirms
//   every field offset and that the function DOES return the new datum_index in EAX (Ghidra's
//   "void" return is wrong: EAX is left holding the datum_new/datum_new_at_index result, or the
//   sentinel -1, on every return path, and both callers use the "return value").
// register convention: EAX -> requested_index (datum_new_at_index's exact index request, or -1
//   for "any free slot"); stack -> machine_index, local_player_index, identifier_record.
//   // blam-cc: EAX -> requested_index
// reconciled: R35 player.unknown_15c -> last_remote_update_id (comment only)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

extern data_array *player_data;                // 0x0087a480
extern wchar_t empty_string;                   // 0x00660c34, established fallback (see
                                                //   src/game/game_engine_build_end_game_result_text.c)
extern datum_index machine_to_player[16];      // 0x006b1460

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern datum_index datum_new_at_index(int16_t index, data_array *array); // 0x4d0430,
    // blam-cc: AX -> index, EDX -> array (matches src/memory/datum_new_at_index.c)

// CORRECTED (phase 4 review, against objdump): Ghidra shows this as void; it actually returns
// the new player's datum_index (or k_datum_index_none on failure) in EAX.
//
// Creates a player datum (at a specific index if requested_index is not -1, otherwise the next
// free one), gives it the reduced field set below (team defaults to 1, every handle field is
// seeded to the wildcard, speed to 1.0), copies the caller's identifier record's name into the
// player name (defaulting to the empty string) and, only on success, block-copies the whole
// 32-byte identifier record a second time into player+0x48 (identifier_name plus its
// unresolved tail through team_index_desired). Finally registers the result (even on failure,
// where that just re-writes the wildcard) as machine_index's entry in machine_to_player, unless
// that slot is already claimed.
datum_index player_new_network(datum_index requested_index, uint32_t machine_index,
                                int16_t local_player_index, uint16_t *identifier_record)
    // blam-cc: EAX -> requested_index, stack -> machine_index, local_player_index, identifier_record
{
    datum_index result;
    player *p;
    wchar_t *name_source;

    if (requested_index == (datum_index)-1) {
        result = datum_new(player_data);
    } else {
        result = datum_new_at_index((int16_t)requested_index, player_data);
    }

    if (result != (datum_index)-1) {
        p = (player *)((uint8_t *)player_data->data + (result & 0xffff) * sizeof(player));

        name_source = &empty_string;
        if (identifier_record != (uint16_t *)0) {
            name_source = (wchar_t *)identifier_record;
        }
        wcsncpy((wchar_t *)p->name, name_source, 11);
        p->name[11] = 0;

        p->local_player_index = local_player_index;
        p->ping = 0;
        p->medal_streak_count = 0;
        p->medal_streak_timer = 0;
        p->unit = (datum_index)-1;
        p->previous_unit = (datum_index)-1;
        p->unknown_1c = -1;
        p->bsp_cluster = -1;
        p->observer_target = (datum_index)-1;
        p->speed = 1.0f;
        p->team = 1;

        p->interaction_type = 0;
        p->interaction_object = (datum_index)-1;

        p->quit_tick = (datum_index)-1;
        p->marked_for_deletion = 0;
        p->odd_man_out = 0;
        p->last_update_id = 0;
        p->baseline_update_id = (datum_index)-1;
        p->unknown_f0 = 0;
        p->unknown_f4 = (datum_index)-1;
        p->unknown_104 = -1;
        p->connection_quality_started = 0;
        p->loss_window_start_ms = 0;
        p->loss_window_units = 0;
        p->latency_last_sample_ms = 0;
        p->latency_bad_sample_count = 0;
        p->unknown_11c = 0;

        p->update_history.queue.capacity = -1;
        p->update_history.queue.record_size = -1;
        p->update_history.queue.records = (void **)(intptr_t)-1;
        p->update_history.queue.write_index = -1;
        // Zeroes the rest of update_history (read_index, storage, has_current/pad, current[8])
        // plus last_remote_update_id (+0x15c, R35) right after it -- 12 dwords, exactly matching Ghidra's own loop.
        memset((uint8_t *)p + 0x130, 0, 0x30);

        p->last_position_update_id = 0;
        p->position_baseline_x = -1;
        p->position_baseline_y = -1;
        p->position_baseline_z = -1;

        p->position_updates.capacity = 0;
        p->position_updates.record_size = 0;
        p->position_updates.records = (void **)0;
        p->position_updates.write_index = -1;
        p->position_updates.read_index = -1;
        p->position_updates.storage = (void *)(intptr_t)-1;
        // Zeroes unknown_188, unknown_18c and the first 0x38 bytes of the unknown_190 tail (the
        // loop stops 8 bytes short of vehicle_updates at +0x1d0, exactly as Ghidra shows).
        memset((uint8_t *)p + 0x188, 0, 0x40);

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
Original Ghidra decompilation (0x473780), from tools/pack.py 0x473780:

void FUN_00473780(uint param_1,undefined2 param_2,wchar_t *param_3)

{
  int iVar1;
  int in_EAX;
  uint uVar2;
  wchar_t *_Source;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined8 uVar6;

  if (in_EAX == -1) {
    uVar6 = datum_new();
  }
  else {
    uVar6 = datum_new_at_index();
  }
  uVar2 = (uint)uVar6;
  if (uVar2 != 0xffffffff) {
    iVar4 = (uVar2 & 0xffff) * 0x200;
    iVar3 = *(int *)((int)((ulonglong)uVar6 >> 0x20) + 0x34) + iVar4;
    _Source = L"";
    if (param_3 != (wchar_t *)0x0) {
      _Source = param_3;
    }
    _wcsncpy((wchar_t *)(iVar3 + 4),_Source,0xb);
    iVar1 = DAT_0087a480;
    *(undefined2 *)(iVar3 + 2) = param_2;
    *(undefined2 *)(iVar3 + 0x1a) = 0;
    *(undefined4 *)(iVar3 + 0xdc) = 0;
    *(undefined4 *)(iVar3 + 0xe0) = 0;
    *(undefined4 *)(iVar3 + 0xe4) = 0;
    *(undefined4 *)(iVar3 + 0x34) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x38) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x1c) = 0xffffffff;
    *(undefined2 *)(iVar3 + 0x3c) = 0xffff;
    *(undefined4 *)(iVar3 + 0x40) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x6c) = 0x3f800000;
    *(undefined4 *)(iVar3 + 0x20) = 1;
    iVar1 = *(int *)(iVar1 + 0x34);
    *(undefined2 *)(iVar1 + 0x28 + iVar4) = 0;
    *(undefined4 *)(iVar1 + 0x24 + iVar4) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0xd0) = 0xffffffff;
    *(undefined1 *)(iVar3 + 0xd5) = 0;
    *(undefined1 *)(iVar3 + 0x8c) = 0;
    *(undefined4 *)(iVar3 + 0xe8) = 0;
    *(undefined4 *)(iVar3 + 0xec) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0xf0) = 0;
    *(undefined4 *)(iVar3 + 0xf4) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x104) = 0xffffffff;
    *(undefined1 *)(iVar3 + 0x108) = 0;
    *(undefined4 *)(iVar3 + 0x10c) = 0;
    *(undefined4 *)(iVar3 + 0x110) = 0;
    *(undefined4 *)(iVar3 + 0x114) = 0;
    *(undefined4 *)(iVar3 + 0x118) = 0;
    *(undefined4 *)(iVar3 + 0x11c) = 0;
    *(undefined4 *)(iVar3 + 0x120) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x124) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x128) = 0xffffffff;
    *(undefined4 *)(iVar3 + 300) = 0xffffffff;
    puVar5 = (undefined4 *)(iVar3 + 0x130);
    for (iVar4 = 0xc; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    *(undefined4 *)(iVar3 + 0x160) = 0;
    *(undefined4 *)(iVar3 + 0x164) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x168) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x16c) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x170) = 0;
    *(undefined4 *)(iVar3 + 0x174) = 0;
    *(undefined4 *)(iVar3 + 0x178) = 0;
    *(undefined4 *)(iVar3 + 0x17c) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x180) = 0xffffffff;
    *(undefined4 *)(iVar3 + 0x184) = 0xffffffff;
    puVar5 = (undefined4 *)(iVar3 + 0x188);
    for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    if (param_3 != (wchar_t *)0x0) {
      puVar5 = (undefined4 *)(iVar3 + 0x48);
      for (iVar4 = 8; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar5 = *(undefined4 *)param_3;
        param_3 = param_3 + 2;
        puVar5 = puVar5 + 1;
      }
    }
  }
  iVar3 = 0;
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
