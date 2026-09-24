// flying_camera_update  (Ghidra: camera_debug_update_transform; renamed)
// address 0x4465d0, size 669 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: flying_camera_update_procs[0] (.data 0x00686aa8 = 0x4465d0), the "flying camera"
//   entry of 0x00686ac0. Applies the debug look input to the editor_camera_data record (yaw,
//   pitch clamped to +-1.5676548 at 0x00672f58 / 0x00672f5c, roll only while
//   flying_camera_allow_roll), builds forward from yaw / pitch, a roll free up vector and rolls
//   it about forward, moves the camera by the yaw rotated move input times
//   flying_camera_speed, either relative to the attached object (through
//   flying_camera_attached_offset) or in world space, and emits a valid command.
// register convention: director_pov_proc (cdecl, three stack arguments: esi = [esp+0x28] data,
//   ebx = [esp+0x24] input, ebp = [esp+0x34] command after the four pushes).
//   // blam-cc: stack -> (data, input, command)
// objdump details Ghidra lost (it did not see the normalize write through ECX):
//   0x446652..0x44668a  right = (forward.j, -forward.i, 0) on the stack, normalized in place
//                       by vector3d_normalize_with_length (ECX); a zero length replaces it with
//                       (1, 0, 0) (0x4466a4 stores 1.0 into right.i, both other terms are 0.0)
//   0x4466bc..0x446707  up = right x forward into command +0x30
//   0x44670a..0x44671d  vector3d_rotate_about_axis(EAX = &up, ECX = &forward, sin, cos)
//   0x44677d            object_try_and_get(ECX = attached object, push -1)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "camera.h"

extern data_array *object_data;                         // 0x008603b0, objects module
extern const real_point3d *global_origin3d_pointer;     // 0x00696714, math module
extern float flying_camera_speed;                       // 0x00686a9c
extern datum_index flying_camera_attached_object;       // 0x00686aa0
extern uint8_t flying_camera_allow_roll;                // 0x006f17fe
extern Vector3D flying_camera_attached_offset;          // 0x006f181c

extern double cos(double x);
extern double sin(double x);

// blam-cc: ECX -> v; length returned on the x87 stack
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, math module
// blam-cc: EAX -> v, ECX -> axis, stack -> (sin_angle, cos_angle)
extern void vector3d_rotate_about_axis(real_vector3d *v, const real_vector3d *axis, real sin_angle,
    real cos_angle);                                             // 0x4cd820, math module
// blam-cc: ECX -> object_index, stack -> type_mask
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, objects module

// blam-cc: stack -> (data, input, command)
void flying_camera_update(director_camera_data *data, camera_input *input, observer_command *command)
{
    editor_camera_data *camera = &data->editor;
    Vector3D *forward = &command->parameters.forward;
    Vector3D *up = &command->parameters.up;
    real_vector3d right;
    float cos_pitch;
    float cos_yaw;
    float sin_yaw;
    float move_x;
    float move_y;
    float move_z;
    Point3D position;

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
        if (flying_camera_allow_roll) {
            camera->roll = input->roll_delta + camera->roll;
        } else {
            camera->roll = 0.0f;
        }
    }

    command->timer = 0.3f; // 0x3e99999a
    cos_pitch = (float)cos(camera->pitch);
    right.k = 0.0f;
    forward->i = (float)cos(camera->yaw) * cos_pitch;
    forward->j = (float)sin(camera->yaw) * cos_pitch;
    forward->k = (float)sin(camera->pitch);
    right.j = -forward->i;
    right.i = forward->j;
    if (vector3d_normalize_with_length(&right) == 0.0f) {
        right.i = 1.0f;
        right.j = 0.0f;
        right.k = 0.0f;
    }
    up->i = right.j * forward->k - right.k * forward->j;
    up->j = right.k * forward->i - right.i * forward->k;
    up->k = right.i * forward->j - right.j * forward->i;
    vector3d_rotate_about_axis((real_vector3d *)up, (const real_vector3d *)forward,
        (real)sin(camera->roll), (real)cos(camera->roll));

    cos_yaw = (float)cos(camera->yaw);
    sin_yaw = (float)sin(camera->yaw);
    move_x = flying_camera_speed * (cos_yaw * input->move_forward - sin_yaw * input->move_left);
    move_y = flying_camera_speed * (sin_yaw * input->move_forward + cos_yaw * input->move_left);
    move_z = flying_camera_speed * input->move_up;

    if (flying_camera_attached_object != k_datum_index_none &&
        object_try_and_get(flying_camera_attached_object, 0xffffffff) != 0) {
        object *attached;

        flying_camera_attached_offset.i = flying_camera_attached_offset.i + move_x;
        flying_camera_attached_offset.j = flying_camera_attached_offset.j + move_y;
        flying_camera_attached_offset.k = flying_camera_attached_offset.k + move_z;
        attached = ((object_header *)object_data->data)[flying_camera_attached_object & 0xffff].data;
        position.x = flying_camera_attached_offset.i + attached->bounding_center.x;
        position.y = flying_camera_attached_offset.j + attached->bounding_center.y;
        position.z = flying_camera_attached_offset.k + attached->bounding_center.z;
    } else {
        position.x = move_x + camera->position.x;
        position.y = move_y + camera->position.y;
        position.z = move_z + camera->position.z;
    }

    camera->position = position;
    command->parameters.position = position;
    command->parameters.focus_offset = *(const Vector3D *)global_origin3d_pointer;
    command->parameters.distance = 0.0f;
    command->parameters.field_of_view = 1.2217305f; // 70 degrees (0x3f9c61aa), not camera->field_of_view
    command->flags = _observer_command_valid_bit;
}

