// game_engine_broadcast_kill_feed_by_relationship  (Ghidra: FUN_00460c10; renamed per its
// summary)
// address 0x460c10, size 245 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Broadcasts one of several kill-feed message variants
// to each recipient depending on their team relationship to the source player"); types/game.h
// player::team (+0x20), team_pair_globals::enemy_bits (+0xa4, indexed a*10+b, the same bitmap
// teams_are_enemies (0x45bd50) inverts); the "no active game engine -> use the raw enemy bitmap,
// otherwise every team is hostile" branch mirrors teams_are_enemies exactly.
// register convention: no register-passed arguments Ghidra recovers; all five are this
// function's own stack parameters.
// UNSURE: same unrecoverable trailing chimera__kill_feed arguments (subject, broadcast) as the
// sibling broadcast helpers in this address range; modeled as forwarded parameters. Also note
// the asymmetry preserved below is genuinely in the binary, not a translation slip: with no
// multiplayer engine loaded, `message_a` is the id used when the pair the raw enemy bitmap says
// ARE hostile; with an engine loaded, `message_a` is instead the id used when the two players
// are on the SAME team. Transcribed exactly as Ghidra's goto/fallthrough shows it, not
// "corrected" to be symmetric.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data;                   // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern team_pair_globals *team_pair_data;        // 0x006b0b84

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void chimera__kill_feed(datum_index recipient, int32_t param_1, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30, this batch

// blam-cc: stack -> source_player, no_source_message, message_a, message_b, subject
// For every in-use player, broadcasts a kill-feed message id to it: `no_source_message` when
// there is no `source_player`; otherwise `message_a` or `message_b` depending on the pair's team
// relationship (see the UNSURE note above for the exact, asymmetric rule); the id is skipped
// when it comes out -1.
void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player, int32_t no_source_message,
    int32_t message_a, int32_t message_b, uint32_t subject)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;

    element = data_iterator_next(&iter);
    while (element != 0) {
        player *p = (player *)element;
        int32_t message = no_source_message;

        if (source_player != (uint32_t)0xffffffff) {
            int16_t their_team = p->team;
            int16_t source_team = ((player *)((uint8_t *)player_data->data +
                (source_player & 0xffff) * sizeof(player)))->team;

            message = message_b; // the fall-through default taken whenever neither branch
                                  // below reaches its own "keep message_a" goto
            if (current_game_engine == 0) {
                if (0 <= source_team && source_team < 10 && 0 <= their_team && their_team < 10) {
                    int32_t pair = their_team + source_team * 10;
                    char enemies = (team_pair_data->enemy_bits[pair >> 5] >>
                        (pair & 0x1f) & 1) != 0;
                    if (enemies) {
                        message = message_a;
                    }
                }
                // else: range check failed -- Ghidra's own fall-through leaves message == message_b
            } else if (their_team == source_team) {
                message = message_a;
            }
        }

        if (message != -1) {
            chimera__kill_feed(iter.index, 0xffffffff, (uint32_t)message, subject, '\0'); // UNSURE: last 2 chimera__kill_feed args
        }
        element = data_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x460c10), from tools/pack.py 0x460c10:

void FUN_00460c10(uint param_1,int param_2,int param_3,int param_4,undefined4 param_5)

{
  short sVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  char cVar6;
  
  iVar3 = *(int *)(DAT_0087a480 + 0x34);
  iVar4 = data_iterator_next();
  do {
    if (iVar4 == 0) {
      return;
    }
    iVar5 = param_2;
    if (param_1 != 0xffffffff) {
      sVar1 = *(short *)(iVar4 + 0x20);
      sVar2 = *(short *)((param_1 & 0xffff) * 0x200 + iVar3 + 0x20);
      if (DAT_006f1d20 == 0) {
        if ((((-1 < sVar2) && (sVar2 < 10)) && (-1 < sVar1)) && (sVar1 < 10)) {
          iVar4 = (int)sVar1 + sVar2 * 10;
          cVar6 = '\x01' - ((1 << ((byte)iVar4 & 0x1f) &
                            *(uint *)(DAT_006b0b84 + 0xa4 + (iVar4 >> 5) * 4)) != 0);
          goto LAB_00460cce;
        }
      }
      else {
        cVar6 = sVar2 != sVar1;
LAB_00460cce:
        iVar5 = param_3;
        if (cVar6 == '\0') goto LAB_00460cdc;
      }
      iVar5 = param_4;
    }
LAB_00460cdc:
    if (iVar5 != -1) {
      chimera__kill_feed(0xffffffff,iVar5,param_5);
    }
    iVar4 = data_iterator_next();
  } while( true );
}
#endif
