// dead_camera_new  (Ghidra: camera_shake_initialize; renamed, Blam-style, not previously named)
// address 0x4450e0, size 344 bytes (Ghidra's reported "size=344" swallows the alternate
//   epilogue at 0x445230..0x445239 that Ghidra mis-split off as its own function
//   "physical_memory_initialize"; that tail is this function's `param_1 == -1` return path and
//   is not a separate function -- see out/phase4/camera_types_notes.md "Misattributed" section)
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/camera_types_notes.md dead_camera_data section: every field this function
//   writes (focus, yaw, pitch, distance, fov, transition_time, local_player, target_player,
//   target_unit, retarget_time) matches types/camera.h dead_camera_data exactly, and it is the
//   only writer of the whole struct ("EAX = this"). The source observer_camera it reads its
//   initial focus from is `&observers[local_player_index].camera` (0x006ac6d0 + i*0x29c, i.e.
//   observer + 0x74), confirmed by the same 0x29c stride camera.h documents for `observer`.
// review fix (phase 4 gate, objdump 0x445212..0x44522a): with no explicit unit the camera
//   targets player +0x38 (player.previous_unit, the unit the player just lost), not +0x34
//   (player.unit, which is -1 for a dead player).
// register convention: this-pointer in EAX (in_EAX), local player index in DX (in_DX, 16-bit);
//   the target unit (or -1 to use the local player's own current unit) is the single cdecl
//   stack parameter. The function never touches EAX again after loading it, so it returns the
//   same this-pointer it was given (its only caller, camera_debug_compute_pov case 3, chains
//   the return value straight into camera_track_compute_pov).
//   // blam-cc: EAX -> this, DX -> local_player_index, stack -> unit
// reconciled: R04 0x006f1d20 void * current_game_engine -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"

extern observer observers[1];                // 0x006ac65c
extern random_seed effect_random_seed;        // 0x00719cd4, UNSURE: distinct from math's random_seed_global (0x00719cd0)
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480, stride 0x200 (no types/players.h yet)
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

// blam-cc: EAX -> this, DX -> local_player_index, stack -> unit
// Seeds a new dead (orbiting) camera target: starts the focus point at the current published
// camera position of `local_player_index` (or a null read if that index is invalid -- the
// compiled code does not guard against DX == -1 here), rolls a fresh random orbit distance,
// yaw and pitch, gives it a 3 second transition, and either follows `unit` directly (retargeting
// disabled, retarget_time = FLT_MAX) or, when unit == -1, follows the local player's own current
// unit with the normal 3/15 second retarget timer.
dead_camera_data *dead_camera_new(dead_camera_data *this, int16_t local_player_index, datum_index unit)
{
    observer_camera *source;
    datum_index local_player;

    source = (local_player_index != -1) ? &observers[local_player_index].camera : (observer_camera *)0;
    this->focus = *(Point3D *)&source->position; // UNSURE: null-dereferenced when local_player_index == -1

    this->field_of_view = 1.2217305f; // 70 degrees

    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    this->distance = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 4.0f + 2.0f;

    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    this->yaw = (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 6.2831855f;

    this->transition_time = 3.0f;

    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    this->pitch = -((real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f * 0.6283184f + 0.47123894f);

    if (unit != k_datum_index_none) {
        this->retarget_time = 3.4028235e38f; // FLT_MAX: an explicit target never auto-retargets
    } else {
        this->retarget_time = (current_game_engine != (game_engine_definition *)0) ? 15.0f : 3.0f;
    }

    if (local_player_index == -1 || local_player_index > 0) {
        local_player = k_datum_index_none; // UNSURE: dead code in this build (k_camera_local_player_count == 1)
    } else {
        local_player = local_player_globals->local_players[local_player_index];
    }
    this->local_player = local_player;

    if (unit == k_datum_index_none) {
        player *p = (player *)((uint8_t *)player_data->data + (local_player & 0xffff) * sizeof(player));
        this->target_unit = p->previous_unit; // player +0x38 (0x445226), the body just left behind
    } else {
        this->target_unit = unit;
    }
    this->target_player = local_player;

    return this;
}

#if 0
Original Ghidra decompilation (0x4450e0), size widened to include the mis-split tail at 0x445230
(Ghidra's "physical_memory_initialize"):

void camera_shake_initialize(int param_1)

{
  undefined4 uVar1;
  undefined4 *in_EAX;
  undefined4 *puVar2;
  uint uVar3;
  short in_DX;

  puVar2 = (undefined4 *)0x0;
  if (in_DX != -1) {
    puVar2 = &DAT_006ac6d0 + in_DX * 0xa7;
  }
  *in_EAX = *puVar2;
  in_EAX[1] = puVar2[1];
  in_EAX[2] = puVar2[2];
  in_EAX[6] = 0x3f9c61aa;
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  in_EAX[5] = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * 4.0 + 2.0;
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  in_EAX[3] = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 * 6.2831855;
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  uVar3 = DAT_00719cd4 >> 0x10;
  in_EAX[7] = 0x40400000;
  in_EAX[4] = -((float)uVar3 * 1.5259022e-05 * 0.6283184 + 0.47123894);
  if (param_1 == -1) {
    if (DAT_006f1d20 == 0) {
      uVar1 = 0x40400000;
    }
    else {
      uVar1 = 0x41700000;
    }
  }
  else {
    uVar1 = 0x7f7fffff;
  }
  in_EAX[0xb] = uVar1;
  if ((in_DX == -1) || (0 < in_DX)) {
    uVar3 = 0xffffffff;
  }
  else {
    uVar3 = *(uint *)(DAT_0087a478 + 4 + in_DX * 4);
  }
  in_EAX[8] = uVar3;
  if (param_1 == -1) {
    in_EAX[10] = *(undefined4 *)((uVar3 & 0xffff) * 0x200 + 0x38 + *(int *)(DAT_0087a480 + 0x34));
    in_EAX[9] = uVar3;
    return;
  }
  in_EAX[10] = param_1;
  in_EAX[9] = uVar3;
  return;
}
#endif
