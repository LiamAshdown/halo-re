// camera_first_person_compute_pov  (Ghidra: camera_first_person_compute_pov, already named)
// address 0x446d60, size 199 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: director_camera_type_first_person's pov_proc (types/camera.h). `data` is
//   first_person_camera_data, whose only field is a cached field_of_view float, so this
//   function's first stack argument (a bare float*) is the director_camera_data pointer itself.
//   Ghidra's decompiler drops every register argument here (the incoming command/vector
//   registers into first_person_camera_for_unit_and_vector, and FUN_00473d70's inputs); this
//   rewrite is built from objdump, see the #if 0 block.
// register convention: matches director_pov_proc exactly -- (director_camera_data *data,
//   camera_input *input, observer_command *command), cdecl, all three on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern player_globals *local_player_globals;           // 0x0087a478
extern director_pov_proc director_last_pov_proc;       // 0x006f17f8

// blam-cc: EAX -> player_handle, ECX -> yaw_pitch, ESI -> out_forward
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch,
    real_vector3d *out_forward); // 0x473d70, game module
extern void first_person_camera_for_unit_and_vector(observer_command *command, Vector3D *vector, datum_index unit); // 0x446b70, this module
// blam-cc: AX -> local_player_index (0x446dbb mov ax,[edi] right before the call)
// UNSURE name: the game module calls 0x471f90 game_engine_get_max_look_pitch, but this caller
// stores its result as the command field of view and eases the fov channel when it changes.
extern real game_engine_get_max_look_pitch(int16_t local_player_index); // 0x471f90, game module

// blam-cc: stack -> (data, input, command)
// The per-mode pov callback for first person: derives the local player's look direction from
// their desired yaw/pitch (player_compute_view_forward_vector), builds the observer command from it and the unit's
// aiming position (first_person_camera_for_unit_and_vector), and eases the field of view over
// 0.18 seconds whenever it changes. Position and orientation are snapped instantly whenever the
// previous update's pov procedure was not this one (i.e. right after switching into first
// person).
void camera_first_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    float *cached_field_of_view = (float *)data; // first_person_camera_data has one field at +0x00
    datum_index local_player;
    Vector3D direction;
    real fov;

    if (input->local_player_index == -1 || input->local_player_index >= 1) {
        local_player = k_datum_index_none; // UNSURE: dead code in this build (k_camera_local_player_count == 1)
    } else {
        local_player = local_player_globals->local_players[input->local_player_index];
    }

    player_compute_view_forward_vector(local_player,
        &player_control_globals_ptr->local_players[input->local_player_index].yaw,
        (real_vector3d *)&direction);

    first_person_camera_for_unit_and_vector(command, &direction,
                                             player_control_globals_ptr->local_players[input->local_player_index].unit);

    fov = game_engine_get_max_look_pitch(input->local_player_index);
    command->parameters.field_of_view = fov;
    if (*cached_field_of_view != fov) {
        command->interpolation_flags[_observer_parameter_field_of_view] = 1;
        *cached_field_of_view = fov;
        command->channel_times[_observer_parameter_field_of_view] = 0.18f;
    }

    if (director_last_pov_proc != camera_first_person_compute_pov) {
        command->channel_times[_observer_parameter_field_of_view] = 0.0f;
    }

    command->interpolation_flags[_observer_parameter_orientation] |= 3;
    command->channel_times[_observer_parameter_orientation] = 0.0f;
    command->channel_times[_observer_parameter_position] = 0.0f;
    command->timer = 0.0f;
    command->interpolation_flags[_observer_parameter_position] |= 3;
    command->flags |= 1;
}

#if 0
Original Ghidra decompilation (0x446d60) -- UNRELIABLE, every register argument is lost; kept
for reference only:

void camera_first_person_compute_pov(float *param_1,short *param_2,uint *param_3)

{
  undefined4 uVar1;
  float10 fVar2;

  uVar1 = *(undefined4 *)(*param_2 * 0x40 + 0x10 + DAT_006b145c);
  FUN_00473d70();
  first_person_camera_for_unit_and_vector(uVar1);
  fVar2 = (float10)FUN_00471f90();
  param_3[8] = (uint)(float)fVar2;
  if ((float10)*param_1 != fVar2) {
    *(undefined1 *)((int)param_3 + 0x4f) = 1;
    *param_1 = (float)fVar2;
    param_3[0x18] = 0x3e3851ec;
  }
  if (DAT_006f17f8 != camera_first_person_compute_pov) {
    param_3[0x18] = 0;
  }
  *(byte *)(param_3 + 0x14) = (byte)param_3[0x14] | 3;
  param_3[0x19] = 0;
  param_3[0x15] = 0;
  param_3[0x12] = 0;
  *(byte *)(param_3 + 0x13) = (byte)param_3[0x13] | 3;
  *param_3 = *param_3 | 1;
  return;
}

Disassembly (objdump -d -M intel, 0x446d60..0x446e26), confirming the register roles Ghidra dropped:

0x446d6c: mov edi, dword ptr [esp + 0x20]     ; input (2nd stack arg)
0x446d95..0x446d9a: eax = local_player_globals->local_players[local_player_index]
0x446da3: lea ecx, [edx + 0xc]                  ; &player_control[idx].desired_yaw (+0x1c overall)
0x446da6: lea esi, [esp + 0xc]                   ; out direction buffer
0x446daa: call 0x473d70                           ; FUN_00473d70(EAX=local_player, ECX=&desired_yaw, ESI=out)
0x446daf: push ebx                                 ; save player_control[idx].unit for the next call's stack arg
0x446db0: mov ebx, dword ptr [esp + 0x28]          ; command (3rd stack arg)
0x446db4: mov eax, esi                              ; vector = out direction buffer
0x446db6: call 0x446b70                              ; first_person_camera_for_unit_and_vector(EBX=command, EAX=vector, stack=unit)
0x446dc1: call 0x471f90                               ; fov
0x446dc9: mov ecx, dword ptr [esp + 0x1c]              ; data (1st stack arg == &data->first_person.field_of_view)
0x446dfe..0x446e05: command.interpolation_flags[4] (orientation) |= 3
0x446e08..0x446e1d: command.interpolation_flags[0] (position) |= 3
0x446e0b/0x446e0e/0x446e11: channel_times[4]=0, channel_times[0]=0, command.timer=0
0x446e19: command.flags |= 1
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
