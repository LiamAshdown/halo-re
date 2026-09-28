// game_engine_ctf_broadcast_state  (Ghidra: FUN_0046ec10; named per its summary)
// address 0x46ec10, size 275 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Requests or broadcasts the current Capture-the-Flag
//   state (active flags, per-team assignments, and captured bitmasks) over the network");
//   types/game.h ctf_globals (live 0x006b1290 / replicated 0x0087a520, flag_id_mask +0,
//   team_flag_id[16] +4); ctf_neutral_flag_id (0x006b1314, mirrored to replicated+0x84);
//   game_engine_bucket_scores[16] (0x006b1318, this batch's game_engine_check_bucket_scores_
//   and_end_round.c) sits at ctf_globals live+0x88 and is mirrored to replicated+0x88;
//   ctf_team_captured_flags_mask (0x006b12d4, ctf_globals::unknown_44, this batch) is mirrored
//   to replicated+0x44.
// register convention: request-vs-broadcast fields pointer and machine index are both ordinary
//   stack parameters (Ghidra's own param_1/param_2).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"

extern ctf_globals ctf_globals_live;    // 0x006b1290
extern ctf_globals ctf_globals_network; // 0x0087a520
extern int32_t ctf_neutral_flag_id;     // 0x006b1314
extern uint8_t shared_hud_text_draw_state; // 0x00871de0
extern network_server_globals *network_server;

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, ECX server
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5, uint32_t unknown_6); // 0x4e1930

void game_engine_ctf_broadcast_state(void *request_fields, int32_t machine_index)
{
    int32_t encoded_bits;

    if (request_fields == (void *)0) {
        void *field = &ctf_globals_network;
        encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x14, 0, &field, 0, 1, 0);
    } else {
        void *fields0 = &ctf_globals_live;
        void *fields1 = &ctf_globals_network;
        int32_t i;

        encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x14, 0, (void **)&fields0, (uint32_t)&fields1, 1, 0);

        *(int32_t *)((uint8_t *)&ctf_globals_network + 0x84) = ctf_neutral_flag_id;
        for (i = 0; i < 16; i++) {
            ((int32_t *)((uint8_t *)&ctf_globals_network + 0x88))[i] =
                ((int32_t *)((uint8_t *)&ctf_globals_live + 0x88))[i];
        }
        for (i = 0; i < 16; i++) {
            ctf_globals_network.team_flag_id[i] = ctf_globals_live.team_flag_id[i];
        }
        for (i = 0; i < 16; i++) {
            ((int32_t *)((uint8_t *)&ctf_globals_network + 0x44))[i] =
                ((int32_t *)((uint8_t *)&ctf_globals_live + 0x44))[i];
        }
        ctf_globals_network.flag_id_mask = ctf_globals_live.flag_id_mask;
    }

    if (encoded_bits > 0) {
        if (machine_index == -1) {
            network_session_broadcast_to_flagged(network_server_pointer, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
        } else {
            network_session_send_to_machine(1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
        }
    }
}

#if 0
Original Ghidra decompilation (0x46ec10), from tools/pack.py 0x46ec10:

void FUN_0046ec10(undefined4 *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  bool bVar5;
  undefined4 *local_8;
  undefined4 local_4;

  bVar5 = param_1 == (void *)0x0;
  param_1 = &DAT_0087a520;
  if (bVar5) {
    local_8 = (undefined4 *)0x0;
    iVar1 = message_delta_encode_message(0,0x14,0,&param_1,0,1,'\0');
  }
  else {
    local_8 = &DAT_006b1290;
    local_4 = 0;
    iVar1 = message_delta_encode_message(1,0x14,0,&local_8,(int)&param_1,1,'\0');
    DAT_0087a5a4 = DAT_006b1314;
    puVar3 = &DAT_006b1318;
    puVar4 = &DAT_0087a5a8;
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    puVar3 = &DAT_006b1294;
    puVar4 = &DAT_0087a524;
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    puVar3 = &DAT_006b12d4;
    puVar4 = &DAT_0087a564;
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    DAT_0087a520 = DAT_006b1290;
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
