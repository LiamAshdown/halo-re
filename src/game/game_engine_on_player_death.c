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
// UNSURE: game_engine_broadcast_kill_feed_gated is called once with two visible arguments (0xffffffff, victim) on the
// self-kill early-return path, and once with none at all on the shared "build a message"
// path -- modelled here as (killer, victim) on both, since killer is whatever register-passed
// value was live at each site. The `in_EDX == -1` broadcast loop (message category 5, no valid
// killer) drains the player iterator without touching any of its elements in Ghidra's own
// rendering; preserved literally as a no-op drain rather than invented.
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
extern void game_engine_broadcast_kill_feed_or_direct(void); // 0x460d10, not in this batch
extern void game_engine_broadcast_kill_feed_gated(datum_index killer, datum_index victim); // 0x460db0, not in this batch; UNSURE args
extern void game_engine_notify_kill_event(int32_t message_type, datum_index victim); // 0x4608d0
extern uint8_t game_engine_build_kill_feed_message_text(wchar_t *out, uint32_t message_type,
    datum_index victim, size_t buffer_size); // 0x45e680, this batch
extern void chimera__multiplayer_message(wchar_t *text); // 0x4ab4b0, UNSURE: char* vs wchar_t*
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

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
            game_engine_broadcast_kill_feed_gated((datum_index)0xffffffff, victim);
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
    game_engine_broadcast_kill_feed_gated(killer, victim);

    if (message_category == 5) {
        if (killer == (datum_index)0xffffffff) {
            data_iterator iter;
            void *element;
            iter.data = player_data;
            iter.next_index = 0;
            iter.index = (datum_index)0xffffffff;
            iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
            element = data_iterator_next(&iter); // UNSURE: drains the iterator with no per-element effect, as Ghidra shows
            while (element != 0) {
                element = data_iterator_next(&iter);
            }
        } else {
            int16_t index = (int16_t)killer;
            if (-1 < index && index < player_data->maximum_count) {
                player *k = (player *)((uint8_t *)player_data->data + player_data->size * index);
                if (k->identifier != 0 &&
                    ((int16_t)((uint32_t)killer >> 16) == 0 || k->identifier == (int16_t)((uint32_t)killer >> 16))) {
                    if (k->local_player_index != -1) {
                        if ((current_game_engine->build_message_text != 0 &&
                             ((char (*)(void))current_game_engine->build_message_text)() != 0) ||
                            game_engine_build_kill_feed_message_text(kill_feed_buffer, 0x0d, victim, 0x400) != 0) {
                            kill_feed_buffer[sizeof(kill_feed_buffer) / sizeof(kill_feed_buffer[0]) - 1] = 0;
                            chimera__multiplayer_message(kill_feed_buffer);
                        }
                        if (network_game_mode == 2) {
                            game_engine_notify_kill_event(0x0d, victim);
                            return;
                        }
                    }
                }
            }
        }
    } else if (message_category == 4) {
        player *k = (player *)((uint8_t *)player_data->data + (killer & 0xffff) * sizeof(player));
        if (3 < k->multikill_count || k->multikill_count == 3 || k->multikill_count == 2 ||
            k->killing_spree_count == 5 || k->killing_spree_count % 5 == 0) {
            game_engine_broadcast_kill_feed_or_direct();
        }

        if (killer == (datum_index)0xffffffff) {
            data_iterator iter;
            void *element;
            iter.data = player_data;
            iter.next_index = 0;
            iter.index = (datum_index)0xffffffff;
            iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
            element = data_iterator_next(&iter);
            while (element != 0) {
                element = data_iterator_next(&iter);
            }
        } else {
            int16_t index = (int16_t)killer;
            if (-1 < index && index < player_data->maximum_count) {
                player *kk = (player *)((uint8_t *)player_data->data + player_data->size * index);
                if (kk->identifier != 0 &&
                    ((int16_t)((uint32_t)killer >> 16) == 0 || kk->identifier == (int16_t)((uint32_t)killer >> 16))) {
                    if (kk->local_player_index != -1) {
                        if ((current_game_engine->build_message_text != 0 &&
                             ((char (*)(void))current_game_engine->build_message_text)() != 0) ||
                            game_engine_build_kill_feed_message_text(kill_feed_buffer, 8, victim, 0x400) != 0) {
                            kill_feed_buffer[sizeof(kill_feed_buffer) / sizeof(kill_feed_buffer[0]) - 1] = 0;
                            chimera__multiplayer_message(kill_feed_buffer);
                        }
                        if (network_game_mode == 2) {
                            game_engine_notify_kill_event(8, victim);
                            return;
                        }
                    }
                }
            }
        }
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
