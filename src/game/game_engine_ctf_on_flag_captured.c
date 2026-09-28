// game_engine_ctf_on_flag_captured  (Ghidra: game_engine_ctf_on_flag_captured, already named)
// address 0x46dde0, size 324 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/game.h player::unknown_88 ("part of the profile block"), unknown_c8 ("flag
//   touches; also mirrored by the profile" -- reused here, on closer reading, as the player's
//   fastest recorded capture time; kept the header's existing name rather than renaming across
//   files); player::unknown_c6 already used identically (an UNSURE short counter) in
//   game_engine_ctf_player_flag_tick.c and game_engine_check_bucket_scores_and_end_round.c, this
//   batch; player+0xc4 written here as a 16-bit elapsed-capture-time value (like objective_time's
//   own narrow-write precedent elsewhere in this batch); game_engine_variant::ctf_option_7c
//   aliased 0x006f1d04; game_engine_check_bucket_scores_and_end_round (0x46db70, this batch);
//   ctf_globals::unknown_44 (0x006b12d4) used here as a per-team 32-bit captured-flags mask,
//   corroborated by FUN_0046df30 (this batch)'s bitmask arithmetic on the same global.
// register convention: flag index is the function's own stack parameter (__cdecl).
// UNSURE: player + 0xc6's real field name.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#include "objects.h"
#include "units.h"

extern data_array *player_data;      // 0x0087a480
extern game_time_globals *game_time; // 0x006f1d6c
extern uint32_t ctf_team_captured_flags_mask[]; // 0x006b12d4, inside ctf_globals::unknown_44
    // (0x006b1290 + 0x44); CORRECTED from an earlier byte-array guess after FUN_0046df30 (this
    // batch) showed it participating in 32-bit bitmask arithmetic indexed by team
extern game_variant game_engine_variant;   // 0x006f1c88 (ctf_option_7c aliased 0x006f1d04)

extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern void game_engine_check_bucket_scores_and_end_round(void);      // 0x46db70, this batch
extern void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player,
    int32_t no_source_message, int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast); // 0x460c10, blam-cc: BL broadcast
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30

void game_engine_ctf_on_flag_captured(uint32_t flag_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (flag_index & 0xffff) * sizeof(player));
    int16_t elapsed = (int16_t)(game_time->game_time - p->unknown_88);
    uint8_t new_record = 0;

    ctf_team_captured_flags_mask[flag_index & 0xffff] = 0;
    game_engine_queue_multiplayer_sound(0x2a, flag_index, 1); // 0x46de12..0x46de26, EDI still the argument

    *(int16_t *)&((struct player *)p)->objective_time = elapsed;
    if (*(int16_t *)((uint8_t *)p + 0xc6) != 0) {
        if (elapsed <= *(int16_t *)((uint8_t *)p + 200)) {
            new_record = 1;
            *(int16_t *)((uint8_t *)p + 200) = elapsed;
        }
    } else {
        *(int16_t *)((uint8_t *)p + 200) = elapsed;
    }

    *(int16_t *)((uint8_t *)p + 0xc6) += 1;
    p->unknown_88 = game_time->game_time;

    game_engine_check_bucket_scores_and_end_round();

    if (game_engine_variant.ctf_option_7c == 2) {
        game_engine_broadcast_kill_feed_by_relationship(flag_index, 0x23, 0x24, 0x22, flag_index, 1); // BL = 1 at 0x46de85
    } else {
        game_engine_broadcast_kill_feed_by_relationship(flag_index, 0x20, 0x21, 0x22, flag_index, 1);
    }

    if (game_engine_variant.ctf_option_7c != 2 && new_record != 0) {
        data_iterator iter;
        void *element;
        iter.data = player_data; // UNSURE: iterator source not directly shown
        iter.next_index = 0;
        iter.index = (datum_index)0xffffffff;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        element = data_iterator_next(&iter);
        while (element != 0) {
            uint32_t recipient = (flag_index == 0xffffffff) ? (uint32_t)iter.index : flag_index;
            chimera__kill_feed((datum_index)recipient, 0x26, flag_index, 1, 0);
            element = data_iterator_next(&iter);
        }
    }
}

#if 0
Original Ghidra decompilation (0x46dde0), from tools/pack.py 0x46dde0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl game_engine_ctf_on_flag_captured(uint flag_index)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  undefined2 uVar4;
  uint uVar5;
  uint local_8;

  iVar2 = (flag_index & 0xffff) * 0x200;
  iVar3 = iVar2 + *(int *)(DAT_0087a480 + 0x34);
  iVar2 = *(int *)(DAT_006f1d6c + 0xc) - *(int *)(iVar2 + 0x88 + *(int *)(DAT_0087a480 + 0x34));
  (&DAT_006b12d4)[flag_index & 0xffff] = 0;
  game_engine_queue_multiplayer_sound(1);
  uVar4 = (undefined2)iVar2;
  *(undefined2 *)(iVar3 + 0xc4) = uVar4;
  bVar1 = false;
  if (*(short *)(iVar3 + 0xc6) != 0) {
    if (*(short *)(iVar3 + 200) <= iVar2) goto LAB_0046de5d;
    bVar1 = true;
  }
  *(undefined2 *)(iVar3 + 200) = uVar4;
LAB_0046de5d:
  iVar2 = DAT_006f1d6c;
  *(short *)(iVar3 + 0xc6) = *(short *)(iVar3 + 0xc6) + 1;
  *(undefined4 *)(iVar3 + 0x88) = *(undefined4 *)(iVar2 + 0xc);
  FUN_0046db70();
  if (_DAT_006f1d04 == 2) {
    FUN_00460c10(flag_index,0x23,0x24,0x22,flag_index);
  }
  else {
    FUN_00460c10(flag_index,0x20,0x21,0x22,flag_index);
  }
  if ((_DAT_006f1d04 != 2) && (bVar1)) {
    local_8 = 0xffffffff;
    iVar2 = data_iterator_next();
    while (iVar2 != 0) {
      uVar5 = flag_index;
      if (flag_index == 0xffffffff) {
        uVar5 = local_8;
      }
      chimera__kill_feed(uVar5,0x26,flag_index,1);
      iVar2 = data_iterator_next();
    }
  }
  return;
}
#endif
