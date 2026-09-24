// camera_track_compute_pov  (Ghidra: camera_track_compute_pov, already named)
// address 0x445380, size 476 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// reviewed (phase 4 gate): objdump 0x445380..0x44555b; matches (teammate helper result is now a byte).
// evidence: out/phase4/camera_types_notes.md calls this the dead camera's pov procedure
//   (director.pov_proc == 0x445380 for director_camera_type_other / dead_camera_data). It reads
//   and writes exactly the dead_camera_data fields types/camera.h documents (focus, yaw, pitch,
//   distance, transition_time, local_player, target_player, target_unit, retarget_time) and
//   produces an observer_command with the layout camera.h's observer_parameters section
//   documents. Confirmed against objdump: object_try_and_get's object-handle argument travels
//   in ECX (elided by Ghidra's decompiler, which shows only the visible `push -1` type-mask
//   argument), and the target unit's world position is object+0xa0 (types/objects.h
//   `object.bounding_center`).
// register convention: matches director_pov_proc exactly -- (director_camera_data *data,
//   camera_input *input, observer_command *command), cdecl, all three on the stack. This
//   function only reads `data` (as `&data->dead`); it never touches the pov-switch (mode) field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "camera.h"

extern data_array *player_data;                    // 0x0087a480, stride 0x200 (no types/players.h yet)
extern game_time_globals *game_time;               // 0x006f1d6c
extern void *current_game_engine;                  // 0x006f1d20, game_engine_definition *; non-NULL means multiplayer
extern const real_point3d *global_origin3d_pointer; // 0x00696714

// cos/sin are single x87 FCOS/FSIN instructions in the original code; declared locally instead
// of via <math.h> because -I types shadows that header name with types/math.h.
extern double cos(double x);
extern double sin(double x);

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, objects module
extern void *datum_get(datum_index handle, data_array *array);                  // 0x4d0680, memory module
extern void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *out_up); // 0x4479c0, this module
                                                     // (rewritten separately); blam-cc: ESI -> forward, EDI -> out_up
extern uint8_t camera_dead_player_has_teammate(datum_index reference_player);               // this module
extern datum_index camera_dead_find_next_teammate(datum_index reference_player, datum_index current_target, uint8_t require_same_team); // this module

// blam-cc: stack -> (data, input, command)
// The dead (orbiting) camera's point of view: focuses on the tracked target unit's bounding
// center (or, once no unit can be resolved, the last observer position captured at
// dead_camera_new time), derives a forward vector from the accumulated yaw/pitch, decays the
// initial-approach distance towards its steady-state value over the transition, and every
// `retarget_time` seconds looks for a different living teammate to follow.
void camera_track_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    dead_camera_data *dead = &data->dead;
    Point3D focus_position;

    if (dead->target_unit != k_datum_index_none) {
        object *target = object_try_and_get(dead->target_unit, 0xffffffff);
        if (target != (object *)0) {
            focus_position = *(Point3D *)&target->bounding_center;
        } else {
            focus_position = dead->focus;
        }
    } else {
        focus_position = dead->focus;
    }

    command->parameters.position = focus_position;
    command->parameters.distance = dead->distance;

    command->parameters.forward.i = (real)cos((double)dead->pitch) * (real)cos((double)dead->yaw);
    command->parameters.forward.j = (real)cos((double)dead->pitch) * (real)sin((double)dead->yaw);
    command->parameters.forward.k = (real)sin((double)dead->pitch);

    vector3d_compute_up_from_forward((Vector3D *)&command->parameters.forward, (Vector3D *)&command->parameters.up);
    command->parameters.field_of_view = dead->field_of_view;
    command->parameters.focus_offset = *(Vector3D *)global_origin3d_pointer;
    command->velocity = *(Vector3D *)global_origin3d_pointer;

    command->flags = 1; // valid bit

    command->timer = (dead->transition_time >= 0.0f) ? dead->transition_time : 0.0f;

    command->interpolation_flags[_observer_parameter_position] = 3;
    command->channel_times[_observer_parameter_position] = 0.0f;

    if (dead->transition_time == 3.0f) {
        // Brand new target (dead_camera_new just set transition_time to exactly 3.0): snap the
        // distance channel too, starting the eased distance from 0.5 instead of wherever it was.
        command->parameters.distance = 0.5f;
        command->interpolation_flags[_observer_parameter_distance] = 3;
        command->channel_times[_observer_parameter_distance] = 0.0f;
    }

    dead->transition_time -= input->dt;
    dead->retarget_time -= input->dt;
    if (dead->retarget_time < 0.0f) {
        dead->retarget_time = 0.0f;
    }

    if (dead->retarget_time == 0.0f && game_time->paused == 0) {
        uint8_t has_teammate = camera_dead_player_has_teammate(dead->local_player);
        datum_index new_target = camera_dead_find_next_teammate(dead->local_player, dead->target_player,
                                                                  (uint8_t)has_teammate);
        datum_index new_unit = k_datum_index_none;

        dead->target_player = new_target;
        if (new_target != k_datum_index_none) {
            player *p = (player *)datum_get(new_target, player_data);
            if (p == (player *)0) {
                dead->target_player = dead->local_player;
            }
            p = (player *)((uint8_t *)player_data->data + (dead->target_player & 0xffff) * sizeof(player));
            new_unit = p->unit;
        }

        if (new_unit != dead->target_unit && new_unit != k_datum_index_none) {
            dead->transition_time = 3.0f;
            dead->target_unit = new_unit;
        }

        dead->retarget_time = (current_game_engine != (void *)0) ? 15.0f : 3.0f;
    }
}

