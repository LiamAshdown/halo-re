// camera_third_person_compute_pov  (Ghidra: camera_third_person_compute_pov, already named)
// address 0x447370, size 770 bytes
// name confidence: 0.85   rewrite confidence: 0.8
// evidence: matches director_pov_proc's signature (types/camera.h) and third_person_camera_data /
// observer_command's field layout field-for-field. Ghidra's own decompile is structurally sound
// (control flow and arithmetic verified against objdump) but could not attribute several locals
// to their real source; confirmed instruction-by-instruction: the hidden
// chimera__spectate_fp_camera_position call fills a camera_basis_out on this function's own
// stack (ESI -> that struct, AX -> input->local_player_index, matching the established
// convention), and the two extra pov-procedure parameters (a second EAX-load right before the
// object_get_root_object_velocities call, and ESI/ECX carrying camera_basis_out.marker_offset
// into first_person_camera_track_offset as its properties argument) are both confirmed live-in
// values, not stack parameters.
// review (phase 4 gate, line by line against objdump 0x447370..0x447671): matches, including
// the channel time floors (0.5 / 0.4 via 0x00672abc / 0x00672bc8), the +-pi/2 pitch clamp, the
// track offset written over the yaw / pitch stack slots, and the 0.6 distance floor.
// register convention: all three parameters are on the stack (cdecl, matching director_pov_proc);
// no register-passed arguments of its own.
//
// UNSURE: camera_basis_out.marker_offset (types/game.h, itself marked UNSURE there) is used here
// as a unit_camera_properties* pointer (passed straight to first_person_camera_track_offset's ECX
// argument) whenever it is non-NULL, which suggests chimera__spectate_fp_camera_position actually
// returns the seat/unit camera properties pointer, not a raw marker offset; not changed here
// since types/game.h is owned by another module's rewrite.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;                             // 0x008603b0
extern player_control_globals *player_control_globals_ptr;  // 0x006b145c

extern double fcos(double angle); // FCOS
extern double fsin(double angle); // FSIN
extern double sqrt(double x);     // FSQRT

// blam-cc: ESI -> out, AX -> local_player_index
extern void chimera__spectate_fp_camera_position(camera_basis_out *out,
    int16_t local_player_index);

// blam-cc: ECX -> properties; angle, out = stack
extern void first_person_camera_track_offset(unit_camera_properties *properties, float angle,
    Vector3D *out);

// blam-cc: ESI -> forward, EDI -> up
extern void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *up);

