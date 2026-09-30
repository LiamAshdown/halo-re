// game_engine_on_player_death  (Ghidra: game_engine_on_player_death, already named)
// address 0x460200, size 1672 bytes
// name confidence: 0.7   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Handles a player death event: schedules the victim's
// respawn timer and builds/broadcasts the appropriate kill-feed message"); types/game.h player
// (respawn_timer +0x2c, respawn_time_growth +0x30, last_death_tick +0x84, killing_spree_count
// +0x96, multikill_count +0x98, betrayal_penalty_count +0xc0, marked_for_deletion +0xd5,
// local_player_index +0x02), game_variant (respawn_time_growth +0x44, respawn_time +0x48,
// suicide_penalty +0x4c, betrayal_penalty +0x70), game_engine_definition (unknown_68, get_score
// slot names don't apply here -- the vtable slot fired first is +0x68, "unknown_68", per
// types/game.h), types/objects.h object (type +0x0b4).
// register convention: killer player handle in EDX (in_EDX); param_1 (a death-causing object
// handle, used only when there is no clear killer), param_2 (victim player handle) and param_3
// (a suicide-style flag byte) are this function's own stack parameters, kept in their original
// order after the register argument.
//   // blam-cc: EDX -> killer, stack -> death_object, victim, is_suicide
// VERIFIED against disassembly 0x460200..0x460887 (2026-09-30). Fixed: the kill-feed broadcasts pass their register
//   and stack args (0x460db0: ESI category, BL 1, stack exclude/alternate/subject; 0x460d10: EAX killer, ESI spree
//   kind, BL 1, stack killer/victim; 0x4608d0: EAX and ECX = the target handle), the kill-feed iterator branches
//   message every player instead of draining it, and the machine notify is outside the local-player test.
//   0x45e680 also takes the target handle in EAX; its prototype and all callers were extended to pass it.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include <wchar.h>
#include <stdint.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_time_globals *game_time;                // 0x006f1d6c
extern data_array *player_data;                     // 0x0087a480
extern data_array *object_data;                  // 0x008603b0
extern int16_t network_game_mode;                   // 0x00719720
extern game_variant game_engine_variant;             // 0x006f1c88

extern void game_engine_player_profile_cache_sync_all(datum_index player_handle); // 0x466cb0, not in this batch
// blam-cc: EAX -> recipient_or_all, ESI -> broadcast_enabled, EBX -> broadcast, stack -> hash_key, subject
extern void game_engine_broadcast_kill_feed_or_direct(datum_index recipient_or_all, int32_t broadcast_enabled,
    char broadcast, int32_t hash_key, datum_index subject); // 0x460d10
// blam-cc: ESI -> broadcast_enabled, EBX -> broadcast, stack -> exclude_index, alternate_recipient, subject
extern void game_engine_broadcast_kill_feed_gated(int32_t broadcast_enabled, int32_t exclude_index,
    int32_t alternate_recipient, datum_index subject, char broadcast); // 0x460db0
// blam-cc: EAX -> player_index, ECX -> hash_key, stack -> message_type, subject
extern void game_engine_notify_kill_event(uint32_t player_index, int32_t hash_key, int32_t message_type,
    datum_index subject); // 0x4608d0
// blam-cc: EAX -> recipient, EBX -> out, stack -> message_type, subject, buffer_size
extern uint8_t game_engine_build_kill_feed_message_text(datum_index recipient, wchar_t *out, uint32_t message_type,
    datum_index subject, size_t buffer_size); // 0x45e680, this batch
