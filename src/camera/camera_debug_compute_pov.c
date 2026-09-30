// camera_debug_compute_pov  (Ghidra: camera_debug_compute_pov, already named)
// address 0x444d50, size 885 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: this is the hs scripted-camera pov procedure (director_camera_type_scripted,
//   pov_proc == 0x444d50), dispatching on camera_script_globals.mode: 0 = a fixed or
//   object-relative cutscene camera point, 1 = a camera animation, 2 = first person through
//   camera_script_globals.object, 3 = the dead/orbiting camera reseeded around
//   camera_script_globals.object. Field mapping confirmed against types/camera.h
//   camera_script_globals and observer_command/observer_parameters. Ghidra's decompiler drops
//   every register-only argument in this function (object_try_and_get's ECX object handle in
//   three places, and the animation-sample callee's ECX/EDI/EAX inputs); this rewrite is built
//   from objdump instead, see the #if 0 block.
// review fixes (phase 4 gate, line by line against objdump 0x444d50..0x4450c4):
//   - mode 0 with a relative object that object_try_and_get rejects jumps straight to the
//     shared tail (0x444ddc je 0x44507f): no command fields are written. The earlier rewrite
//     fell through and emitted a command at the default position.
//   - mode 1 clamps the frame as an int16 (0x444f9d test ax,ax after __ftol), not as int32.
//   - the frame count / 30 constant is 0x00672ac8 (30.0); 0x006966f8 is the pointer to the
//     global zero point that other modules call global_zero_vector3d_pointer. Its value is never
//     observable here: every path that reaches the position store overwrites it first.
// register convention: matches director_pov_proc exactly -- (director_camera_data *data,
//   camera_input *input, observer_command *command), cdecl, all three on the stack (confirmed:
//   `mov esi,[ebp+0x10]` loads the 3rd stack slot as `command` right after the prologue).
//   // blam-cc: stack -> (data, input, command)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "camera.h"
#include "fn_camera.h"

extern camera_script_globals camera_script;         // 0x006869d0
extern game_time_globals *game_time;                // 0x006f1d6c
extern int16_t network_game_mode;                   // 0x00719720
extern tag_instance *tag_instances;                 // 0x0087bc14
extern real_point3d *global_zero_vector3d_pointer;  // 0x006966f8 -> 0x0065c230 {0,0,0}, math module

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, objects module


extern void animation_get_root_node_matrix(real_matrix4x3 *out, int16_t frame, ModelAnimationsAnimation *animation, GBXModel *model); // 0x4d49b0, model animation module, not yet rewritten;
                                                 // UNSURE signature (register roles read off this call
                                                 // site only: EAX -> out_transform, ECX -> frame_index,
                                                 // EDI -> animation_element, stack -> unused_flag == 0)
extern int32_t __ftol(double x); // 0x6391b4, MSVC 7.1 CRT float-to-int truncation (operand on the x87 stack)

// cos/sin/atan2 are single x87 instructions in the original code; declared locally instead of
// via <math.h> because -I types shadows that header name with types/math.h.
extern double cos(double x);
extern double sin(double x);
extern double atan2(double y, double x);

