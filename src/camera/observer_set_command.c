// observer_set_command  (Ghidra: FUN_00447ab0; renamed for this rewrite)
// address 0x447ab0, size 155 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// reviewed (phase 4 gate): objdump 0x447ab0..0x447b4a; the channel time merge and the 2.0 clamp match.
// evidence: out/phase4/camera_types_notes.md ("observer_set_command (0x447ab0) copies it into
// the observer at +0x08"); field offsets (+0x04 command, +0x48 timer, +0x4c interpolation_flags,
// +0x54 channel_times relative to the command, +0x5c relative to the observer for the
// current_command copy) match types/camera.h's observer / observer_command layout exactly. The
// function reads its target through observers[i].command (set elsewhere, e.g. by camera_update),
// so it takes no separate command argument.
// register convention: local player index in DX (in_DX); no other parameters.

#include "tags.h"
#include "memory.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern observer observers[1]; // 0x006ac65c

// blam-cc: DX -> local_player_index
// Blends the director's new command into observers[local_player_index]: for each of the five
// observer_parameter channels, normalises the target time into command->channel_times[i] (either
// the shared timer, or the channel's own time, never shortening an in-progress ease unless the
// command snaps), clamps it to 2 seconds, and then -- only if the command carries the valid bit
// -- copies the whole (now-normalised) command into the observer's current_command.
void observer_set_command(int16_t local_player_index)
{
    observer *self = &observers[local_player_index];
    observer_command *command = self->command;
    int32_t i;

    if ((command->flags & _observer_command_valid_bit) == 0) {
        return;
    }

    for (i = 0; i < k_observer_parameter_count; i++) {
        float clamp_source;
        uint8_t use_clamp = 0;

        if ((command->interpolation_flags[i] & _observer_interpolation_own_time_bit) == 0) {
            if (command->timer < self->current_command.channel_times[i] &&
                (command->flags & _observer_command_snap_bit) == 0) {
                clamp_source = self->current_command.channel_times[i];
                use_clamp = 1;
            } else {
                command->channel_times[i] = command->timer;
            }
        } else if ((command->interpolation_flags[i] & _observer_interpolation_exact_bit) == 0 &&
            command->channel_times[i] < self->current_command.channel_times[i]) {
            clamp_source = self->current_command.channel_times[i];
            use_clamp = 1;
        }

        if (use_clamp) {
            command->channel_times[i] = (clamp_source <= 2.0f) ? clamp_source : 2.0f;
        }
    }

    self->current_command = *command;
}

#if 0
Original Ghidra decompilation (0x447ab0):

void FUN_00447ab0(void)

{
  byte *pbVar1;
  float *pfVar2;
  short in_DX;
  int iVar3;
  byte *pbVar4;
  int iVar5;
  undefined4 *puVar6;
  float *pfVar7;
  undefined4 *puVar8;

  iVar3 = in_DX * 0x29c;
  pbVar1 = *(byte **)(&DAT_006ac660 + iVar3);
  pfVar7 = (float *)(pbVar1 + 0x54);
  pfVar2 = (float *)(&DAT_006ac6b8 + iVar3);
  pbVar4 = pbVar1 + 0x4c;
  if ((*pbVar1 & 1) != 0) {
    iVar5 = 5;
    do {
      if ((*pbVar4 & 1) == 0) {
        pbVar1 = *(byte **)(&DAT_006ac660 + iVar3);
        if ((*(float *)(pbVar1 + 0x48) < *pfVar2) && ((*pbVar1 & 8) == 0)) goto LAB_00447b11;
        *pfVar7 = *(float *)(pbVar1 + 0x48);
      }
      else if (((*pbVar4 & 2) == 0) && (*pfVar7 < *pfVar2)) {
LAB_00447b11:
        if (*pfVar2 <= 2.0) {
          *pfVar7 = *pfVar2;
        }
        else {
          *pfVar7 = 2.0;
        }
      }
      pfVar7 = pfVar7 + 1;
      pfVar2 = pfVar2 + 1;
      pbVar4 = pbVar4 + 1;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
    puVar6 = *(undefined4 **)(&DAT_006ac660 + iVar3);
    puVar8 = (undefined4 *)(&DAT_006ac664 + iVar3);
    for (iVar5 = 0x1a; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar8 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar8 = puVar8 + 1;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
