// observer_new  (Ghidra: FUN_00447740; renamed for this rewrite)
// address 0x447740, size 291 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: out/phase4/camera_types_notes.md ("observer_new (0x447740, EDX = this) writes
// 0x72616421 at in_EDX[0] AND in_EDX[0xa6]"), which is exactly types/camera.h's
// k_observer_signature header/trailer pair. Every field offset below matches the observer /
// observer_camera / observer_parameters layout in types/camera.h field-for-field, and the
// pointer/value pairs read from .rdata (dumped directly) match the module's own established
// global names: 0x00696718->(1,0,0) global_forward3d_pointer, 0x00696720->(0,0,1)
// global_up3d_pointer, 0x00696714 and 0x006966f8 both -> (0,0,0) but are two distinct globals
// (global_origin3d_pointer and global_zero_vector3d_pointer respectively; kept distinct here
// even though they currently share a target, matching which one Ghidra actually reads for each
// field).
// register convention: observer * in EDX (in_EDX); no stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#include "fn_camera.h"
#include <string.h>

extern const real_vector3d *global_forward3d_pointer;    // 0x00696718 -> (1, 0, 0)
extern const real_vector3d *global_up3d_pointer;         // 0x00696720 -> (0, 0, 1)
extern const real_point3d *global_origin3d_pointer;      // 0x00696714 -> (0, 0, 0)
extern real_point3d *global_zero_vector3d_pointer;       // 0x006966f8 -> (0, 0, 0)

// blam-cc: EDX -> this
void observer_new(observer *this)
{
    this->parameters.forward = *(const Vector3D *)global_forward3d_pointer;
    this->parameters.up = *(const Vector3D *)global_up3d_pointer;
    this->parameters.field_of_view = 0.8726646f; // 50 degrees

    this->camera.position = *(const Point3D *)global_zero_vector3d_pointer;
    this->camera.leaf_index = -1;
    this->camera.cluster_index = -1;
    this->camera.velocity = *(const Vector3D *)global_origin3d_pointer;
    this->camera.forward = *(const Vector3D *)global_forward3d_pointer;
    this->camera.up = *(const Vector3D *)global_up3d_pointer;
    this->camera.field_of_view = 0.8726646f; // 50 degrees

    memset(&this->current_command, 0, sizeof(this->current_command));

    this->current_command.parameters.forward = this->parameters.forward;
    this->current_command.parameters.up = this->parameters.up;
    this->current_command.parameters.field_of_view = this->parameters.field_of_view;

    this->trailer_signature = k_observer_signature;
    this->header_signature = k_observer_signature;
    this->updated = 1;
    this->has_command = 0;
}

#if 0
Original Ghidra decompilation (0x447740):

void FUN_00447740(void)

{
  undefined *puVar1;
  undefined *puVar2;
  int iVar3;
  undefined4 *in_EDX;
  undefined4 *puVar4;

  puVar2 = PTR_DAT_00696718;
  in_EDX[0x34] = *(undefined4 *)PTR_DAT_00696718;
  in_EDX[0x35] = *(undefined4 *)(puVar2 + 4);
  in_EDX[0x36] = *(undefined4 *)(puVar2 + 8);
  puVar1 = PTR_DAT_00696720;
  in_EDX[0x37] = *(undefined4 *)PTR_DAT_00696720;
  in_EDX[0x38] = *(undefined4 *)(puVar1 + 4);
  in_EDX[0x39] = *(undefined4 *)(puVar1 + 8);
  puVar1 = PTR_DAT_006966f8;
  in_EDX[0x33] = 0x3f5f66f3;
  in_EDX[0x1d] = *(undefined4 *)puVar1;
  in_EDX[0x1e] = *(undefined4 *)(puVar1 + 4);
  in_EDX[0x1f] = *(undefined4 *)(puVar1 + 8);
  puVar1 = PTR_DAT_00696714;
  *(undefined2 *)(in_EDX + 0x21) = 0xffff;
  in_EDX[0x20] = 0xffffffff;
  in_EDX[0x22] = *(undefined4 *)puVar1;
  in_EDX[0x23] = *(undefined4 *)(puVar1 + 4);
  in_EDX[0x24] = *(undefined4 *)(puVar1 + 8);
  in_EDX[0x25] = *(undefined4 *)puVar2;
  in_EDX[0x26] = *(undefined4 *)(puVar2 + 4);
  in_EDX[0x27] = *(undefined4 *)(puVar2 + 8);
  puVar1 = PTR_DAT_00696720;
  in_EDX[0x28] = *(undefined4 *)PTR_DAT_00696720;
  in_EDX[0x29] = *(undefined4 *)(puVar1 + 4);
  in_EDX[0x2a] = *(undefined4 *)(puVar1 + 8);
  in_EDX[0x2b] = 0x3f5f66f3;
  puVar4 = in_EDX + 2;
  for (iVar3 = 0x1a; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  in_EDX[0xb] = in_EDX[0x34];
  in_EDX[0xc] = in_EDX[0x35];
  in_EDX[0xd] = in_EDX[0x36];
  in_EDX[0xe] = in_EDX[0x37];
  in_EDX[0xf] = in_EDX[0x38];
  in_EDX[0x10] = in_EDX[0x39];
  in_EDX[10] = in_EDX[0x33];
  in_EDX[0xa6] = 0x72616421;
  *in_EDX = 0x72616421;
  *(undefined1 *)(in_EDX + 0x1c) = 1;
  *(undefined1 *)((int)in_EDX + 0x71) = 0;
  return;
}
#endif