#if 0
Original Ghidra decompilation (0x4465d0):

void camera_debug_update_transform(float *param_1,int param_2,undefined4 *param_3)

{
  float *pfVar1;
  undefined4 uVar2;
  float fVar3;
  uint uVar4;
  undefined *puVar5;
  char cVar6;
  int iVar7;
  float10 fVar8;
  float10 fVar9;
  float10 extraout_ST0;
  float local_18;
  float local_14;

  if (*(char *)(param_2 + 2) != '\0') {
    param_1[3] = *(float *)(param_2 + 8) + param_1[3];
    cVar6 = DAT_006f17fe;
    fVar3 = *(float *)(param_2 + 0xc) + param_1[4];
    if (-1.5676548 <= fVar3) {
      if (1.5676548 < fVar3) {
        fVar3 = 1.5676548;
      }
    }
    else {
      fVar3 = -1.5676548;
    }
    param_1[4] = fVar3;
    if (cVar6 == '\0') {
      param_1[5] = 0.0;
    }
    else {
      param_1[5] = *(float *)(param_2 + 0x10) + param_1[5];
    }
  }
  param_3[0x12] = 0x3e99999a;
  fVar8 = (float10)fcos((float10)param_1[4]);
  pfVar1 = (float *)(param_3 + 9);
  fVar9 = (float10)fcos((float10)param_1[3]);
  *pfVar1 = (float)(fVar9 * fVar8);
  fVar9 = (float10)fsin((float10)param_1[3]);
  param_3[10] = (float)(fVar9 * fVar8);
  fVar8 = (float10)fsin((float10)param_1[4]);
  param_3[0xb] = (float)fVar8;
  local_18 = (float)param_3[10];
  fVar3 = -*pfVar1;
  fVar8 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 == fVar8) {
    local_18 = 1.0;
    fVar3 = 0.0;
  }
  param_3[0xc] = fVar3 * (float)param_3[0xb] - (float)param_3[10] * 0.0;
  param_3[0xd] = *pfVar1 * 0.0 - local_18 * (float)param_3[0xb];
  param_3[0xe] = local_18 * (float)param_3[10] - fVar3 * *pfVar1;
  fVar8 = (float10)fcos((float10)param_1[5]);
  fVar9 = (float10)fsin((float10)param_1[5]);
  vector3d_rotate_about_axis((float)fVar9,(float)fVar8);
  uVar4 = DAT_00686aa0;
  fVar8 = (float10)fcos((float10)param_1[3]);
  fVar9 = (float10)fsin((float10)param_1[3]);
  local_18 = _DAT_00686a9c *
             (float)(fVar8 * (float10)*(float *)(param_2 + 0x14) -
                    fVar9 * (float10)*(float *)(param_2 + 0x18));
  local_14 = (float)((float10)_DAT_00686a9c *
                    (fVar8 * (float10)*(float *)(param_2 + 0x18) +
                    fVar9 * (float10)*(float *)(param_2 + 0x14)));
  fVar8 = (float10)_DAT_00686a9c * (float10)*(float *)(param_2 + 0x1c);
  if ((DAT_00686aa0 == 0xffffffff) ||
     (iVar7 = object_try_and_get(0xffffffff), fVar8 = extraout_ST0, iVar7 == 0)) {
    local_18 = local_18 + *param_1;
    local_14 = local_14 + param_1[1];
    fVar8 = fVar8 + (float10)param_1[2];
  }
  else {
    _DAT_006f181c = _DAT_006f181c + local_18;
    _DAT_006f1820 = _DAT_006f1820 + local_14;
    _DAT_006f1824 = (float)((float10)_DAT_006f1824 + extraout_ST0);
    iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
    local_18 = _DAT_006f181c + *(float *)(iVar7 + 0xa0);
    local_14 = _DAT_006f1820 + *(float *)(iVar7 + 0xa4);
    fVar8 = (float10)_DAT_006f1824 + (float10)*(float *)(iVar7 + 0xa8);
  }
  *param_1 = local_18;
  param_1[1] = local_14;
  param_1[2] = (float)fVar8;
  param_3[1] = local_18;
  param_3[2] = local_14;
  puVar5 = PTR_DAT_00696714;
  param_3[3] = (float)fVar8;
  param_3[4] = *(undefined4 *)puVar5;
  param_3[5] = *(undefined4 *)(puVar5 + 4);
  uVar2 = *(undefined4 *)(puVar5 + 8);
  param_3[7] = 0;
  param_3[8] = 0x3f9c61aa;
  *param_3 = 1;
  param_3[6] = uVar2;
  return;
}
#endif
