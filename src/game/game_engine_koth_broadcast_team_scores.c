// game_engine_koth_broadcast_team_scores  (Ghidra: FUN_0046d060; named per this rewrite)
// address 0x46d060, size 354 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Requests or broadcasts the King-of-the-Hill variant's
//   team score/target state over the network, converting tick counts to seconds where needed");
//   king_alt_score_target / king_alt_team_score / king_alt_player_score (this batch,
//   0x006b1148/114c/118c, contiguous in that order); message_delta_encode_message /
//   network_session_send_to_machine / network_session_broadcast_to_flagged already established call shapes.
// register convention: mode and machine-index parameters are both ordinary stack parameters
//   (Ghidra's own param_1/param_2).
// UNSURE: the source copy reads 0x51 (81) dwords starting at king_alt_score_target, well past
//   the 17 dwords (target + 16 team scores) that are individually divided by 30 -- the
//   remaining 64 dwords (16 more, divided, presumably per-player scores, plus 48 more copied
//   verbatim into a third destination) are modeled as opaque blocks at their real source/
//   destination addresses rather than given semantic names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern uint8_t shared_hud_text_draw_state; // 0x00871de0
extern uint8_t *network_session; // 0x0071c2d4
extern game_variant game_engine_variant; // 0x006f1c88 (unknown_8c aliased 0x006f1d14)
extern int32_t king_alt_score_target;      // 0x006b1148, this batch
extern int32_t king_alt_team_score[16];    // 0x006b114c, this batch
extern int32_t king_alt_player_score[];    // 0x006b118c, this batch (only the first 16 dwords
                                            // of this array are read here)
extern int32_t king_alt_team_scores_network[16];   // 0x0087a680, request-flag field / seconds dest
extern int32_t king_alt_player_scores_network[16]; // 0x0087a6c4
extern int32_t king_alt_team_scores_network2[16];  // 0x0087a684
extern int32_t king_alt_scores_network_tail[16];   // 0x0087a744, UNSURE identity, raw verbatim copy

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5, uint32_t unknown_6); // 0x4e1930

void game_engine_koth_broadcast_team_scores(int32_t mode, int32_t machine_index)
{
    int32_t encoded_bits;

    if (mode == 0) {
        void *field = &king_alt_team_scores_network[0];
        encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x12, 0, &field, 0, 1, 0);
    } else {
        int32_t target_and_team[17];
        int32_t player_scores[16];
        int32_t i;

        target_and_team[0] = king_alt_score_target;
        for (i = 0; i < 16; i++) {
            target_and_team[i + 1] = king_alt_team_score[i];
            player_scores[i] = ((int32_t *)king_alt_player_score)[i];
        }

        if (game_engine_variant.unknown_8c != 2) {
            for (i = 0; i < 16; i++) {
                target_and_team[i + 1] /= 30;
                player_scores[i] /= 30;
            }
        }

        {
            void *fields0 = target_and_team;
            void *fields1 = &king_alt_team_scores_network[0];
            encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x12, 0, (void **)&fields0,
                (uint32_t)&fields1, 1, 0);
        }

        for (i = 0; i < 16; i++) {
            king_alt_player_scores_network[i] = player_scores[i];
        }
        for (i = 0; i < 16; i++) {
            king_alt_team_scores_network2[i] = target_and_team[i + 1];
        }
        for (i = 0; i < 16; i++) {
            king_alt_scores_network_tail[i] = ((int32_t *)king_alt_player_score)[32 + i]; // UNSURE
        }
    }

    if (encoded_bits > 0) {
        if (machine_index == -1) {
            network_session_broadcast_to_flagged(encoded_bits, network_server_pointer, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
        } else {
            network_session_send_to_machine(1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
        }
    }
}

#if 0
Original Ghidra decompilation (0x46d060), from tools/pack.py 0x46d060:

void FUN_0046d060(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  undefined4 *puVar4;
  int *piVar5;
  undefined4 *puVar6;
  undefined4 *local_15c;
  int *local_158;
  undefined4 local_154;
  int local_150 [17];
  int local_10c [32];
  undefined4 local_8c [34];

  if (param_1 == 0) {
    local_15c = &DAT_0087a680;
    local_158 = (int *)0x0;
    iVar1 = message_delta_encode_message(0,0x12,0,&local_15c,0,1,'\0');
  }
  else {
    piVar3 = &DAT_006b1148;
    piVar5 = local_150;
    for (iVar1 = 0x51; iVar1 != 0; iVar1 = iVar1 + -1) {
      *piVar5 = *piVar3;
      piVar3 = piVar3 + 1;
      piVar5 = piVar5 + 1;
    }
    if (DAT_006f1d14 != 2) {
      iVar1 = 0;
      do {
        local_150[iVar1 + 1] = local_150[iVar1 + 1] / 0x1e;
        local_10c[iVar1] = local_10c[iVar1] / 0x1e;
        iVar1 = iVar1 + 1;
      } while (iVar1 < 0x10);
    }
    local_158 = local_150;
    local_15c = &DAT_0087a680;
    local_154 = 0;
    iVar1 = message_delta_encode_message(1,0x12,0,&local_158,(int)&local_15c,1,'\0');
    piVar3 = local_10c;
    piVar5 = &DAT_0087a6c4;
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *piVar5 = *piVar3;
      piVar3 = piVar3 + 1;
      piVar5 = piVar5 + 1;
    }
    piVar3 = local_150;
    piVar5 = &DAT_0087a684;
    for (iVar2 = 0x10; piVar3 = piVar3 + 1, iVar2 != 0; iVar2 = iVar2 + -1) {
      *piVar5 = *piVar3;
      piVar5 = piVar5 + 1;
    }
    puVar4 = local_8c;
    puVar6 = &DAT_0087a744;
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar6 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar6 = puVar6 + 1;
    }
  }
  if (0 < iVar1) {
    if (param_2 == -1) {
      FUN_004e1a80(1,&DAT_00871de0);
      return;
    }
    network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,0,3);
  }
  return;
}
#endif