extern void object_get_root_object_velocities(datum_index object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, objects module

// blam-cc: stack -> data, input, command (director_pov_proc)
// Third person (chase) camera: orbits the unit at the player's look angles (clamped to +-90
// degrees of pitch), offsets the focus point by the unit's camera track sample (recoil/sway) and
// crouch/jump state, and derives distance from the track offset's magnitude and the player's zoom
// input.
void camera_third_person_compute_pov(director_camera_data *data, camera_input *input,
    observer_command *command)
{
    third_person_camera_data *tp = &data->third_person;
    camera_basis_out basis;
    object *unit_object;
    uint8_t crouch_or_jump;
    float yaw, pitch;
    float cos_pitch, sin_pitch;
    Vector3D track_offset;
    float track_magnitude;
    local_player_control *player;

    chimera__spectate_fp_camera_position(&basis, input->local_player_index);

    *(real_point3d *)&command->parameters.position = basis.position;
    command->timer = 0.0f;
    command->flags = 0;
    command->parameters.field_of_view = 1.2217305f; // 70 degrees

    if (tp->initialized != 0 &&
        (basis.unit != tp->unit || basis.seat_index != tp->seat_index)) {
        command->timer = 1.0f;
    }
    tp->unit = basis.unit;
    tp->seat_index = basis.seat_index;

    if (basis.marker_offset != 0) {
        unit_object = ((object_header *)object_data->data)[basis.unit & 0xffff].data;
        crouch_or_jump = (uint8_t)((((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))
            ->control_flags & 3) != 0);

        if (crouch_or_jump != tp->crouch_or_jump) {
            command->interpolation_flags[_observer_parameter_focus_offset] = 1;
            if (command->channel_times[_observer_parameter_focus_offset] < 0.5f) {
                command->channel_times[_observer_parameter_focus_offset] = 0.5f;
            }
            tp->crouch_or_jump = crouch_or_jump;
        }

        if (!input->has_look_input) {
            if (tp->yaw_offset != 0.0f || tp->pitch_offset != 0.0f) {
                tp->yaw_offset = 0.0f;
                tp->pitch_offset = 0.0f;
            }
        } else {
            tp->yaw_offset = input->yaw_delta + tp->yaw_offset;
            tp->pitch_offset = input->pitch_delta + tp->pitch_offset;
            command->interpolation_flags[_observer_parameter_orientation] = 1;
            if (command->channel_times[_observer_parameter_orientation] < 0.4f) {
                command->channel_times[_observer_parameter_orientation] = 0.4f;
            }
        }

        {
            float distance_scale = tp->distance_scale - input->zoom_delta * 0.05f;
            if (distance_scale < 0.0f) {
                distance_scale = 0.0f;
            } else if (distance_scale > 5.0f) {
                distance_scale = 5.0f;
            }
            tp->distance_scale = distance_scale;
        }
        if (input->zoom_delta != 0.0f) {
            command->interpolation_flags[_observer_parameter_distance] = 1;
            if (command->channel_times[_observer_parameter_distance] < 0.4f) {
                command->channel_times[_observer_parameter_distance] = 0.4f;
            }
            command->interpolation_flags[_observer_parameter_focus_offset] = 1;
            if (command->channel_times[_observer_parameter_focus_offset] < 0.4f) {
                command->channel_times[_observer_parameter_focus_offset] = 0.4f;
            }
        }

        player = &player_control_globals_ptr->local_players[input->local_player_index];
        yaw = player->yaw + tp->yaw_offset;
        pitch = player->pitch + tp->pitch_offset;
        if (pitch < -1.5707964f) {
            pitch = -1.5707964f;
        } else if (pitch > 1.5707964f) {
            pitch = 1.5707964f;
        }

        cos_pitch = (float)fcos((double)pitch);
        sin_pitch = (float)fsin((double)pitch);

        command->parameters.forward.i = (float)fcos((double)yaw) * cos_pitch;
        command->parameters.forward.j = (float)fsin((double)yaw) * cos_pitch;
        command->parameters.forward.k = sin_pitch;

        first_person_camera_track_offset((unit_camera_properties *)basis.marker_offset, pitch,
            &track_offset);

        track_magnitude = (float)sqrt((double)(track_offset.i * track_offset.i +
            track_offset.j * track_offset.j + track_offset.k * track_offset.k));

        command->parameters.focus_offset.i =
            (cos_pitch * track_magnitude + track_offset.i) * tp->distance_scale;
        command->parameters.focus_offset.j = -(track_offset.j * tp->distance_scale);
        command->parameters.focus_offset.k =
            (sin_pitch * track_magnitude + track_offset.k) * tp->distance_scale;

        {
            float distance = (track_magnitude - 0.6f) * tp->distance_scale + 0.6f;
            if (distance <= 0.6f) {
                distance = 0.6f;
            }
            command->parameters.distance = distance;
        }

        object_get_root_object_velocities(basis.unit, (real_vector3d *)&command->velocity, 0);
        command->flags |= _observer_command_valid_bit;
    }

    vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);
    tp->initialized = 1;
}

#if 0
Original Ghidra decompilation (0x447370):

void camera_third_person_compute_pov(char *param_1,short *param_2,uint *param_3)