// blam-cc: stack -> (data, input, command)
// The hs scripted camera's pov procedure. `data` (the director mode union) is not read except
// in the dead-camera case, where it is reseeded and handed to camera_track_compute_pov.
void camera_debug_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    real time_scale;
    Point3D default_position;

    default_position = *(Point3D *)global_zero_vector3d_pointer;

    time_scale = (network_game_mode == 1 || network_game_mode == 2) ? 1.0f : game_time->speed;

    command->flags = 8; // snap bit: the scripted camera always commits immediately
    if (game_time->paused) {
        command->flags |= 0x20; // frozen bit while the game is paused
    }

    switch (camera_script.mode) {
    case _camera_script_mode_point: {
        if (camera_script.object != k_datum_index_none) {
            object *obj = object_try_and_get(camera_script.object, 0xffffffff);
            if (obj == (object *)0) {
                break; // 0x444ddc: straight to the shared tail
            }
            default_position = *(Point3D *)&obj->bounding_center;
        }

        command->timer = (time_scale == 0.0f) ? 0.0f : camera_script.time_remaining / time_scale;
        command->parameters.field_of_view = camera_script.field_of_view;
        command->parameters.forward = camera_script.forward;
        command->parameters.up = camera_script.up;

        if (camera_script.object == k_datum_index_none) {
            command->parameters.position = camera_script.position;
            command->flags |= 1;
        } else {
            // Relative point: rotate camera_script.position into the object's yaw (derived from
            // camera_script.forward) and add it to the object's world position.
            real yaw = (real)atan2((double)camera_script.forward.j, (double)camera_script.forward.i);
            real dot = camera_script.position.x * camera_script.forward.i +
                       camera_script.position.y * camera_script.forward.j +
                       camera_script.position.z * camera_script.forward.k;
            real remaining_x, remaining_y, remaining_z;
            if (dot > 0.0f) {
                dot = 0.0f;
            }

            command->parameters.position = default_position;
            command->parameters.distance = -dot;

            remaining_x = camera_script.position.x - dot * camera_script.forward.i;
            remaining_y = camera_script.position.y - dot * camera_script.forward.j;
            remaining_z = camera_script.position.z - dot * camera_script.forward.k;

            command->channel_times[_observer_parameter_position] = 0.0f;
            command->interpolation_flags[_observer_parameter_position] = 1;
            command->flags |= 1;

            command->parameters.focus_offset.i = (real)sin((double)yaw) * remaining_y +
                                                  remaining_x * (real)cos((double)yaw);
            command->parameters.focus_offset.j = (real)sin((double)yaw) * remaining_x -
                                                  (real)cos((double)yaw) * remaining_y;
            command->parameters.focus_offset.k = remaining_z;
        }
        break;
    }

    case _camera_script_mode_animation: {
        ModelAnimations *anims = (ModelAnimations *)tag_instances[camera_script.animation_tag & 0xffff].data;
        ModelAnimationsAnimation *anim =
            (ModelAnimationsAnimation *)((uint8_t *)anims->animations.pointer +
                                         (int32_t)camera_script.animation_index * sizeof(ModelAnimationsAnimation));
        int16_t frame = (int16_t)__ftol(
            (double)anim->frame_count - (double)(camera_script.time_remaining * 30.0f)); // 0x00672ac8
        int16_t frame_index;
        real_matrix4x3 sample;

        if (frame < 0) {
            frame_index = 0;
        } else if (frame > anim->frame_count - 1) {
            frame_index = (int16_t)(anim->frame_count - 1);
        } else {
            frame_index = frame;
        }

        animation_get_root_node_matrix(&sample, frame_index, anim, 0);

        command->parameters.forward = *(Vector3D *)&sample.forward;
        command->parameters.up = *(Vector3D *)&sample.up;
        command->parameters.position = *(Point3D *)&sample.position;
        command->parameters.distance = 0.0f;
        command->timer = 0.0f;
        command->parameters.field_of_view = 1.2217305f; // 70 degrees
        command->flags |= 1;
        break;
    }

    case _camera_script_mode_first_person: {
        object *obj = object_try_and_get(camera_script.object, 3);
        if (obj != (object *)0) {
            first_person_camera_command_for_unit(camera_script.object, command);
        }
        break;
    }

    case _camera_script_mode_dead: {
        object *obj = object_try_and_get(camera_script.object, 3);
        if (obj != (object *)0) {
            director_camera_data *track_data = data;
            if (camera_script.changed) {
                track_data = (director_camera_data *)dead_camera_new(&data->dead, input->local_player_index,
                                                                       camera_script.object);
            }
            camera_track_compute_pov(track_data, input, command);
        }
        break;
    }
    }

    camera_script.changed = 0;
    camera_script.time_remaining -= time_scale * input->dt;
    if (camera_script.time_remaining < 0.0f) {
        camera_script.time_remaining = 0.0f;
    }
}

