// observer_update  (Ghidra: camera_shake_tick; renamed for this rewrite)
// address 0x447880, size 275 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// reviewed (phase 4 gate): objdump 0x447880..0x447992; matches; game_time +0x1c is game.h leftover_time.
// evidence: out/phase4/camera_types_notes.md ("camera_shake_tick: observer_update. Nothing in it
// is shake. It sets the command, advances the spline, commits, and adds a first person bob via
// 0x55cca0"); the byte it stamps at 0x006ac6cc is observer.updated (types/camera.h), matching the
// struct comment's own citation of this address (0x4478ae) as one of the two writers. Confirmed
// against objdump: the object_try_and_get call passes ECX = player.unit (loaded two instructions
// earlier and never overwritten), matching the established per-call-site convention; the boolean
// float comparison Ghidra renders as "a < 0.0 != (a == 0.0)" is algebraically a <= 0.0.
// register convention: dt and add_bob are on the stack (Ghidra's recognized param_1, param_2);
// no register-passed arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "game.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480, stride 0x200
extern game_time_globals *game_time;         // 0x006f1d6c

extern director directors[1];                // 0x006ac560
extern float observer_dt;                    // 0x006ac658
extern observer observers[1];                // 0x006ac65c

extern void camera_first_person_compute_pov(director_camera_data *data, camera_input *input,
    observer_command *command); // 0x446d60, this module

// blam-cc: ECX -> object_index, stack -> kind
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

// blam-cc: DX -> local_player_index (0x447ab0, this module)
extern void observer_set_command(int16_t local_player_index);

// blam-cc: DI -> local_player_index (0x447b50, this module)
extern void observer_advance(int16_t local_player_index);

// blam-cc: AX -> local_player_index (0x448900, this module)
extern void observer_commit(int16_t local_player_index);

extern uint32_t unit_predict_movement_delta(real_vector3d *out_position_delta,
    real_vector3d *out_forward_delta, real_vector3d *out_up_delta, float time_fraction);
    // 0x55cca0, units module

// Per-frame observer entry point: sets the observer's dt, applies its pending command (if any),
// advances the easing spline, commits the result to observers_camera[0], and (when add_bob is
// set, the local player is on foot in first person with no pending transition) adds a one-tick
// movement-prediction bob to the published camera position.
void observer_update(float dt, uint8_t add_bob)
{
    datum_index local_player;
    float time_fraction;
    real_vector3d position_delta;
    real_vector3d forward_delta;
    real_vector3d up_delta;

    observer_dt = dt;

    local_player = local_player_globals->local_players[0];
    if (local_player == (datum_index)k_datum_index_none) {
        return;
    }

    time_fraction = game_time->leftover_time;
    observers[0].updated = 1;
    observer_set_command(0);
    if (observer_dt != 0.0f) {
        observer_advance(0);
    }
    observer_commit(0);

    if (!add_bob) {
        return;
    }

    {
        player *p = &((player *)player_data->data)[local_player & 0xffff];
        if (p->unit != (datum_index)k_datum_index_none) {
            object *unit_object = object_try_and_get(p->unit, 3 /* biped | vehicle */);
            if (unit_object != 0 && unit_object->parent_object != (datum_index)k_datum_index_none) {
                return;
            }
        }
    }

    if (directors[0].pov_proc != camera_first_person_compute_pov ||
        !(directors[0].transition_time <= 0.0f)) {
        return;
    }

    if (unit_predict_movement_delta(&position_delta, &forward_delta, &up_delta, time_fraction) !=
        0) {
        observers[0].camera.position.x += position_delta.i;
        observers[0].camera.position.y += position_delta.j;
        observers[0].camera.position.z += position_delta.k;
    }
}

#if 0
Original Ghidra decompilation (0x447880):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void camera_shake_tick(float param_1,char param_2)

{
  uint uVar1;
  undefined4 uVar2;
  char cVar3;
  int iVar4;
  float local_24;
  float local_20;
  float local_1c;
  undefined1 local_18 [12];
  undefined1 local_c [12];

  _DAT_006ac658 = param_1;
  uVar1 = *(uint *)(DAT_0087a478 + 4);
  if (uVar1 != 0xffffffff) {
    uVar2 = *(undefined4 *)(DAT_006f1d6c + 0x1c);
    DAT_006ac6cc = 1;
    FUN_00447ab0();
    if (_DAT_006ac658 != 0.0) {
      FUN_00447b50();
    }
    FUN_00448900();
    if (param_2 != '\0') {
      if (((*(int *)((uVar1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) != -1) &&
          (iVar4 = object_try_and_get(3), iVar4 != 0)) && (*(int *)(iVar4 + 0x11c) != -1)) {
        return;
      }
      if (((DAT_006ac568 == camera_first_person_compute_pov) &&
          (DAT_006ac564 < 0.0 != (DAT_006ac564 == 0.0))) &&
         (cVar3 = FUN_0055cca0(&local_24,local_c,local_18,uVar2), cVar3 != '\0')) {
        DAT_006ac6d0 = DAT_006ac6d0 + local_24;
        DAT_006ac6d4 = DAT_006ac6d4 + local_20;
        DAT_006ac6d8 = DAT_006ac6d8 + local_1c;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
