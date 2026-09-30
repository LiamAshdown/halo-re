// game_engine_capture_player_profile  (Ghidra: FUN_00466ee0; named per
// out/phase4/game_functions.md: "Captures a player's current profile-like fields from the live
// player record, forwards them for network sync, and optionally refreshes the cache table
// entry.")
// address 0x466ee0, size 291 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x466ee0
//   --stop-address=0x467010): `slot` arrives in EAX (Ghidra's undocumented in_EAX), indexing
//   player_profile_cache[slot].player directly as a player-array index (not through datum_get)
//   -- the same field-by-field layout as game_engine_apply_player_profile_entry.c, mirrored:
//   player.kills(0x9c)->tail.kills, unknown_a0(0xa0)->unknown_0c, assists(0xa4)->assists,
//   unknown_a8(0xa8)->unknown_14, betrayals/deaths/suicides(0xac/0xae/0xb0), objective_time
//   (0xc4, copied as a 4+2 byte pair together with unknown_c8/0xc8, exactly like the apply
//   direction and for the same struct-packing reason), unknown_88->unknown_24, odd_man_out,
//   speed. The king-only rescale is a 16-bit signed divide-by-30 written out by the compiler as
//   its magic-multiply reciprocal; this rewrite uses plain `/ 30`, which recompiles to the same
//   instructions and is exactly what the disassembly (movsx+imul 0x88888889+shifts) computes.
//   hash_table_get (0x4f05e0) and the 0x00687558+0xc table are foreign/UNSURE, matching the
//   precedent in game_engine_notify_kill_event.c and game_engine_touch_multiplayer_predicted_
//   resources.c respectively.
// register convention: slot index in EAX (in_EAX); `commit` is this function's own recognized
//   stack parameter.
//   // blam-cc: EAX -> slot, stack -> commit
// UNSURE: the 0x00687558+0xc table's identity/owning module; hash_table_get's real signature;
//   see game_engine_send_player_profile_update.c for the EAX/ECX ambiguity at that call.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h" // hash_table

extern player_profile player_profile_cache[16]; // 0x006b0b88
extern data_array *player_data;                 // 0x0087a480
extern game_variant game_engine_variant;        // 0x006f1c88

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern uint8_t *machine_table; // 0x00687558, its hash_table sits at +0x0c
extern void game_engine_send_player_profile_update(void *has_payload, void *profile_tail,
                                                     int32_t target); // 0x467010, this batch

// blam-cc: EAX -> slot, stack -> commit
// Captures player_profile_cache[slot].player's current stat fields into a local snapshot,
// applies the king-only objective_time rescale (ticks -> seconds), sends the snapshot over the
// network (broadcast), and, when `commit` is 1, writes the snapshot back into the cache slot.
void game_engine_capture_player_profile(int32_t slot, int32_t commit)
{
    datum_index player_handle;
    player *p;
    player_profile snapshot;
    int32_t lookup_result;

    player_handle = player_profile_cache[slot].player;
    p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));

    lookup_result = 0;
    if (player_handle != (datum_index)0xffffffff) {
        lookup_result = hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)player_handle); // 0x466ef4..0x466f1c
        if (lookup_result == -1) {
            lookup_result = 0;
        }
    }

    snapshot.kills = p->kills;
    snapshot.unknown_0a = p->unknown_9e;
    snapshot.unknown_0c = p->unknown_a0;
    snapshot.assists = p->assists;
    snapshot.unknown_12 = p->unknown_a6;
    snapshot.unknown_14 = p->unknown_a8;
    snapshot.betrayals = p->betrayals;
    snapshot.deaths = p->deaths;
    snapshot.suicides = p->suicides;
    snapshot.objective_time = p->objective_time;
    snapshot.objective_score = p->objective_score;
    snapshot.slayer_target = p->slayer_target;
    snapshot.odd_man_out = p->odd_man_out;
    snapshot.speed = p->speed;

    if (game_engine_variant.game_engine_index == _game_engine_king) {
        int16_t *low_word = (int16_t *)&snapshot.objective_time;
        *low_word = *low_word / 30;
    }

    game_engine_send_player_profile_update(&lookup_result, &snapshot.kills, -1);

    if (commit == 1) {
        player_profile_cache[slot].kills = snapshot.kills;
        player_profile_cache[slot].unknown_0a = snapshot.unknown_0a;
        player_profile_cache[slot].unknown_0c = snapshot.unknown_0c;
        player_profile_cache[slot].assists = snapshot.assists;
        player_profile_cache[slot].unknown_12 = snapshot.unknown_12;
        player_profile_cache[slot].unknown_14 = snapshot.unknown_14;
        player_profile_cache[slot].betrayals = snapshot.betrayals;
        player_profile_cache[slot].deaths = snapshot.deaths;
        player_profile_cache[slot].suicides = snapshot.suicides;
        player_profile_cache[slot].objective_time = snapshot.objective_time;
        player_profile_cache[slot].objective_score = snapshot.objective_score;
        player_profile_cache[slot].slayer_target = snapshot.slayer_target;
        player_profile_cache[slot].odd_man_out = snapshot.odd_man_out;
        player_profile_cache[slot].speed = snapshot.speed;
    }
}

#if 0
Original Ghidra decompilation (0x466ee0), from tools/pack.py 0x466ee0:

void FUN_00466ee0(int param_1)

{
  uint uVar1;
  int in_EAX;
  undefined4 *puVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 local_28 [4];
  undefined2 local_18;
  undefined2 local_16;
  undefined2 local_14;
  undefined4 local_12;
  undefined2 local_e;
  undefined4 local_c;
  undefined1 local_8;
  undefined4 local_4;

  iVar3 = ((&DAT_006b0b8c)[in_EAX * 0xc] & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  if ((&DAT_006b0b8c)[in_EAX * 0xc] != 0xffffffff) {
    hash_table_get();
  }
  local_28[0] = *(undefined4 *)(iVar3 + 0x9c);
  local_28[1] = *(undefined4 *)(iVar3 + 0xa0);
  local_28[2] = *(undefined4 *)(iVar3 + 0xa4);
  local_28[3] = *(undefined4 *)(iVar3 + 0xa8);
  local_18 = *(undefined2 *)(iVar3 + 0xac);
  local_16 = *(undefined2 *)(iVar3 + 0xae);
  local_14 = *(undefined2 *)(iVar3 + 0xb0);
  local_12 = *(undefined4 *)(iVar3 + 0xc4);
  local_e = *(undefined2 *)(iVar3 + 200);
  local_c = *(undefined4 *)(iVar3 + 0x88);
  local_8 = *(undefined1 *)(iVar3 + 0x8c);
  local_4 = *(undefined4 *)(iVar3 + 0x6c);
  if (DAT_006f1cb8 == 4) {
    uVar1 = (uint)local_12 >> 0x10;
    local_12 = CONCAT22((short)uVar1,
                        ((short)local_12 / 0x1e + ((short)local_12 >> 0xf)) -
                        (short)((longlong)(int)(short)local_12 * 0x88888889 >> 0x3f));
  }
  FUN_00467010(local_28,0xffffffff);
  if (param_1 == 1) {
    puVar2 = local_28;
    puVar4 = (undefined4 *)(&DAT_006b0b90 + in_EAX * 0x30);
    for (iVar3 = 10; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar4 = puVar4 + 1;
    }
  }
  return;
}
#endif
