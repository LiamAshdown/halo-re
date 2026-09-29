// handle_remote_player_action_update  (Ghidra: handle_remote_player_action_update, already named)
// address 0x4e60c0, size 420 bytes
// name confidence: 0.9   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md; cea-pdb match on both literal strings; shares
// the player_data validation and PTR_DAT_00687558-style handle shape with this batch's
// player_update_client_remote_player_action_update_from_network.c and
// player_update_remote_player_action_update_apply.c, which are its two callers; types/game.h
// player (unknown_e8/ec/f0..11c). Disassembly (objdump -d -M intel) resolves the register
// convention, the wraparound distance check, and the action-record shape, all of which Ghidra's
// decompile either dropped or (in one case) actively misrepresents -- see the UNSURE note below.
// register convention: EAX -> control_source (the 12-dword/48-byte record the caller decoded,
// copied onto the player in the baseline case), stack -> header, is_baseline.
//   // blam-cc: EAX -> control_source, stack -> header, is_baseline
// REVIEW PASS 2026-09-20: control_source and the index pointer now use the shared
// types/networking.h remote_player_action_state / remote_player_update_header declarations
// instead of raw byte pointers, and is_baseline is uint8_t rather than int32_t -- two of the
// five callers (0x4e5870, 0x4e5a30) pass a stack dword whose upper three bytes are stale, and
// this function only ever tests AL.
// UNSURE (load-bearing, contradicts Ghidra's own decompile): Ghidra's pseudo-C claims
// `in_EAX[0x20] = 0xff; in_EAX[0x21] = 0xff;` right after the yaw/pitch computation, but the
// disassembly is `mov WORD PTR [ebx+0x20],ax` -- a plain register store, and AX at that point
// still holds the low 16 bits of `player_data->size * index` from the player lookup earlier in
// this same function (nothing reloads EAX in between). This is transcribed as the real,
// apparently-unintentional leftover-register write, not as the fabricated 0xffff constant.
// UNSURE: see player_update_history_log_printf_filtered.c (this module) for why its
// category_flags parameter was dropped in favour of a plain variadic format -- this function's
// three call sites are exactly the evidence that finding is based on.
// UNSURE: circular_queue_push's exact signature; disassembly shows the pushed record pointer plus
// EBX = &candidate->update_history live at the call, but the `add esp,0x10` cleanup accounts for
// more stack space than the one visible push, which was not further chased (foreign, < this
// batch, 0x47a1a0).
// UNSURE: the yaw/pitch fields (control_source+8/+0xc) and the direction vector they are computed
// from (control_source+0x24/0x28/0x2c) are not named in any header; control_source's type is left
// as a raw byte pointer since it is this batch's own decoded scratch record, not a struct owned
// by types/networking.h or types/game.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern data_array *player_data; // 0x0087a480
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick
extern double atan2(double y, double x); // x87 FPATAN
extern double sqrt(double x);            // x87 FSQRT, Ghidra SQRT() pseudo-function

extern void player_update_history_log_printf_filtered(player *target_player, int32_t category,
    const char *format, ...); // this module, 0x4e5f20
extern uint8_t circular_queue_push(circular_queue *queue, void *record);
    // blam-cc: EBX -> queue, stack -> record; foreign (< this batch), 0x47a1a0

// Applies a decoded remote-player action update (baseline: latch control_source directly onto
// the player's control record; delta: validate the baseline id and action-index distance, then
// apply), computing yaw/pitch from the resulting direction vector and pushing an action-queue
// entry the first time a real action index is seen.
void handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline)
    // blam-cc: EAX -> control_source, stack -> header, is_baseline
{
    int32_t handle;
    int16_t index;
    int16_t salt;
    int32_t stride_offset;
    player *candidate;
    uint8_t baseline_id;
    uint8_t action_index;

    handle = header->player_index;
    candidate = 0;
    if (handle != -1) {
        index = (int16_t)handle;
        if (index >= 0 && index < player_data->maximum_count) {
            salt = (int16_t)((uint32_t)handle >> 16);
            stride_offset = (int32_t)player_data->size * (int32_t)index;
            candidate = (player *)((uint8_t *)player_data->data + stride_offset);
            if (candidate->identifier == 0 || (salt != 0 && candidate->identifier != salt)) {
                candidate = 0;
            }
        }
    }
    if (candidate == 0) {
        return;
    }

    if (is_baseline == 1) {
        baseline_id = header->baseline_id;
        candidate->baseline_update_id = baseline_id;
        memcpy(&candidate->unknown_f0, control_source, sizeof(*control_source));
    } else {
        int32_t distance;

        baseline_id = header->baseline_id;
        if ((uint32_t)baseline_id != candidate->baseline_update_id) {
            player_update_history_log_printf_filtered(candidate, 2,
                "[%d]: Threw away remote player action update with base baseline, [%d] != [%d].",
                game_time->game_time, baseline_id, candidate->baseline_update_id);
            return;
        }
        action_index = header->update_id;
        if ((int32_t)action_index <= candidate->last_update_id) {
            distance = ((int32_t)action_index - candidate->last_update_id) + 0x40;
        } else {
            distance = (int32_t)action_index - candidate->last_update_id;
        }
        if (distance >= 0x20) {
            return;
        }
    }

    action_index = header->update_id;
    if (candidate->last_update_id != -1) {
        float x = control_source->direction.i;
        float y = control_source->direction.j;
        float z = control_source->direction.k;

        control_source->yaw = (float)atan2(y, x);
        control_source->pitch = (float)atan2(z, sqrt(x * x + y * y));
        // UNSURE: leftover register write, not the 0xffff Ghidra's decompile claims -- see file header
        control_source->unknown_20 = (uint16_t)stride_offset;
        player_update_history_log_printf_filtered(candidate, 2, "Received action [%d]", action_index);

        if (candidate->last_update_id != -1) { // UNSURE: Ghidra's own check here is `!= -1` a second time; kept as-is
            uint32_t record[11];
            uint8_t first_byte = (uint8_t)control_source->flags;

            record[0] = action_index;
            record[1] = first_byte;
            record[2] = first_byte;
            memcpy(&record[3], &control_source->unknown_04, sizeof(uint32_t) * 8);
            if (!circular_queue_push((circular_queue *)&candidate->update_history, record)) {
                player_update_history_log_printf_filtered(candidate, 2,
                    "[%d]: Remote player action_queue overflow.\n",
                    game_time->game_time);
            }
        }
    }
    candidate->last_update_id = action_index;
}

