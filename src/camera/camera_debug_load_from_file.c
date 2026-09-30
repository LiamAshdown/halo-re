// camera_debug_load_from_file  (Ghidra: camera_debug_load_from_file, already named)
// address 0x445940, size 382 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: reads back the four lines camera_debug_save_to_file writes ("camera.txt", mode "r"
//   at 0x00660014) and installs the editor camera: pov_proc = 0x446e90 (the editor pov, which
//   Ghidra has no function for), look_scale 1.0, unknown_c0 0, and the write-only word
//   director.unknown_00 = 2. objdump resolves the stack slots Ghidra kept anonymous:
//     esp+0x28..0x30 position, esp+0x1c..0x24 forward, esp+0x04..0x0c saved up, esp+0x00 fov
//   (frame base = esp after `sub esp,0x34`), then
//     0x4459d8 editor_camera_set_position_and_direction  EAX = &directors[0].data (0x6ac56c),
//              ECX = &forward, EDX = &position
//     0x4459e3 vector3d_compute_up_from_forward  ESI = &forward (ECX survives 0x446e30),
//              EDI = &computed_up (esp+0x10)
//     0x4459ee vector3d_angle_between_4cd4f0  EDX = &saved_up, ECX = &computed_up
//   The roll (+0x14, 0x006ac580) is that angle, negated when dot(forward, saved_up x
//   computed_up) > 0; the cross product is written over computed_up in place.
// register convention: none; cdecl, no arguments.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#include "fn_math.h"
#include "fn_camera.h"

extern director directors[1]; // 0x006ac560

// blam-cc: EAX -> out, ECX -> direction, EDX -> position

// blam-cc: ESI -> forward, EDI -> out_up
extern void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *out_up); // 0x4479c0, this module
// blam-cc: EDX -> a, ECX -> b; result on the x87 stack

// the editor pov procedure 0x446e90 (no Ghidra function; see README known gaps)


// Loads camera.txt and switches local player 0 to the editor camera at that position and
// orientation. The roll is recovered as the signed angle between the saved up vector and the
// roll-free up vector of the saved forward.
void camera_debug_load_from_file(void)
{
    float field_of_view;
    Vector3D saved_up;
    Vector3D computed_up;
    Vector3D forward;
    Point3D position;
    void *file = fopen("camera.txt", "r");

    if (file == 0) {
        return;
    }
    fscanf(file, "%f %f %f\n", &position.x, &position.y, &position.z);
    fscanf(file, "%f %f %f\n", &forward.i, &forward.j, &forward.k);
    fscanf(file, "%f %f %f\n", &saved_up.i, &saved_up.j, &saved_up.k);
    fscanf(file, "%f\n", &field_of_view);
    fclose(file);

    editor_camera_set_position_and_direction(&directors[0].data.editor, &forward, &position);
    vector3d_compute_up_from_forward(&forward, &computed_up);
    directors[0].data.editor.roll =
        vector3d_angle_between_4cd4f0((real_vector3d *)&saved_up, (real_vector3d *)&computed_up);

    {
        // computed_up = saved_up x computed_up, written in place
        float x = computed_up.k * saved_up.j - computed_up.j * saved_up.k;
        float y = saved_up.k * computed_up.i - computed_up.k * saved_up.i;
        float z = computed_up.j * saved_up.i - computed_up.i * saved_up.j;
        computed_up.i = x;
        computed_up.j = y;
        computed_up.k = z;
    }
    if (forward.k * computed_up.k + forward.j * computed_up.j + computed_up.i * forward.i > 0.0f) {
        directors[0].data.editor.roll = -directors[0].data.editor.roll;
    }

    directors[0].data.editor.field_of_view = field_of_view;
    directors[0].pov_proc = editor_camera_compute_pov;
    directors[0].look_scale = 1.0f;
    directors[0].unknown_c0 = 0;
    directors[0].unknown_00 = 2;
}

#if 0
Original Ghidra decompilation (0x445940):

void camera_debug_load_from_file(void)

{
  FILE *_File;
  float10 fVar1;
  undefined4 local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  _File = (FILE *)FUN_00624186(&DAT_00660024,&DAT_00660014);
  if (_File != (FILE *)0x0) {
    _fscanf(_File,"%f %f %f\n",local_c,local_8,local_4);
    _fscanf(_File,"%f %f %f\n",&local_18,&local_14,&local_10);
    _fscanf(_File,"%f %f %f\n",&local_30,&local_2c,&local_28);
    _fscanf(_File,"%f\n",&local_34);
    _fclose(_File);
    FUN_00446e30();
    FUN_004479c0();
    fVar1 = (float10)vector3d_angle_between_4cd4f0();
    _DAT_006ac580 = (float)fVar1;
    if (0.0 < (local_1c * local_2c - local_20 * local_28) * local_18 +
              local_14 * (local_28 * local_24 - local_1c * local_30) +
              local_10 * (local_20 * local_30 - local_24 * local_2c)) {
      _DAT_006ac580 = -_DAT_006ac580;
    }
    _DAT_006ac584 = local_34;
    DAT_006ac568 = &LAB_00446e90;
    _DAT_006ac624 = 0x3f800000;
    DAT_006ac620 = 0;
    _DAT_006ac560 = 2;
  }
  return;
}
#endif