#if 0
Original Ghidra decompilation (0x445380):

void camera_track_compute_pov(undefined4 *param_1,int param_2,undefined4 *param_3)

{
  undefined4 uVar1;
  float fVar2;
  undefined *puVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  float10 fVar7;
  float10 fVar8;

  if (param_1[10] != -1) {
    iVar5 = object_try_and_get(0xffffffff);
    if (iVar5 != 0) {
      param_3[1] = *(undefined4 *)(iVar5 + 0xa0);
      param_3[2] = *(undefined4 *)(iVar5 + 0xa4);
      param_3[3] = *(undefined4 *)(iVar5 + 0xa8);
      goto LAB_004453d5;
    }
  }
  param_3[1] = *param_1;
  param_3[2] = param_1[1];
  param_3[3] = param_1[2];
LAB_004453d5:
  param_3[7] = param_1[5];
  fVar7 = (float10)fcos((float10)(float)param_1[4]);
  fVar8 = (float10)fcos((float10)(float)param_1[3]);
  param_3[9] = (float)(fVar8 * fVar7);
  fVar8 = (float10)fsin((float10)(float)param_1[3]);
  param_3[10] = (float)(fVar8 * fVar7);
  fVar7 = (float10)fsin((float10)(float)param_1[4]);
  param_3[0xb] = (float)fVar7;
  FUN_004479c0();
  puVar3 = PTR_DAT_00696714;
  param_3[8] = param_1[6];
  param_3[4] = *(undefined4 *)puVar3;
  param_3[5] = *(undefined4 *)(puVar3 + 4);
  param_3[6] = *(undefined4 *)(puVar3 + 8);
  param_3[0xf] = *(undefined4 *)puVar3;
  param_3[0x10] = *(undefined4 *)(puVar3 + 4);
  param_3[0x11] = *(undefined4 *)(puVar3 + 8);
  *param_3 = 1;
  if (0.0 <= (float)param_1[7]) {
    uVar1 = param_1[7];
  }
  else {
    uVar1 = 0;
  }
  param_3[0x12] = uVar1;
  param_3[0x15] = 0;
  *(undefined1 *)(param_3 + 0x13) = 3;
  if (param_1[7] == 0x40400000) {
    param_3[7] = 0x3f000000;
    param_3[0x17] = 0;
    *(undefined1 *)((int)param_3 + 0x4e) = 3;
  }
  param_1[7] = (float)param_1[7] - *(float *)(param_2 + 4);
  if (0.0 <= (float)param_1[0xb] - *(float *)(param_2 + 4)) {
    fVar2 = (float)param_1[0xb] - *(float *)(param_2 + 4);
  }
  else {
    fVar2 = 0.0;
  }
  param_1[0xb] = fVar2;
  if ((fVar2 == 0.0) && (*(char *)(DAT_006f1d6c + 2) == '\0')) {
    iVar5 = -1;
    FUN_00445240(param_1[8]);
    iVar6 = FUN_004452c0(param_1[8],param_1[9]);
    param_1[9] = iVar6;
    iVar4 = DAT_0087a480;
    if (iVar6 != -1) {
      iVar5 = datum_get();
      if (iVar5 == 0) {
        param_1[9] = param_1[8];
      }
      iVar5 = *(int *)((param_1[9] & 0xffff) * 0x200 + 0x34 + *(int *)(iVar4 + 0x34));
    }
    if ((iVar5 != param_1[10]) && (iVar5 != -1)) {
      param_1[7] = 0x40400000;
      param_1[10] = iVar5;
    }
    if (DAT_006f1d20 != 0) {
      param_1[0xb] = 0x41700000;
      return;
    }
    param_1[0xb] = 0x40400000;
  }
  return;
}

Disassembly excerpt (objdump -d -M intel) confirming param_3[7] (parameters.distance) is written
TWICE (once unconditionally from transition_time via the 0.0-clamp, once forced to 0.5 on a
brand new target) and that the flags/channel writes at param_3[0x15]/[0x13] and [0x17]/+0x4e are
the POSITION and DISTANCE channels channel_time/interpolation_flags respectively (not the
literal decompiled indices, which mislabel which struct field is being touched byte-for-byte):

0x445447: fcomp dword ptr [ebp + 0x1c]        ; ebp = dead_camera_data pointer (this function's "param_1")
0x445461: mov cl, 3
0x445463: mov dword ptr [ebx + 0x54], eax      ; command->channel_times[0] (position) = 0
0x445466: mov byte ptr [ebx + 0x4c], cl        ; command->interpolation_flags[0] (position) = 3
0x445469: mov edx, dword ptr [ebp + 0x1c]      ; transition_time (unclamped)
0x445471: cmp edx, edi                          ; edi == 0x40400000 (3.0f)
0x445475: mov dword ptr [ebx + 0x1c], 0x3f000000  ; command->parameters.distance = 0.5f
0x44547c: mov dword ptr [ebx + 0x5c], eax      ; command->channel_times[2] (distance) = 0
0x44547f: mov byte ptr [ebx + 0x4e], cl        ; command->interpolation_flags[2] (distance) = 3
#endif
