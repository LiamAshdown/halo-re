// game_engine_init_player_look_state_from_object  (Ghidra:
// game_engine_init_player_look_state_from_object, already named)
// address 0x470e80, size 303 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: types/game.h local_player_control (all fields, matching game_engine_reset_player_
// look_state.c's own field mapping exactly); types/units.h unit_control_data::desired_
// facing_vector (0x224), desired_weapon_index (0x2f4), desired_grenade_index (0x31d),
// desired_zoom_level (0x321); types/objects.h object_header (stride 0x0c, object pointer +0x08);
// camera_observer_get_target_angles.c for the established atan2(y, x) mapping of fpatan.
// register convention: local-player index in AX (Ghidra's `in_AX`), unit handle in EDX (Ghidra's
// `in_EDX`).
//   // blam-cc: EDX -> unit, AX -> local_player_index
// UNSURE: the trailing `_isnan` calls compute a boolean that is never stored or branched on
// (Ghidra's own rendering shows the same); transcribed literally as dead diagnostic code rather
// than removed, since the task requires preserving every call and its order.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <string.h>

extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern data_array *object_headers;                          // 0x008603b0

extern double atan2(double y, double x); // x87 FPATAN
extern double sqrt(double x);            // x87 FSQRT

// blam-cc: EDX -> unit, AX -> local_player_index
// Zeroes and reinitializes local_player_index's look-state record exactly like
// game_engine_reset_player_look_state, but seeds `unit` into it and, when `unit` is valid,
// derives its initial yaw/pitch from the unit's own desired_facing_vector and copies its desired
// weapon/grenade/zoom indices.
void game_engine_init_player_look_state_from_object(datum_index unit, int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];

    memset(look, 0, sizeof(*look));
    look->unit = unit;
    look->desired_weapon_index = -1;
    look->desired_grenade_index = -1;
    look->desired_zoom_level = -1;
    look->autolevelling_active = 0;
    look->nameplate_target = k_datum_index_none;
    look->pitch_maximum = 1.4906585f;
    look->pitch_minimum = -1.4906585f;
    look->suppressed_buttons = 0;
    look->suppressed_until_released = 0;

    if (unit != k_datum_index_none) {
        unit_data *u = *(unit_data **)((uint8_t *)object_headers->data +
            (uint32_t)(uint16_t)unit * object_headers->size + 8);
        real horizontal;

        look->yaw = (real)atan2((double)u->desired_facing_vector.j, (double)u->desired_facing_vector.i);
        horizontal = (real)sqrt((double)(u->desired_facing_vector.i * u->desired_facing_vector.i +
            u->desired_facing_vector.j * u->desired_facing_vector.j));
        look->pitch = (real)atan2((double)u->desired_facing_vector.k, (double)horizontal);
        if (look->yaw < 0.0f) {
            look->yaw = look->yaw + 6.2831855f;
        }
        look->desired_weapon_index = u->desired_weapon_index;
        look->desired_grenade_index = (int16_t)u->desired_grenade_index;
        look->desired_zoom_level = (int16_t)u->desired_zoom_level;

        if (!_isnan((double)look->pitch) && look->pitch <= 1.4922565f && -1.4922565f <= look->pitch) {
            _isnan((double)look->yaw); // UNSURE: dead diagnostic, see header
        }
    }
}

#if 0
Original Ghidra decompilation (0x470e80), from tools/pack.py 0x470e80:

void game_engine_init_player_look_state_from_object(void)

{
  uint *puVar1;
  short in_AX;
  int iVar2;
  uint in_EDX;
  uint *puVar3;
  float10 fVar4;

  puVar1 = (uint *)(in_AX * 0x40 + 0x10 + DAT_006b145c);
  puVar3 = puVar1;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *puVar1 = in_EDX;
  *(undefined2 *)(puVar1 + 8) = 0xffff;
  *(undefined2 *)((int)puVar1 + 0x22) = 0xffff;
  *(undefined2 *)(puVar1 + 9) = 0xffff;
  *(undefined1 *)((int)puVar1 + 0x26) = 0;
  puVar1[10] = 0xffffffff;
  puVar1[0xf] = 0x3fbf0243;
  puVar1[0xe] = 0xbfbf0243;
  *(undefined2 *)(puVar1 + 2) = 0;
  *(undefined2 *)((int)puVar1 + 10) = 0;
  if (in_EDX != 0xffffffff) {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc);
    fVar4 = (float10)fpatan((float10)*(float *)(iVar2 + 0x228),(float10)*(float *)(iVar2 + 0x224));
    puVar1[3] = (uint)(float)fVar4;
    fVar4 = (float10)fpatan((float10)*(float *)(iVar2 + 0x22c),
                            SQRT((float10)*(float *)(iVar2 + 0x224) *
                                 (float10)*(float *)(iVar2 + 0x224) +
                                 (float10)*(float *)(iVar2 + 0x228) *
                                 (float10)*(float *)(iVar2 + 0x228)));
    puVar1[4] = (uint)(float)fVar4;
    if ((float)puVar1[3] < 0.0) {
      puVar1[3] = (uint)((float)puVar1[3] + 6.2831855);
    }
    *(undefined2 *)(puVar1 + 8) = *(undefined2 *)(iVar2 + 0x2f4);
    *(short *)((int)puVar1 + 0x22) = (short)*(char *)(iVar2 + 0x31d);
    *(short *)(puVar1 + 9) = (short)*(char *)(iVar2 + 0x321);
    iVar2 = __isnan((double)(float)puVar1[4]);
    if (((iVar2 == 0) && ((float)puVar1[4] < 1.4922565 != ((float)puVar1[4] == 1.4922565))) &&
       (-1.4922565 <= (float)puVar1[4])) {
      __isnan((double)(float)puVar1[3]);
    }
  }
  return;
}
#endif