extern void chimera__multiplayer_message(wchar_t *text); // 0x4ab4b0, UNSURE: char* vs wchar_t*
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// 0x46042e..0x4604f8 (type 0x0d) and 0x4606a4..0x460769 (type 8), and the same body inside the two player-iterator
// loops at 0x4604f9 / 0x460770 (reached only when `target` is -1, which the category tests above make impossible):
// validates the target handle, builds the localized text for a locally driven player and, in a network game,
// always tells the target's machine.
static void game_engine_on_player_death_send_message(datum_index target, uint32_t message_type, datum_index victim,
    wchar_t *buffer)
{
    int16_t index = (int16_t)target;
    int16_t salt = (int16_t)((uint32_t)target >> 16);
    player *t;

    if (index < 0 || index >= player_data->maximum_count) {
        return;
    }
    t = (player *)((uint8_t *)player_data->data + player_data->size * index);
    if (t->identifier == 0 || (salt != 0 && t->identifier != salt)) {
        return;
    }
    if (t->local_player_index != -1) {
        if ((current_game_engine->build_message_text != 0 &&
             ((char (*)(datum_index, uint32_t, datum_index, wchar_t *, uint32_t))current_game_engine->build_message_text)(
                 target, message_type, victim, buffer, 0x400) != 0) ||
            game_engine_build_kill_feed_message_text(target, buffer, message_type, victim, 0x400) != 0) {
            buffer[0x3ff] = 0;
            chimera__multiplayer_message(buffer);
        }
    }
    if (network_game_mode == 2) {
        game_engine_notify_kill_event(target, target, message_type, victim);
    }
}

// 0x4604f9 / 0x460770: with a valid `killer` only that player is messaged; -1 walks every player.
static void game_engine_on_player_death_message_players(datum_index killer, uint32_t message_type, datum_index victim,
    wchar_t *buffer)
{
    if (killer != (datum_index)0xffffffff) {
        game_engine_on_player_death_send_message(killer, message_type, victim, buffer);
    } else {
        data_iterator iter;
        void *element;

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)0xffffffff;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
        element = data_iterator_next(&iter);
        while (element != 0) {
            game_engine_on_player_death_send_message(iter.index, message_type, victim, buffer);
            element = data_iterator_next(&iter);
        }
    }
}

