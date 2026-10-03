// editor_camera_compute_pov  (no Ghidra function; new entry)
// address 0x446e90, size 353 bytes (0x446e90..0x446ff0)
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: camera_debug_load_from_file (0x445940) installs it as director.pov_proc
//   (0x445a95 mov ds:0x6ac568,0x446e90); the only .data / code reference to the address. It is
//   the editor_camera_data twin of flying_camera_update (0x4465d0): the same yaw / pitch input
//   with the same +-1.5676548 clamp (0x00672f58 / 0x00672f5c), but roll input always applies,
//   the move input is not scaled by flying_camera_speed, only moves while look input is present,
//   has no attached object, and publishes the record's own field of view instead of a constant.
//   The up vector is the roll free up of vector3d_compute_up_from_forward rolled about forward.
// register convention: director_pov_proc (cdecl, three stack arguments; ecx = [esp+0x8] input,
//   ebx = [esp+0x14] data, ebp = [esp+0x20] command after the prologue).
//   // blam-cc: stack -> (data, input, command)
// No Ghidra decompilation exists for this address; the #if 0 block carries the objdump.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const real_point3d *global_origin3d_pointer;     // 0x00696714, math module

extern double cos(double x);
extern double sin(double x);

// blam-cc: ESI -> forward, EDI -> out_up
extern void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *out_up); // 0x4479c0, this module
// blam-cc: EAX -> v, ECX -> axis, stack -> (sin_angle, cos_angle)
extern void vector3d_rotate_about_axis(real_vector3d *v, const real_vector3d *axis, real sin_angle,
    real cos_angle);                                             // 0x4cd820, math module

// blam-cc: stack -> (data, input, command)
void editor_camera_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    editor_camera_data *camera = &data->editor;
    float cos_pitch;

    if (input->has_look_input) {
        float pitch;

        camera->yaw = input->yaw_delta + camera->yaw;
        pitch = input->pitch_delta + camera->pitch;
        if (pitch < -1.5676548f) {
            pitch = -1.5676548f;
        } else if (pitch > 1.5676548f) {
            pitch = 1.5676548f;
        }
        camera->pitch = pitch;
        camera->roll = input->roll_delta + camera->roll;
    }

    command->timer = 0.3f; // 0x3e99999a
    cos_pitch = (float)cos(camera->pitch);
    command->parameters.forward.i = (float)cos(camera->yaw) * cos_pitch;
    command->parameters.forward.j = (float)sin(camera->yaw) * cos_pitch;
    command->parameters.forward.k = (float)sin(camera->pitch);
    vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);
    vector3d_rotate_about_axis((real_vector3d *)&command->parameters.up,
        (const real_vector3d *)&command->parameters.forward,
        (real)sin(camera->roll), (real)cos(camera->roll));

    if (input->has_look_input) {
        float cos_yaw = (float)cos(camera->yaw);
        float sin_yaw = (float)sin(camera->yaw);
        float move_x = cos_yaw * input->move_forward - sin_yaw * input->move_left;
        float move_y = sin_yaw * input->move_forward + cos_yaw * input->move_left;

        camera->position.x = move_x + camera->position.x;
        camera->position.y = move_y + camera->position.y;
        camera->position.z = input->move_up + camera->position.z;
    }

    command->parameters.position = camera->position;
    command->parameters.focus_offset = *(const Vector3D *)global_origin3d_pointer;
    command->parameters.distance = 0.0f;
    command->parameters.field_of_view = camera->field_of_view;
    command->flags = _observer_command_valid_bit;
}

#if 0
No Ghidra function exists at 0x446e90. objdump -d -M intel (condensed):

  446e90: mov ecx,[esp+0x8]                  ; input
  446e94: mov al,[ecx+0x2]                   ; has_look_input
  446e9d: mov ebx,[esp+0x14]                 ; data (after sub esp,0xc / push ebx)
  446ea2: je 0x446eeb
  446ea4: yaw += input->yaw_delta            ; [ebx+0xc]
  446ead: pitch = clamp(pitch + pitch_delta, [0x672f5c] -1.5676548, [0x672f58] 1.5676548)
  446ee2: roll += input->roll_delta          ; [ebx+0x14], no allow-roll gate
  446eeb: mov ebp,[esp+0x20]                 ; command
  446eef: mov DWORD PTR [ebp+0x48],0x3e99999a ; timer 0.3
  446ef6..446f1d: forward = (cos yaw cos pitch, sin yaw cos pitch, sin pitch) -> [ebp+0x24]
  446f20: call 0x4479c0                      ; ESI = &forward, EDI = &up
  446f25..446f3d: vector3d_rotate_about_axis(EAX = &up, ECX = &forward, sin roll, cos roll)
  446f42: mov eax,[esp+0x2c] / mov cl,[eax+0x2] ; has_look_input again
  446f50: je 0x446fa9
  446f52..446fa6: position += (cos*fwd - sin*left, sin*fwd + cos*left, up)   ; [eax+0x14/0x18/0x1c]
  446fa9..446fbb: command->position = data->position
  446fbe..446fdb: focus_offset = *global_origin3d_pointer, distance = 0
  446fde: mov eax,[ebx+0x18] / mov [ebp+0x20],eax   ; fov = data->field_of_view
  446fe4: mov DWORD PTR [ebp+0x0],0x1               ; flags = valid
  446ff0: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
