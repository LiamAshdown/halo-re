// build_local_player_position_update  (Ghidra: build_local_player_position_update, already named)
// address 0x4e81e0, size 257 bytes
// name confidence: 0.9   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md; types/game.h player (unknown_e8 is the start of a
// 0xc-dword run; this function reads +0xe8, +0xec, +0xf0, +0xf4, +0xf8, +0xfc, +0x100 inside it);
// message_delta_parameters_protocol_send_update.c (this batch) fixes message_delta_encode_message's
// real signature; the caller network_client_send_local_player_updates.c (this batch) passes EBX =
// &(a local byte) and ESI = the player object, which is what fixes this function's register
// convention.
// register convention: EBX -> out_changed (a caller-owned byte, cleared unconditionally and left
// clear unless the ack is actually sent), ESI -> plr.
//   // blam-cc: EBX -> out_changed, ESI -> plr
// UNSURE: player+0xf4 is read here as a single byte (the ack's baseline_id slot) even though
// types/game.h documents +0xf4 as the 4-byte unknown_f4/datum_index slot of the same run; only
// the low byte is used by this function, so both are kept as they are (no header change). The
// on-stack scratch record built from +0xe8/+0xf4/+0xf8/+0xfc/+0x100 matches
// types/networking.h local_player_update_ack's shape (update_id, baseline_id, pad, position) but
// is built here field-by-field rather than through that type, since the source fields split
// across a byte read of +0xf4 that the named type does not model.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_ack_resend_interval_ms; // 0x00689484
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick

extern uint32_t __stdcall GetTickCount(void);
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void network_player_update_history_log_write(const char *format, ...); // this module, 0x4e7f90

// If plr's queued position-ack sequence number is valid (0..63) and either no ack has been sent
// yet or the resend interval has elapsed, encodes a rate-limited message-0x23 local-player
// position acknowledgement from plr's cached fields, logs it, and records the send time. Returns
// the encoded message size, or 0 if nothing was sent.
int32_t build_local_player_position_update(uint8_t *out_changed, player *plr)
    // blam-cc: EBX -> out_changed, ESI -> plr
{
    int32_t encoded_size;
    uint8_t *plr_bytes;
    local_player_update_ack ack;
    void *ack_ptr;
    uint32_t next_id;
    uint32_t logged_id;

    encoded_size = 0;
    *out_changed = 0;
    plr_bytes = (uint8_t *)plr;
    if (-1 < *(int32_t *)(plr_bytes + 0xf4) && *(int32_t *)(plr_bytes + 0xf4) < 0x40) {
        ack.update_id = *(uint8_t *)(plr_bytes + 0xe8);
        ack.baseline_id = *(uint8_t *)(plr_bytes + 0xf4);
        *(uint32_t *)&ack.position.x = *(uint32_t *)(plr_bytes + 0xf8);
        *(uint32_t *)&ack.position.y = *(uint32_t *)(plr_bytes + 0xfc);
        *(uint32_t *)&ack.position.z = *(uint32_t *)(plr_bytes + 0x100);
        if (*(int32_t *)(plr_bytes + 0xec) == -1 ||
            *(int32_t *)(plr_bytes + 0xf0) + network_ack_resend_interval_ms <=
                game_time->game_time) {
            ack_ptr = &ack;
            encoded_size = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x23, 0, &ack_ptr, 0, 1, '\0');
            *out_changed = 0;
            next_id = (*(uint32_t *)(plr_bytes + 0xe8) + 1) & 0x8000001f;
            if ((int32_t)next_id < 0) {
                next_id = (next_id - 1 | 0xffffffe0) + 1;
            }
            logged_id = ack.baseline_id;
            *(uint32_t *)(plr_bytes + 0xe8) = next_id;
            network_player_update_history_log_write("[%d]: [%d]:\t Acked [%d]\n", GetTickCount(),
                game_time->game_time, logged_id);
            *(int32_t *)(plr_bytes + 0xec) = *(int32_t *)(plr_bytes + 0xf4);
            *(int32_t *)(plr_bytes + 0xf0) = game_time->game_time;
        }
    }
    return encoded_size;
}

#if 0
Original Ghidra decompilation (0x4e81e0), from tools/pack.py 0x4e81e0:

int build_local_player_position_update(void)

{
  int iVar1;
  int iVar2;
  DWORD DVar3;
  uint uVar4;
  uint uVar5;
  undefined1 *unaff_EBX;
  int unaff_ESI;
  undefined4 uVar6;
  undefined1 *local_18;
  undefined4 local_14;
  undefined1 local_10;
  byte local_f;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar2 = 0;
  *unaff_EBX = 0;
  if ((-1 < *(int *)(unaff_ESI + 0xf4)) && (*(int *)(unaff_ESI + 0xf4) < 0x40)) {
    local_10 = *(undefined1 *)(unaff_ESI + 0xe8);
    local_f = *(byte *)(unaff_ESI + 0xf4);
    local_c = *(undefined4 *)(unaff_ESI + 0xf8);
    local_8 = *(undefined4 *)(unaff_ESI + 0xfc);
    local_4 = *(undefined4 *)(unaff_ESI + 0x100);
    if ((*(int *)(unaff_ESI + 0xec) == -1) ||
       ((uint)(*(int *)(unaff_ESI + 0xf0) + DAT_00689484) <= *(uint *)(DAT_006f1d6c + 0xc))) {
      local_18 = &local_10;
      local_14 = 0;
      iVar2 = message_delta_encode_message(0,0x23,0,&local_18,0,1,'\0');
      *unaff_EBX = 0;
      iVar1 = DAT_006f1d6c;
      uVar5 = *(int *)(unaff_ESI + 0xe8) + 1U & 0x8000001f;
      if ((int)uVar5 < 0) {
        uVar5 = (uVar5 - 1 | 0xffffffe0) + 1;
      }
      uVar4 = (uint)local_f;
      *(uint *)(unaff_ESI + 0xe8) = uVar5;
      uVar6 = *(undefined4 *)(iVar1 + 0xc);
      DVar3 = GetTickCount();
      network_player_update_history_log_write("[%d]: [%d]:\t Acked [%d]\n",DVar3,uVar6,uVar4);
      iVar1 = DAT_006f1d6c;
      *(undefined4 *)(unaff_ESI + 0xec) = *(undefined4 *)(unaff_ESI + 0xf4);
      *(undefined4 *)(unaff_ESI + 0xf0) = *(undefined4 *)(iVar1 + 0xc);
    }
  }
  return iVar2;
}
#endif