// blam-cc: EDX -> killer, stack -> death_object, victim, is_suicide
// Schedules `victim`'s respawn timer (base + accumulated growth, plus suicide/betrayal
// penalties, clamped to [0x5a, 9000] ticks), refunds the killer's own accumulated growth on a
// clean kill, then, unless the victim is already marked for deletion, classifies the death into
// one of five kill-feed message categories and broadcasts the resulting text.
void game_engine_on_player_death(datum_index killer, datum_index death_object, datum_index victim,
    char is_suicide)
{
    player *v;
    uint8_t message_category;
    wchar_t kill_feed_buffer[1024]; // matches Ghidra's `acStack_800[2046]` + the uStack_2 terminator (byte-sized, but holds wide chars)

    if (current_game_engine == 0) {
        return;
    }

    v = (player *)((uint8_t *)player_data->data + (victim & 0xffff) * sizeof(player));
    v->last_death_tick = game_time->game_time;

    if (current_game_engine->on_player_death != 0) {
        // FIXED 2026-09-28: 0x460247..0x46025b pushes is_suicide, victim, death_object, killer (the slayer
        //   handler 0x46f580 reads all four); the call passed none.
        ((void (*)(datum_index, datum_index, datum_index, char))current_game_engine->on_player_death)(killer, death_object,
            victim, is_suicide);
    }

    {
        uint8_t clean_kill = killer != (datum_index)0xffffffff && victim != (datum_index)0xffffffff;
        clean_kill = (is_suicide == 0 && clean_kill && killer != victim);

        v->respawn_timer = v->respawn_time_growth + game_engine_variant.respawn_time;

        if (0 < game_engine_variant.respawn_time_growth) {
            int32_t grown = v->respawn_time_growth + game_engine_variant.respawn_time_growth;
            int32_t cap = game_engine_variant.respawn_time_growth * 5;
            v->respawn_time_growth = (grown <= cap) ? grown : cap;

            if (clean_kill && killer != (datum_index)0xffffffff) {
                player *k = (player *)((uint8_t *)player_data->data + (killer & 0xffff) * sizeof(player));
                int32_t refunded = k->respawn_time_growth - game_engine_variant.respawn_time_growth;
                k->respawn_time_growth = (refunded < 1) ? 0 : refunded;
            }
        }
    }

    if (killer == victim) {
        v->respawn_timer = v->respawn_timer + game_engine_variant.suicide_penalty;
    }
    if (v->betrayal_penalty_count != 0) {
        v->respawn_timer = v->respawn_timer + v->betrayal_penalty_count * game_engine_variant.betrayal_penalty;
        v->betrayal_penalty_count = 0;
    }
    if (v->respawn_timer < 0x5a) {
        v->respawn_timer = 0x5a;
    }
    if (9000 < v->respawn_timer) {
        v->respawn_timer = 9000;
    }

    if (network_game_mode == 2) {
        game_engine_player_profile_cache_sync_all((datum_index)0xffffffff);
    }

    if (v->marked_for_deletion != 0) {
        return;
    }

    if (killer != (datum_index)0xffffffff) {
        if (killer == victim) {
            // 0x4603dc: esi = 6, bl = 1, push killer, push victim, push -1
            game_engine_broadcast_kill_feed_gated(6, -1, victim, killer, 1);
            return;
        }
        message_category = (is_suicide != 0) + 4; // 4 (killed) or 5 (killed, alternate wording)
    } else if (death_object != (datum_index)0xffffffff) {
        object *obj = ((object_header *)object_data->data)[death_object & 0xffff].data;
        if (obj->type == 0) {
            message_category = 2;
        } else if (obj->type == 1) {
            message_category = 3;
        } else {
            message_category = 1;
        }
    } else {
        message_category = 1;
    }
    // 0x460406: esi = category, bl = 1, push killer, push victim, push killer
    game_engine_broadcast_kill_feed_gated(message_category, killer, victim, killer, 1);

    if (message_category == 5) {
        game_engine_on_player_death_message_players(killer, 0x0d, victim, kill_feed_buffer);
    } else if (message_category == 4) {
        player *k = (player *)((uint8_t *)player_data->data + (killer & 0xffff) * sizeof(player));
        int32_t spree_type = 0;
        int32_t send_spree = 1;

        // 0x46062d..0x460690: the medal/spree kind goes out in ESI
        if (k->multikill_count >= 4) {
            spree_type = 0x0a;
        } else if (k->multikill_count == 3) {
            spree_type = 9;
        } else if (k->multikill_count == 2) {
            spree_type = 7;
        } else if (k->killing_spree_count == 5) {
            spree_type = 0x0b;
        } else if (k->killing_spree_count % 5 == 0) {
            spree_type = 0x0c;
        } else {
            send_spree = 0;
        }
        if (send_spree) {
            game_engine_broadcast_kill_feed_or_direct(killer, spree_type, 1, killer, victim);
        }

        game_engine_on_player_death_message_players(killer, 8, victim, kill_feed_buffer);
    }
}

#if 0
Original Ghidra decompilation (0x460200), from tools/pack.py 0x460200:

/* WARNING: Removing unreachable block (ram,0x0046053d) */
/* WARNING: Removing unreachable block (ram,0x0046054e) */
/* WARNING: Removing unreachable block (ram,0x00460558) */
/* WARNING: Removing unreachable block (ram,0x00460571) */
/* WARNING: Removing unreachable block (ram,0x00460576) */
/* WARNING: Removing unreachable block (ram,0x0046057f) */
/* WARNING: Removing unreachable block (ram,0x00460586) */
/* WARNING: Removing unreachable block (ram,0x00460592) */
/* WARNING: Removing unreachable block (ram,0x004605a9) */
/* WARNING: Removing unreachable block (ram,0x004605c3) */
/* WARNING: Removing unreachable block (ram,0x004605da) */
/* WARNING: Removing unreachable block (ram,0x004605e0) */
/* WARNING: Removing unreachable block (ram,0x004605ea) */
/* WARNING: Removing unreachable block (ram,0x004607ad) */
/* WARNING: Removing unreachable block (ram,0x004607be) */
/* WARNING: Removing unreachable block (ram,0x004607c8) */
/* WARNING: Removing unreachable block (ram,0x004607e1) */
/* WARNING: Removing unreachable block (ram,0x004607e6) */
/* WARNING: Removing unreachable block (ram,0x004607ef) */
/* WARNING: Removing unreachable block (ram,0x004607f6) */
/* WARNING: Removing unreachable block (ram,0x00460802) */
/* WARNING: Removing unreachable block (ram,0x00460819) */
/* WARNING: Removing unreachable block (ram,0x00460833) */
/* WARNING: Removing unreachable block (ram,0x0046084a) */
/* WARNING: Removing unreachable block (ram,0x00460850) */
/* WARNING: Removing unreachable block (ram,0x0046085a) */

