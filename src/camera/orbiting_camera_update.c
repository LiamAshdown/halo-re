// orbiting_camera_update  (Ghidra: FUN_00446870; renamed)
// address 0x446870, size 303 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: flying_camera_update_procs[1] (.data 0x00686aac = 0x446870), the "orbiting camera"
//   entry of 0x00686ac0 (out/phase4/camera_types_notes.md: "the orbiting sub-mode update (not
//   a spectator camera)"). Orbits the first person camera position of the local player's unit
//   (chimera__spectate_fp_camera_position 0x472020) at orbiting_camera_data.distance, yaw and
//   pitch (clamped to +-1.2566371 at 0x00673010 / 0x00673014); zoom pulls the distance in by
//   zoom / 3, floored at 0.6.
// register convention: director_pov_proc (cdecl, three stack arguments; ebx = [esp+0x2c] data,
//   edi = [esp+0x30] input, ebp = [esp+0x34] command after the four pushes).
//   // blam-cc: stack -> (data, input, command)
// objdump for the calls Ghidra left without arguments:
//   0x44687b..0x446882  AX = input->local_player_index, ESI = &basis (stack) -> 0x472020
//   0x44692b..0x446950  ESI = &command->forward, EDI = &command->up -> 0x4479c0
//   0x446955..0x44695e  EAX = basis.unit, ESI = &command->velocity, EDI = 0 -> 0x4f6aa0
// The command is only marked valid (flags = 1) when the player has a unit; the focus offset,
// distance, fov (70 degrees) and timer (0.5) are written either way.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"

extern director directors[1];                           // 0x006ac560
extern const real_point3d *global_origin3d_pointer;     // 0x00696714, math module

extern double cos(double x);
extern double sin(double x);

// blam-cc: AX -> local_player_index, ESI -> out
extern void chimera__spectate_fp_camera_position(camera_basis_out *out,
    int16_t local_player_index);                        // 0x472020, game module
// blam-cc: ESI -> forward, EDI -> out_up
extern void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *out_up); // 0x4479c0, this module
// blam-cc: EAX -> object_index, ESI -> out_velocity, EDI -> out_angular_velocity (may be NULL)
extern void object_get_root_object_velocities(datum_index object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity);               // 0x4f6aa0, objects module

// blam-cc: stack -> (data, input, command)
void orbiting_camera_update(director_camera_data *data, camera_input *input, observer_command *command)
{
    orbiting_camera_data *orbit = &data->orbiting;
    camera_basis_out basis;
    float distance;

    chimera__spectate_fp_camera_position(&basis, input->local_player_index);
    command->parameters.position = *(Point3D *)&basis.position;

    if (input->has_look_input) {
        float pitch;

        orbit->yaw = input->yaw_delta + orbit->yaw;
        pitch = input->pitch_delta + orbit->pitch;
        if (pitch < -1.2566371f) {
            pitch = -1.2566371f;
        } else if (pitch > 1.2566371f) {
            pitch = 1.2566371f;
        }
        orbit->pitch = pitch;
        directors[input->local_player_index].look_input_consumed = 1;
    }

    distance = orbit->distance - input->zoom_delta * 0.33333334f;
    if (distance <= 0.6f) {
        distance = 0.6f;
    }
    orbit->distance = distance;

    if (basis.unit != k_datum_index_none) {
        float cos_pitch = (float)cos(orbit->pitch);

        command->parameters.forward.i = (float)cos(orbit->yaw) * cos_pitch;
        command->parameters.forward.j = (float)sin(orbit->yaw) * cos_pitch;
        command->parameters.forward.k = (float)sin(orbit->pitch);
        vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);
        object_get_root_object_velocities(basis.unit, (real_vector3d *)&command->velocity, 0);
        command->flags = _observer_command_valid_bit;
    }

    command->parameters.focus_offset = *(const Vector3D *)global_origin3d_pointer;
    command->parameters.distance = orbit->distance;
    command->parameters.field_of_view = 1.2217305f; // 70 degrees (0x3f9c61aa)
    command->timer = 0.5f;
}

#if 0
Original Ghidra decompilation (0x446870):

void FUN_00446870(int param_1,short *param_2,undefined4 *param_3)

{
  float fVar1;
  undefined *puVar2;
  float10 fVar3;
  float10 fVar4;
  int local_18;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  chimera__spectate_fp_camera_position();
  param_3[1] = local_c;
  param_3[2] = local_8;
  param_3[3] = local_4;
  if ((char)param_2[1] != '\0') {
    *(float *)(param_1 + 0xc) = *(float *)(param_2 + 4) + *(float *)(param_1 + 0xc);
    fVar1 = *(float *)(param_2 + 6) + *(float *)(param_1 + 0x10);
    if (-1.2566371 <= fVar1) {
      if (1.2566371 < fVar1) {
        fVar1 = 1.2566371;
      }
    }
    else {
      fVar1 = -1.2566371;
    }
    *(float *)(param_1 + 0x10) = fVar1;
    (&DAT_006ac5b2)[*param_2 * 0xf8] = 1;
  }
  fVar1 = *(float *)(param_1 + 4) - *(float *)(param_2 + 0x10) * 0.33333334;
  if (fVar1 <= 0.6) {
    fVar1 = 0.6;
  }
  *(float *)(param_1 + 4) = fVar1;
  if (local_18 != -1) {
    fVar3 = (float10)fcos((float10)*(float *)(param_1 + 0x10));
    fVar4 = (float10)fcos((float10)*(float *)(param_1 + 0xc));
    param_3[9] = (float)(fVar4 * fVar3);
    fVar4 = (float10)fsin((float10)*(float *)(param_1 + 0xc));
    param_3[10] = (float)(fVar4 * fVar3);
    fVar3 = (float10)fsin((float10)*(float *)(param_1 + 0x10));
    param_3[0xb] = (float)fVar3;
    FUN_004479c0();
    FUN_004f6aa0();
    *param_3 = 1;
  }
  puVar2 = PTR_DAT_00696714;
  param_3[4] = *(undefined4 *)PTR_DAT_00696714;
  param_3[5] = *(undefined4 *)(puVar2 + 4);
  param_3[6] = *(undefined4 *)(puVar2 + 8);
  param_3[7] = *(undefined4 *)(param_1 + 4);
  param_3[8] = 0x3f9c61aa;
  param_3[0x12] = 0x3f000000;
  return;
}
#endif
