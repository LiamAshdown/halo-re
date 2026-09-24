// game_engine_compute_look_angles_from_vector  (Ghidra: game_engine_compute_look_angles_from_vector,
// already named)
// address 0x470d80, size 90 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/game.h local_player_control (yaw +0x0c, pitch +0x10), player_control_globals
// (local_players at +0x10); src/game/camera_observer_get_target_angles.c for the established
// atan2(y, x) mapping of Ghidra's fpatan and the identical yaw/pitch-from-vector formula.
// register convention: the direction vector arrives in EAX (Ghidra's `in_EAX`, a real_vector3d*);
// the local-player index arrives in CX (Ghidra's `in_CX`).
//   // blam-cc: EAX -> facing, CX -> local_player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern player_control_globals *player_control_globals_ptr; // 0x006b145c

extern double atan2(double y, double x); // x87 FPATAN
extern double sqrt(double x);            // x87 FSQRT

// blam-cc: EAX -> facing, CX -> local_player_index
// Derives yaw and pitch from `facing` and stores them into the given local player's look-state
// record, wrapping yaw into [0, 2*pi).
void game_engine_compute_look_angles_from_vector(real_vector3d *facing, int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    real horizontal;

    look->yaw = (real)atan2((double)facing->j, (double)facing->i);
    horizontal = (real)sqrt((double)(facing->i * facing->i + facing->j * facing->j));
    look->pitch = (real)atan2((double)facing->k, (double)horizontal);
    if (look->yaw < 0.0f) {
        look->yaw = look->yaw + 6.2831855f;
    }
}

#if 0
Original Ghidra decompilation (0x470d80), from tools/pack.py 0x470d80:

void game_engine_compute_look_angles_from_vector(void)

{
  int iVar1;
  float *in_EAX;
  short in_CX;
  float10 fVar2;

  fVar2 = (float10)fpatan((float10)in_EAX[1],(float10)*in_EAX);
  iVar1 = in_CX * 0x40 + 0x10 + DAT_006b145c;
  *(float *)(iVar1 + 0xc) = (float)fVar2;
  fVar2 = (float10)fpatan((float10)in_EAX[2],
                          SQRT((float10)*in_EAX * (float10)*in_EAX +
                               (float10)in_EAX[1] * (float10)in_EAX[1]));
  *(float *)(iVar1 + 0x10) = (float)fVar2;
  if (*(float *)(iVar1 + 0xc) < 0.0) {
    *(float *)(iVar1 + 0xc) = *(float *)(iVar1 + 0xc) + 6.2831855;
  }
  return;
}
#endif
