// object_set_position_and_orientation
// address 0x4f51c0, size 248 bytes
// name confidence: 0.55 (still FUN_004f51c0 in Ghidra; functions.md's summary -- "Sets an
//   object's velocity plus forward/up orientation vectors" -- calls the first field "velocity",
//   but the offsets actually written (object+0x5c/0x60/0x64) land on the documented position
//   real_point3d, not velocity at 0x068; renamed accordingly)
// rewrite confidence: 0.8
// evidence: types/objects.h object (position at 0x05c, forward at 0x074, up at 0x080); callees
//   object_recalculate_bounding_radius 0x4f8310, object_set_cluster_and_parent 0x4f5c30,
//   object_unlink_cluster_or_notify_parent 0x4f5de0. The three stack arguments and the two
//   implicit-register callee arguments were read straight off the disassembly at 0x4f51c0
//   (objdump -d -M intel bin/halo.exe), which resolved the register channels the decompiler
//   could not model.
// register convention: three stack arguments (object_index, forward, up) plus the position
//   pointer in EDI. The disassembly loads them at [esp+0x14], [esp+0x1c] and [esp+0x24] after
//   its own pushes -- i.e. stack slots 1, 2 and 3 -- and never writes EDI, confirming EDI is
//   an incoming register argument.
//     0x4f51cd  mov ebx,[esp+0x14]   ; arg1 object_index
//     0x4f51d9  mov ebp,[esp+0x1c]   ; arg2 forward
//     0x4f5220  mov eax,[esp+0x24]   ; arg3 up
//     0x4f51ec  test edi,edi         ; arg4 position, never assigned
// resolved from disassembly (previously UNSURE): the default-up branch stages the vector
//   { forward.j, -forward.i, 0 } at [esp+0xc], normalizes it in place
//   (0x4f524c lea ecx,[esp+0xc] / call 0x401990 -- vector3d_normalize_with_length takes its
//   vector in ECX), substitutes (1,0,0) when the length came back zero, and then computes
//   up = forward x that vector
//   (0x4f5288 lea edx,[esp+0xc] / lea eax,[esi+0x80] / push edx / mov ecx,ebp / call 0x4052c0;
//   vector3d_cross_product writes its result through EAX, so up = perpendicular x forward).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
// 0x4052c0: out (EAX) = stack_operand x ecx_operand. Verified against the body at 0x4052c0:
//   *EAX = ECX[2]*stack[1] - stack[2]*ECX[1], i.e. the classic cross product with the stack
//   argument as the LEFT operand.
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand);
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30, this batch; NULL location probes it
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0, this batch

// Repositions and reorients an object: writes the supplied position, forward and up vectors
// (any of which may be NULL to leave that field alone), derives a default up vector when only
// a forward vector is given, then recomputes the bounding radius and relinks the object into
// its BSP cluster.
void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward,
                                          real_vector3d *up, real_point3d *position)
    // blam-cc: stack -> object_index, forward, up; EDI -> position
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    object_unlink_cluster_or_notify_parent(object_index);

    if (position != 0) {
        obj->position = *position;
    }

    if (forward != 0) {
        obj->forward = *forward;

        if (up == 0) {
            real_vector3d perpendicular;

            perpendicular.i = forward->j;
            perpendicular.j = -forward->i;
            perpendicular.k = 0.0f;

            if (vector3d_normalize_with_length(&perpendicular) == 0.0f) {
                perpendicular.i = 1.0f;
                perpendicular.k = 0.0f;
                perpendicular.j = 0.0f;
            }
            // up = perpendicular x forward
            vector3d_cross_product(&obj->up, forward, &perpendicular);
        } else {
            obj->up = *up;
        }
    }

    object_recalculate_bounding_radius(object_index);
    object_set_cluster_and_parent(object_index, 0);
}

#if 0
Original Ghidra decompilation (0x4f51c0):

void FUN_004f51c0(uint param_1,float *param_2,undefined4 *param_3)

{
  int iVar1;
  undefined4 *unaff_EDI;
  float10 fVar2;
  float local_c;
  float local_8;
  undefined4 local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  FUN_004f5de0();
  if (unaff_EDI != (undefined4 *)0x0) {
    *(undefined4 *)(iVar1 + 0x5c) = *unaff_EDI;
    *(undefined4 *)(iVar1 + 0x60) = unaff_EDI[1];
    *(undefined4 *)(iVar1 + 100) = unaff_EDI[2];
  }
  if (param_2 != (float *)0x0) {
    *(float *)(iVar1 + 0x74) = *param_2;
    *(float *)(iVar1 + 0x78) = param_2[1];
    *(float *)(iVar1 + 0x7c) = param_2[2];
    if (param_3 == (undefined4 *)0x0) {
      local_c = param_2[1];
      local_8 = -*param_2;
      local_4 = 0;
      fVar2 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar2) {
        local_c = 1.0;
        local_4 = 0;
        local_8 = 0.0;
      }
      vector3d_cross_product(&local_c);
    }
    else {
      *(undefined4 *)(iVar1 + 0x80) = *param_3;
      *(undefined4 *)(iVar1 + 0x84) = param_3[1];
      *(undefined4 *)(iVar1 + 0x88) = param_3[2];
    }
  }
  object_recalculate_bounding_radius(param_1);
  FUN_004f5c30(param_1,0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