void game_engine_on_player_death(uint param_1,uint param_2,char param_3)

{
  short sVar1;
  bool bVar2;
  char cVar3;
  int iVar4;
  short *psVar5;
  int iVar6;
  uint uVar7;
  short sVar8;
  uint in_EDX;
  int iVar9;
  int iVar10;
  short sVar11;
  char acStack_800 [2046];
  undefined2 uStack_2;

  iVar4 = DAT_006f1d20;
  iVar9 = (param_2 & 0xffff) * 0x200;
  iVar10 = *(int *)(DAT_0087a480 + 0x34) + iVar9;
  if (DAT_006f1d20 == 0) {
    return;
  }
  *(undefined4 *)(iVar10 + 0x84) = *(undefined4 *)(DAT_006f1d6c + 0xc);
  if (*(code **)(iVar4 + 0x68) != (code *)0x0) {
    (**(code **)(iVar4 + 0x68))();
  }
  if ((in_EDX == 0xffffffff) || (param_2 == 0xffffffff)) {
    bVar2 = false;
  }
  else {
    bVar2 = true;
  }
  if (((param_3 != '\0') || (!bVar2)) || (in_EDX == param_2)) {
    bVar2 = false;
  }
  else {
    bVar2 = true;
  }
  *(int *)(iVar10 + 0x2c) = *(int *)(iVar10 + 0x30) + DAT_006f1cd0;
  if (0 < DAT_006f1ccc) {
    iVar6 = *(int *)(iVar10 + 0x30) + DAT_006f1ccc;
    *(int *)(iVar10 + 0x30) = iVar6;
    iVar4 = DAT_006f1ccc * 5;
    if (iVar6 <= DAT_006f1ccc * 5) {
      iVar4 = iVar6;
    }
    *(int *)(iVar10 + 0x30) = iVar4;
    if ((bVar2) && (in_EDX != 0xffffffff)) {
      iVar4 = (in_EDX & 0xffff) * 0x200;
      iVar6 = iVar4 + *(int *)(DAT_0087a480 + 0x34);
      uVar7 = *(int *)(iVar4 + 0x30 + *(int *)(DAT_0087a480 + 0x34)) - DAT_006f1ccc;
      *(uint *)(iVar6 + 0x30) = uVar7;
      *(uint *)(iVar6 + 0x30) = uVar7 & ((int)uVar7 < 1) - 1;
    }
  }
  if (in_EDX == param_2) {
    *(int *)(iVar10 + 0x2c) = *(int *)(iVar10 + 0x2c) + DAT_006f1cd4;
  }
  if (*(short *)(iVar10 + 0xc0) != 0) {
    *(int *)(iVar10 + 0x2c) = *(int *)(iVar10 + 0x2c) + *(short *)(iVar10 + 0xc0) * DAT_006f1cf8;
    *(undefined2 *)(iVar10 + 0xc0) = 0;
  }
  iVar4 = *(int *)(iVar10 + 0x2c);
  if (iVar4 < 0x5b) {
    iVar4 = 0x5a;
  }
  *(int *)(iVar10 + 0x2c) = iVar4;
  if (9000 < iVar4) {
    iVar4 = 9000;
  }
  *(int *)(iVar10 + 0x2c) = iVar4;
  if (DAT_00719720 == 2) {
    FUN_00466cb0(0xffffffff);
  }
  if (*(char *)(*(int *)(DAT_0087a480 + 0x34) + 0xd5 + iVar9) != '\0') {
    return;
  }
  if (in_EDX != 0xffffffff) {
    if (in_EDX == param_2) {
      FUN_00460db0(0xffffffff,param_2);
      return;
    }
    cVar3 = (param_3 != '\0') + '\x04';
    goto LAB_00460406;
  }
  if (param_1 != 0xffffffff) {
    sVar8 = *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0xb4
                      );
    if (sVar8 == 0) {
      cVar3 = '\x02';
      goto LAB_00460406;
    }
    if (sVar8 == 1) {
      cVar3 = '\x03';
      goto LAB_00460406;
    }
  }
  cVar3 = '\x01';
