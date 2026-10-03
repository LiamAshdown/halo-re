// game_engine_koth_broadcast_hill_times  (Ghidra: FUN_0046b7f0; named per its summary)
// address 0x46b7f0, size 297 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Requests or (when mode!=0) formats and broadcasts a
//   network HUD message reporting per-team hill-occupation time in seconds"); king_bucket_credit_
//   ticks (0x006b0ec0, this batch); message_delta_encode_message / network_session_send_to_
//   machine / network_session_broadcast_to_flagged already established call shapes elsewhere in this module.
//   0x0087a7e0 is the 16-dword network-replicated seconds array this message carries.
// register convention: mode and machine-index parameters are both ordinary stack parameters
//   (Ghidra's own param_1/param_2).
// UNSURE: the source copy reads 0x6b (107) dwords starting at king_bucket_credit_ticks, well
//   past that array's own 16 entries (into whatever globals happen to sit after it up to
//   0x006b1068); the 107th value ends up in DAT_0087a984 for reasons not recovered here. Kept
//   literal (same source span, same destination) rather than trimmed to 16, since trimming would
//   silently change DAT_0087a984's value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t shared_hud_text_draw_state; // 0x00871de0
extern int32_t king_team_hill_seconds_network[16]; // 0x0087a7e0
extern int32_t king_bucket_credit_ticks[16];       // 0x006b0ec0, this batch (source span extends
    // 107 dwords past this address -- see UNSURE)
extern int32_t king_hill_broadcast_overrun_value;  // 0x0087a984, UNSURE identity, see header
extern network_server_globals *network_server;

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server
    // parameters are visible at this call site -- the rest are forwarded pass-through from this
    // function's own (unrecovered) caller, modeled here as zero
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5, uint32_t unknown_6); // 0x4e1930

// If `mode` is 0, requests the current per-team hill times (sends message type 0x13 with just
// the request-flag fields). Otherwise converts the raw tick buckets to seconds (divide by 30),
// broadcasts them, and copies the converted 16-entry array (plus the UNSURE 107th raw dword)
// into the replicated globals. Either way, if the encoder produced a positive bit length, sends
// it to `machine_index` (or broadcasts via network_session_broadcast_to_flagged when machine_index is -1).
void game_engine_koth_broadcast_hill_times(int32_t mode, int32_t machine_index)
{
    int32_t encoded_bits;

    if (mode == 0) {
        void *field = &king_team_hill_seconds_network[0];
        void *no_extra = (void *)0;
        (void)no_extra;
        encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x13, 0, &field, 0, 1, 0);
    } else {
        int32_t seconds[107];
        int32_t i;

        for (i = 0; i < 107; i++) {
            seconds[i] = ((int32_t *)king_bucket_credit_ticks)[i];
        }
        for (i = 0; i < 16; i++) {
            seconds[i] = seconds[i] / 30;
        }

        {
            void *seconds_field = seconds;
            void *count_field = &king_team_hill_seconds_network[0]; // UNSURE: local_1bc (zero) sits
                // between these two fields; modeled as a zero count/extra field, see original
            int32_t zero_extra = 0;
            (void)count_field;
            (void)zero_extra;
            encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x13, 0, (void **)&seconds_field, (uint32_t)&king_team_hill_seconds_network[0], 1, 0);
        }

        for (i = 0; i < 16; i++) {
            king_team_hill_seconds_network[i] = seconds[i];
        }
        king_hill_broadcast_overrun_value = seconds[106];
    }

    if (encoded_bits > 0) {
        if (machine_index == -1) {
            network_session_broadcast_to_flagged(encoded_bits, network_server, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
        } else {
            network_session_send_to_machine(1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
        }
    }
}

#if 0
Original Ghidra decompilation (0x46b7f0), from tools/pack.py 0x46b7f0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0046b7f0(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int *piVar4;
  undefined4 *local_1c4;
  int *local_1c0;
  undefined4 local_1bc;
  int local_1b8 [105];
  undefined4 local_14;

  if (param_1 == 0) {
    local_1c4 = &DAT_0087a7e0;
    local_1c0 = (int *)0x0;
    iVar1 = message_delta_encode_message(0,0x13,0,&local_1c4,0,1,'\0');
  }
  else {
    piVar3 = &DAT_006b0ec0;
    piVar4 = local_1b8;
    for (iVar1 = 0x6b; iVar1 != 0; iVar1 = iVar1 + -1) {
      *piVar4 = *piVar3;
      piVar3 = piVar3 + 1;
      piVar4 = piVar4 + 1;
    }
    iVar1 = 0;
    do {
      local_1b8[iVar1] = local_1b8[iVar1] / 0x1e;
      iVar1 = iVar1 + 1;
    } while (iVar1 < 0x10);
    local_1c0 = local_1b8;
    local_1c4 = &DAT_0087a7e0;
    local_1bc = 0;
    iVar1 = message_delta_encode_message(1,0x13,0,&local_1c0,(int)&local_1c4,1,'\0');
    piVar3 = local_1b8;
    piVar4 = &DAT_0087a7e0;
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *piVar4 = *piVar3;
      piVar3 = piVar3 + 1;
      piVar4 = piVar4 + 1;
    }
    _DAT_0087a984 = local_14;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
