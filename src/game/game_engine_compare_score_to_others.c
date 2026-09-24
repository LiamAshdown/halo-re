// game_engine_compare_score_to_others  (Ghidra: FUN_00463480; renamed per its summary)
// address 0x463480, size 347 bytes
// name confidence: 0.35   rewrite confidence: 0.15 (zero callers in this build; likely reached
// only through a vtable slot nothing currently populates)
// evidence: out/phase4/game_functions.md ("Compares one object score against every other
// tracked object (via the game engine score callback) to compute whether it is winning, tied,
// and how many objects tie with it"); types/game.h game_engine_definition::get_score (+0x4c,
// "0x463480 / kill-feed builder; takes a player handle (or -1) and returns its score" -- this
// function is that comment's own self-reference); player::team (+0x20).
// register convention: no register-passed arguments; param_1/param_2 are this function's own
// stack parameters (a subject handle and a team-mode flag).
//   // blam-cc: stack -> subject, team_mode
// UNSURE: the inner get_score call is shown by Ghidra as literally
// `(**get_score)(0xffffffff, team_mode)` for every iterated entry, never actually passing that
// entry's own identity -- this looks wrong (it would score the same "-1" subject every time) but
// is transcribed exactly as decompiled rather than "corrected" to pass the entry, since nothing
// in this batch's evidence proves what the real argument should be. The returned value packs a
// 16-bit flag set (bit0 any-tie, bit1 all-tied, bit2 exactly-one-other-compared, bit3
// team_mode) in its low half and a "higher score" count in its high half.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern data_array *player_data;                     // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// blam-cc: stack -> subject, team_mode
uint32_t game_engine_compare_score_to_others(uint32_t subject, int32_t team_mode)
{
    uint8_t all_tied = 1;
    uint8_t any_tied = 0;
    int32_t compared_count = 1;
    uint16_t higher_count = 0;
    uint16_t flags;
    uint32_t seen_teams = 0;

    if (current_game_engine->get_score != 0) {
        int32_t subject_score = ((int32_t (*)(uint32_t, int32_t))current_game_engine->get_score)(subject, team_mode);
        data_iterator iter;
        void *element;

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)0xffffffff;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        element = data_iterator_next(&iter);
        while (element != 0) {
            player *entry = (player *)element;
            uint8_t skip;

            if (team_mode == 1) {
                player *subject_player = (player *)((uint8_t *)player_data->data +
                    (subject & 0xffff) * sizeof(player));
                skip = (entry->team == subject_player->team);
            } else {
                skip = (subject == 0xffffffff);
            }

            if (!skip) {
                if (team_mode == 1) {
                    uint32_t team_bit = 1u << (entry->team & 0x1f);
                    if ((team_bit & seen_teams) != 0) {
                        goto next;
                    }
                    seen_teams = seen_teams | team_bit;
                }
                {
                    int32_t other_score = ((int32_t (*)(uint32_t, int32_t))current_game_engine->get_score)
                        (0xffffffff, team_mode); // UNSURE: see file header
                    compared_count = compared_count + 1;
                    if (other_score == subject_score) {
                        any_tied = 1;
                    } else {
                        all_tied = 0;
                        if (subject_score < other_score) {
                            higher_count = higher_count + 1;
                        }
                    }
                }
            }
        next:
            element = data_iterator_next(&iter);
        }
    }

    flags = (team_mode != 1) ? 0 : 0xffff;
    if (any_tied) {
        flags = (flags & 8) | 1;
    } else {
        flags = flags & 8;
    }
    if (all_tied && any_tied) {
        flags = flags | 2;
    }
    if (compared_count == 2) {
        flags = flags | 4;
    }
    return ((uint32_t)higher_count << 16) | flags;
}

#if 0
Original Ghidra decompilation (0x463480), from tools/pack.py 0x463480:

uint FUN_00463480(uint param_1,int param_2)

{
  int iVar1;
  bool bVar2;
  byte bVar3;
  ushort uVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  bool bVar8;
  undefined4 uStack_1c;
  uint local_18;
  int local_14;

  iVar1 = *(int *)(DAT_0087a480 + 0x34);
  bVar3 = 1;
  bVar2 = false;
  local_14 = 1;
  uStack_1c._2_2_ = 0;
  if (*(code **)(DAT_006f1d20 + 0x4c) != (code *)0x0) {
    local_18 = 0;
    iVar5 = (**(code **)(DAT_006f1d20 + 0x4c))(param_1,param_2);
    iVar6 = data_iterator_next();
    uStack_1c._2_2_ = 0;
    while (iVar6 != 0) {
      if (param_2 == 1) {
        bVar8 = *(int *)(iVar6 + 0x20) == *(int *)((param_1 & 0xffff) * 0x200 + iVar1 + 0x20);
      }
      else {
        bVar8 = param_1 == 0xffffffff;
      }
      if (!bVar8) {
        if (param_2 == 1) {
          uVar7 = 1 << ((byte)*(undefined4 *)(iVar6 + 0x20) & 0x1f);
          if ((uVar7 & local_18) != 0) goto LAB_0046356f;
          local_18 = local_18 | uVar7;
        }
        iVar6 = (**(code **)(DAT_006f1d20 + 0x4c))(0xffffffff,param_2);
        local_14 = local_14 + 1;
        if (iVar6 == iVar5) {
LAB_0046356a:
          bVar2 = true;
        }
        else {
          bVar3 = 0;
          if (iVar5 < iVar6) {
            uStack_1c._2_2_ = uStack_1c._2_2_ + 1;
          }
          else if (iVar6 == iVar5) goto LAB_0046356a;
        }
      }
LAB_0046356f:
      iVar6 = data_iterator_next();
    }
  }
  uVar4 = (param_2 != 1) - 1;
  if (bVar2) {
    uVar4 = uVar4 & 8 | 1;
  }
  else {
    uVar4 = uVar4 & 8;
  }
  if ((bool)(bVar3 & bVar2)) {
    uVar4 = uVar4 | 2;
  }
  if (local_14 == 2) {
    uStack_1c = CONCAT22(uStack_1c._2_2_,uVar4) | 4;
    return uStack_1c;
  }
  uStack_1c = CONCAT22(uStack_1c._2_2_,uVar4);
  return uStack_1c;
}
#endif