LAB_00460406:
  FUN_00460db0();
  sVar11 = (short)in_EDX;
  sVar8 = (short)(in_EDX >> 0x10);
  if (cVar3 == '\x05') {
    if (in_EDX == 0xffffffff) {
      iVar4 = data_iterator_next();
      if (iVar4 != 0) {
        do {
          iVar4 = data_iterator_next();
        } while (iVar4 != 0);
        return;
      }
    }
    else if ((-1 < sVar11) && (sVar11 < *(short *)(DAT_0087a480 + 0x20))) {
      iVar4 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar11;
      sVar11 = *(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34));
      if ((sVar11 != 0) && ((sVar8 == 0 || (sVar11 == sVar8)))) {
        if ((*(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34) + 2) != -1) &&
           (((*(code **)(DAT_006f1d20 + 0x6c) != (code *)0x0 &&
             (cVar3 = (**(code **)(DAT_006f1d20 + 0x6c))(), cVar3 != '\0')) ||
            (cVar3 = game_engine_build_kill_feed_message_text(0xd,param_2,0x400), cVar3 != '\0'))))
        {
          uStack_2 = 0;
          chimera__multiplayer_message(acStack_800);
        }
        if (DAT_00719720 == 2) {
          FUN_004608d0(0xd,param_2);
          return;
        }
      }
    }
  }
  else if (cVar3 == '\x04') {
    iVar4 = (in_EDX & 0xffff) * 0x200;
    sVar1 = *(short *)(iVar4 + 0x98 + *(int *)(DAT_0087a480 + 0x34));
    if (((3 < sVar1) || (sVar1 == 3)) ||
       ((sVar1 == 2 ||
        ((sVar1 = *(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34) + 0x96), sVar1 == 5 ||
         ((int)sVar1 % 5 == 0)))))) {
      FUN_00460d10();
    }
    if (in_EDX == 0xffffffff) {
      iVar4 = data_iterator_next();
      while (iVar4 != 0) {
        iVar4 = data_iterator_next();
      }
    }
    else if ((-1 < sVar11) && (sVar11 < *(short *)(DAT_0087a480 + 0x20))) {
      psVar5 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar11 +
                        *(int *)(DAT_0087a480 + 0x34));
      sVar11 = *psVar5;
      if ((sVar11 != 0) && ((sVar8 == 0 || (sVar11 == sVar8)))) {
        if ((psVar5[1] != -1) &&
           (((*(code **)(DAT_006f1d20 + 0x6c) != (code *)0x0 &&
             (cVar3 = (**(code **)(DAT_006f1d20 + 0x6c))(), cVar3 != '\0')) ||
            (cVar3 = game_engine_build_kill_feed_message_text(8,param_2,0x400), cVar3 != '\0')))) {
          uStack_2 = 0;
          chimera__multiplayer_message(acStack_800);
        }
        if (DAT_00719720 == 2) {
          FUN_004608d0(8,param_2);
          return;
        }
      }
    }
  }
  return;
}
#endif
