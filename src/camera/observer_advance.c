// observer_advance  (Ghidra: FUN_00447b50; renamed for this rewrite)
// address 0x447b50, size 124 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// reviewed (phase 4 gate): objdump 0x447b50..0x447bd6; call order and register arguments match.
// evidence: out/phase4/camera_types_notes.md's proposed name; confirmed instruction-by-instruction
// against objdump, which also resolves the exact register/pointer setup for its five callees
// (observer_compute_remaining_offset, observer_compute_spline_coefficients,
// observer_evaluate_spline_acceleration, observer_evaluate_spline_velocity,
// observer_evaluate_spline_value_and_orthonormalize -- all this module) and confirms the guard
// is observer_command_frozen_bit on the observer's own command pointer.
// register convention: local player index in DI (unaff_DI); no other parameters.

#include "tags.h"
#include "memory.h"
#include "camera.h"

extern observer observers[1]; // 0x006ac65c
extern float observer_dt;     // 0x006ac658

// blam-cc: EAX -> target, ECX -> current, EDX -> out (this module)
extern void observer_compute_remaining_offset(float *target, float *current, float *out);
// blam-cc: EDX -> local_player_index (this module)
extern void observer_compute_spline_coefficients(int16_t local_player_index);
// blam-cc: EAX -> local_player_index (this module)
extern void observer_evaluate_spline_acceleration(int16_t local_player_index);
// blam-cc: EAX -> local_player_index (this module)
extern void observer_evaluate_spline_velocity(int16_t local_player_index);
// blam-cc: EAX -> local_player_index (this module)
extern void observer_evaluate_spline_value_and_orthonormalize(int16_t local_player_index);

// blam-cc: DI -> local_player_index
// Re-fits the observer's quintic spline from its current remaining offset/velocity/acceleration
// (unless the observer's command is frozen), evaluates the new value/velocity/acceleration, and
// ticks every channel's remaining time down by dt (floored at 0).
void observer_advance(int16_t local_player_index)
{
    observer *o = &observers[local_player_index];
    int32_t i;

    if ((o->command->flags & _observer_command_frozen_bit) != 0) {
        return;
    }

    observer_compute_remaining_offset((float *)&o->current_command.parameters,
        (float *)&o->parameters, (float *)&o->remaining_offset);
    observer_compute_spline_coefficients(local_player_index);
    observer_evaluate_spline_acceleration(local_player_index);
    observer_evaluate_spline_velocity(local_player_index);
    observer_evaluate_spline_value_and_orthonormalize(local_player_index);

    for (i = 0; i < k_observer_parameter_count; i++) {
        float t = o->current_command.channel_times[i] - observer_dt;

        if (t <= 0.0f) {
            t = 0.0f;
        }
        o->current_command.channel_times[i] = t;
    }
}

#if 0
Original Ghidra decompilation (0x447b50):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00447b50(void)

{
  float fVar1;
  int iVar2;
  float *pfVar3;
  short unaff_DI;

  pfVar3 = (float *)(&DAT_006ac6b8 + unaff_DI * 0x29c);
  if ((**(byte **)(&DAT_006ac660 + unaff_DI * 0x29c) & 0x20) == 0) {
    FUN_00448710();
    FUN_00447be0();
    FUN_00447e40();
    FUN_00448010();
    FUN_00448210();
    iVar2 = 5;
    do {
      fVar1 = *pfVar3 - _DAT_006ac658;
      if (fVar1 <= 0.0) {
        fVar1 = 0.0;
      }
      *pfVar3 = fVar1;
      pfVar3 = pfVar3 + 1;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
  }
  return;
}
#endif