#if 0
Original Ghidra decompilation (0x444d50):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void camera_debug_compute_pov(undefined4 param_1,int param_2,uint *param_3)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  float10 fVar9;
  float10 fVar10;
  float local_54;
  uint local_44;
  uint local_3c;
  uint local_38;
  uint local_34;
  uint local_24;
  uint local_20;
  uint local_1c;
  uint local_18;
  uint local_14;
  uint local_10;

  iVar5 = DAT_006f1d6c;
  uVar7 = *(uint *)PTR_DAT_006966f8;
  uVar8 = *(uint *)(PTR_DAT_006966f8 + 4);
  local_44 = *(uint *)(PTR_DAT_006966f8 + 8);
  if ((DAT_00719720 == 1) || (DAT_00719720 == 2)) {
    local_54 = 1.0;
  }
  else {
    local_54 = *(float *)(DAT_006f1d6c + 0x18);
  }
  *param_3 = 8;
  *param_3 = (-(uint)(*(char *)(iVar5 + 2) != '\0') & 0x20) + 8;
  iVar5 = DAT_00686a04;
  switch(DAT_006869d2) {
  case 0:
    if (DAT_00686a04 != -1) {
      iVar5 = object_try_and_get(0xffffffff);
      if (iVar5 == 0) break;
      uVar7 = *(uint *)(iVar5 + 0xa0);
      uVar8 = *(uint *)(iVar5 + 0xa4);
      local_44 = *(uint *)(iVar5 + 0xa8);
    }
    if (local_54 == 0.0) {
      fVar2 = 0.0;
    }
    else {
      fVar2 = _DAT_006869d8 / local_54;
    }
    param_3[0x12] = (uint)fVar2;
    param_3[8] = DAT_00686a00;
    pfVar1 = (float *)(param_3 + 9);
    *pfVar1 = DAT_006869e8;
    param_3[10] = DAT_006869ec;
    param_3[0xb] = DAT_006869f0;
    param_3[0xc] = DAT_006869f4;
    param_3[0xd] = DAT_006869f8;
    param_3[0xe] = DAT_006869fc;
    if (DAT_00686a04 == -1) {
      param_3[1] = (uint)DAT_006869dc;
      param_3[2] = (uint)DAT_006869e0;
      param_3[3] = (uint)DAT_006869e4;
      *param_3 = *param_3 | 1;
    }
    else {
      fVar10 = (float10)fpatan((float10)(float)param_3[10],(float10)*pfVar1);
      fVar2 = DAT_006869dc * *pfVar1 +
              DAT_006869e0 * (float)param_3[10] + DAT_006869e4 * (float)param_3[0xb];
      if (0.0 < fVar2) {
        fVar2 = 0.0;
      }
      param_3[1] = uVar7;
      param_3[7] = (uint)-fVar2;
      param_3[2] = uVar8;
      param_3[3] = local_44;
      fVar4 = DAT_006869dc - fVar2 * *pfVar1;
      fVar3 = DAT_006869e0 - fVar2 * (float)param_3[10];
      fVar2 = DAT_006869e4 - fVar2 * (float)param_3[0xb];
      param_3[0x15] = 0;
      *(undefined1 *)(param_3 + 0x13) = 1;
      fVar9 = (float10)fsin((float10)(float)fVar10);
      *param_3 = *param_3 | 1;
      fVar10 = (float10)fcos((float10)(float)fVar10);
      param_3[4] = (uint)(float)(fVar9 * (float10)fVar3 + (float10)fVar4 * fVar10);
      param_3[5] = (uint)(float)(fVar9 * (float10)fVar4 - fVar10 * (float10)fVar3);
      param_3[6] = (uint)fVar2;
    }
    break;
  case 1:
    __ftol();
    FUN_004d49b0(0);
    param_3[9] = local_3c;
    param_3[10] = local_38;
    param_3[0xb] = local_34;
    param_3[0xc] = local_24;
    param_3[0xd] = local_20;
    param_3[0xe] = local_1c;
    param_3[1] = local_18;
    param_3[2] = local_14;
    param_3[3] = local_10;
    param_3[7] = 0;
    param_3[0x12] = 0;
    param_3[8] = 0x3f9c61aa;
    *param_3 = *param_3 | 1;
    break;
  case 2:
    iVar5 = object_try_and_get(3);
    if (iVar5 != 0) {
      FUN_00446d30(param_3);
    }
    break;
  case 3:
    iVar6 = object_try_and_get(3);
    if (iVar6 != 0) {
      if (DAT_006869d1 != '\0') {
        param_1 = camera_shake_initialize(iVar5);
      }
      camera_track_compute_pov(param_1,param_2,param_3);
    }
  }
  DAT_006869d1 = 0;
  _DAT_006869d8 = _DAT_006869d8 - local_54 * *(float *)(param_2 + 4);
  if (0.0 <= _DAT_006869d8) {
    return;
  }
  _DAT_006869d8 = 0.0;
  return;
}

Disassembly (objdump -d -M intel), the register roles Ghidra's decompiler dropped:

0x444dc5..0x444dd2: mov ecx,[0x686a04] (camera_script.object); cmp ecx,-1; je ...; push -1; call 0x4f6ec0
    -> object_try_and_get(ECX = camera_script.object, stack type_mask = -1)
0x44501d..0x445027 (case 2) / 0x445040..0x44504a (case 3): mov edi,[0x686a04]; push 3; mov ecx,edi; call 0x4f6ec0
    -> object_try_and_get(ECX = camera_script.object, stack type_mask = 3)
0x445033..0x445036 (case 2): push esi(command); mov ecx,edi(camera_script.object); call 0x446d30
    -> FUN_00446d30(ECX = unit, stack = command) -- see src/camera/first_person_camera_command_for_unit.c
0x445062..0x445077 (case 3): mov dx,[edx] (input->local_player_index); push edi(unit); call 0x4450e0 (dead_camera_new,
    EAX = data already in eax, DX = local_player_index, stack = unit); then push esi,ecx,eax; call 0x445380
    (camera_track_compute_pov(data=eax's post-call value, input=ecx, command=esi))
0x444f53..0x444f96: builds the animation-frame float (frame_count - time_remaining*30) and truncates it with __ftol
0x444fb2..0x444fb8: push 0; lea eax,[esp+0x24]; call 0x4d49b0  (ECX = frame index still live from the clamp
    above, EDI = animation element pointer, EAX = out real_matrix4x3, stack = 0)
#endif