#if 0
Original Ghidra decompilation (0x4e60c0), from tools/pack.py 0x4e60c0:

void handle_remote_player_action_update(int *param_1,char param_2)

{
  char cVar1;
  short sVar2;
  byte *in_EAX;
  uint uVar3;
  int iVar4;
  int iVar5;
  short sVar6;
  byte *pbVar7;
  undefined4 *puVar8;
  float10 fVar9;
  uint local_2c;
  uint local_28;
  uint local_24;
  undefined4 local_20 [8];

  iVar4 = *param_1;
  if (((iVar4 != -1) && (sVar2 = (short)iVar4, -1 < sVar2)) &&
     (sVar2 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar5 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar2;
    sVar2 = *(short *)(iVar5 + *(int *)(DAT_0087a480 + 0x34));
    iVar5 = iVar5 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar2 != 0) && ((sVar6 = (short)((uint)iVar4 >> 0x10), sVar6 == 0 || (sVar2 == sVar6)))) {
      if (param_2 == '\x01') {
        *(uint *)(iVar5 + 0xec) = (uint)*(byte *)((int)param_1 + 5);
        pbVar7 = in_EAX;
        puVar8 = (undefined4 *)(iVar5 + 0xf0);
        for (iVar4 = 0xc; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar8 = *(undefined4 *)pbVar7;
          pbVar7 = pbVar7 + 4;
          puVar8 = puVar8 + 1;
        }
      }
      else {
        if ((uint)*(byte *)((int)param_1 + 5) != *(uint *)(iVar5 + 0xec)) {
          player_update_history_log_printf_filtered
                    (2,
                     "[%d]: Threw away remote player action update with base baseline, [%d] != [%d]."
                     ,*(undefined4 *)(DAT_006f1d6c + 0xc),(uint)*(byte *)((int)param_1 + 5),
                     *(uint *)(iVar5 + 0xec));
          return;
        }
        uVar3 = (uint)*(byte *)(param_1 + 1);
        iVar4 = *(int *)(iVar5 + 0xe8);
        if (iVar4 < (int)uVar3) {
          iVar4 = uVar3 - iVar4;
        }
        else {
          iVar4 = (uVar3 - iVar4) + 0x40;
        }
        if (0x1f < iVar4) {
          return;
        }
      }
      if (*(int *)(iVar5 + 0xe8) != -1) {
        fVar9 = (float10)fpatan((float10)*(float *)(in_EAX + 0x28),
                                (float10)*(float *)(in_EAX + 0x24));
        *(float *)(in_EAX + 8) = (float)fVar9;
        fVar9 = (float10)fpatan((float10)*(float *)(in_EAX + 0x2c),
                                SQRT((float10)*(float *)(in_EAX + 0x24) *
                                     (float10)*(float *)(in_EAX + 0x24) +
                                     (float10)*(float *)(in_EAX + 0x28) *
                                     (float10)*(float *)(in_EAX + 0x28)));
        *(float *)(in_EAX + 0xc) = (float)fVar9;
        in_EAX[0x20] = 0xff;
        in_EAX[0x21] = 0xff;
        player_update_history_log_printf_filtered(2,"Received action [%d]",(char)param_1[1]);
        local_28 = (uint)*in_EAX;
        puVar8 = local_20;
        for (iVar4 = 8; in_EAX = in_EAX + 4, iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar8 = *(undefined4 *)in_EAX;
          puVar8 = puVar8 + 1;
        }
        local_2c = (uint)*(byte *)(param_1 + 1);
        local_24 = local_28;
        cVar1 = circular_queue_push(&local_2c);
        if (cVar1 == '\0') {
          player_update_history_log_printf_filtered
                    (2,"[%d]: Remote player action_queue overflow.\n",
                     *(undefined4 *)(DAT_006f1d6c + 0xc));
        }
      }
      *(uint *)(iVar5 + 0xe8) = (uint)*(byte *)(param_1 + 1);
    }
  }
  return;
}
#endif