{
  int iVar1;
  uint uVar2;
  float fVar3;
  bool bVar4;
  float10 fVar5;
  float10 fVar6;
  float local_24;
  float local_20;
  float local_1c;
  uint local_18;
  short local_14;
  int local_10;
  uint local_c;
  uint local_8;
  uint local_4;

  chimera__spectate_fp_camera_position();
  param_3[1] = local_c;
  param_3[2] = local_8;
  param_3[3] = local_4;
  param_3[0x12] = 0;
  *param_3 = 0;
  param_3[8] = 0x3f9c61aa;
  if ((*param_1 != '\0') &&
     ((local_18 != *(uint *)(param_1 + 8) || (local_14 != *(short *)(param_1 + 0xc))))) {
    param_3[0x12] = 0x3f800000;
  }
  *(uint *)(param_1 + 8) = local_18;
  *(short *)(param_1 + 0xc) = local_14;
  if (local_10 != 0) {
    bVar4 = (*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_18 & 0xffff) * 0xc) +
                      0x208) & 3) != 0;
    if (bVar4 != (bool)param_1[2]) {
      *(undefined1 *)((int)param_3 + 0x4d) = 1;
      if (0.5 <= (float)param_3[0x16]) {
        uVar2 = param_3[0x16];
      }
      else {
        uVar2 = 0x3f000000;
      }
      param_3[0x16] = uVar2;
      param_1[2] = bVar4;
    }
    if ((char)param_2[1] == '\0') {
      if ((*(float *)(param_1 + 0x10) != 0.0) || (*(float *)(param_1 + 0x14) != 0.0)) {
        param_1[0x14] = '\0';
        param_1[0x15] = '\0';
        param_1[0x16] = '\0';
        param_1[0x17] = '\0';
        param_1[0x10] = '\0';
        param_1[0x11] = '\0';
        param_1[0x12] = '\0';
        param_1[0x13] = '\0';
      }
    }
    else {
      *(float *)(param_1 + 0x10) = *(float *)(param_2 + 4) + *(float *)(param_1 + 0x10);
      *(float *)(param_1 + 0x14) = *(float *)(param_2 + 6) + *(float *)(param_1 + 0x14);
      *(undefined1 *)(param_3 + 0x14) = 1;
      if (0.4 <= (float)param_3[0x19]) {
        param_3[0x19] = param_3[0x19];
      }
      else {
        param_3[0x19] = 0x3ecccccd;
      }
    }
    fVar3 = *(float *)(param_1 + 0x18) - *(float *)(param_2 + 0x10) * 0.05;
    if (0.0 <= fVar3) {
      if (5.0 < fVar3) {
        fVar3 = 5.0;
      }
    }
    else {
      fVar3 = 0.0;
    }
    *(float *)(param_1 + 0x18) = fVar3;
    if (*(float *)(param_2 + 0x10) != 0.0) {
      *(undefined1 *)((int)param_3 + 0x4e) = 1;
      if (0.4 <= (float)param_3[0x17]) {
        uVar2 = param_3[0x17];
      }
      else {
        uVar2 = 0x3ecccccd;
      }
      param_3[0x17] = uVar2;
      *(undefined1 *)((int)param_3 + 0x4d) = 1;
      if (0.4 <= (float)param_3[0x16]) {
        uVar2 = param_3[0x16];
      }
      else {
        uVar2 = 0x3ecccccd;
      }
      param_3[0x16] = uVar2;
    }
    iVar1 = *param_2 * 0x40 + 0x10 + DAT_006b145c;
    local_24 = *(float *)(iVar1 + 0xc) + *(float *)(param_1 + 0x10);
    local_20 = *(float *)(iVar1 + 0x10) + *(float *)(param_1 + 0x14);
    if (-1.5707964 <= local_20) {
      if (1.5707964 < local_20) {
        local_20 = 1.5707964;
      }
    }
    else {
      local_20 = -1.5707964;
    }
    fVar5 = (float10)fcos((float10)local_20);
    fVar6 = (float10)fcos((float10)local_24);
    param_3[9] = (uint)(float)(fVar6 * fVar5);
    fVar6 = (float10)fsin((float10)local_24);
    param_3[10] = (uint)(float)(fVar6 * fVar5);
    fVar6 = (float10)fsin((float10)local_20);
    param_3[0xb] = (uint)(float)fVar6;
    FUN_00447190(local_20,&local_24);
    fVar3 = SQRT(local_24 * local_24 + local_20 * local_20 + local_1c * local_1c);
    param_3[7] = (uint)fVar3;
    param_3[4] = (uint)(((float)fVar5 * fVar3 + local_24) * *(float *)(param_1 + 0x18));
    param_3[5] = (uint)-(local_20 * *(float *)(param_1 + 0x18));
    param_3[6] = (uint)(((float)fVar6 * fVar3 + local_1c) * *(float *)(param_1 + 0x18));
    fVar3 = (fVar3 - 0.6) * *(float *)(param_1 + 0x18) + 0.6;
    if (fVar3 <= 0.6) {
      fVar3 = 0.6;
    }
    param_3[7] = (uint)fVar3;
    FUN_004f6aa0();
    *param_3 = *param_3 | 1;
  }
  FUN_004479c0();
  *param_1 = '\x01';
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
