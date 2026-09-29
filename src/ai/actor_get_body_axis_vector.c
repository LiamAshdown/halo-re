// actor_get_body_axis_vector  (Ghidra: actor_get_body_axis_vector, renamed)
// address 0x405390, size 376 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (verified against objdump 0x405390..0x405507)
// evidence: types/ai.h actor.unit_index (0x18); types/objects.h object.up (0x80); types/
//   math.h global_forward3d_pointer (0x696718)/global_up3d_pointer (0x696720); phase-4
//   summary "one of the actor's four body-relative axis vectors (forward, back, or a
//   perpendicular pair)".
// register convention: actor index in EAX, a unit index to compare against actor.unit_index
//   in EDX, and an in/out record (an int16 axis selector at +8, a real_vector3d result at
//   +0xc) in ECX.
//   // blam-cc: EAX -> actor_index, EDX -> unit_index, ECX -> axis_request
// The ECX record is types/ai.h actor_axis_request; the review pass folded it back into the
// header. Only the two fields this function touches are named.
// UNSURE: when unit_index equals the actor's own unit_index, the reference vector is read
//   directly from actor.facing (offset 0x174), which the review pass established IS the
//   orientation forward vector rather than a position. This
//   is what the decompiled code does, but a *position* standing in for a *forward direction*
//   is suspicious; it may be that the real source is a cached direction the phase-4 offset
//   miner conflated with position, or a genuine quirk of the original. Preserved literally.
//   FUN_00569720 (the "different unit" fallback) is outside this session's range and is
//   assumed, by symmetry with the case above, to write the same three floats by pointer,
//   though Ghidra shows it called with no visible arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "fn_ai.h"

// TYPES (folded into types/ai.h by the review pass): see note above.

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0
extern const real_vector3d *global_forward3d_pointer; // 0x00696718
extern const real_vector3d *global_up3d_pointer;       // 0x00696720

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0
// 0x569720, not yet rewritten (a different module): fetches the reference vector for a unit
// other than the actor's own, by pointer (see UNSURE above).
extern void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out); // 0x569720, ECX, EAX

void actor_get_body_axis_vector(uint32_t actor_index, uint32_t unit_index, actor_axis_request *request)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    real_vector3d reference;

    if (unit_index == a->unit_index) {
        reference.i = a->facing.i;
        reference.j = a->facing.j;
        reference.k = a->facing.k;
    } else {
        // 0x4053d3: ECX = the unit (esi), EAX = &reference; the draft passed three arguments to this two-argument
        //   function, so it wrote the vector through the unit index as a pointer
        unit_get_forward_vector_or_marker_normal(unit_index, &reference);
    }

    switch (request->axis) {
    case 0:
        request->result = reference;
        return;
    case 1:
        request->result.i = -reference.i;
        request->result.j = -reference.j;
        request->result.k = -reference.k;
        return;
    case 2:
    case 3:
        {
            real_vector3d perp;

            vector3d_cross_product(&perp, &reference, global_up3d_pointer);
            if (vector3d_normalize_with_length(&perp) == 0.0f) {
                object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;

                vector3d_cross_product(&perp, &reference, &obj->up);
                if (vector3d_normalize_with_length(&perp) == 0.0f) {
                    perp = *global_forward3d_pointer;
                }
            }
            if (request->axis == 2) {
                request->result = perp;
            } else {
                request->result.i = -perp.i;
                request->result.j = -perp.j;
                request->result.k = -perp.k;
            }
        }
        return;
    }
}

#if 0
Original Ghidra decompilation (0x405390):

void FUN_00405390(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  int in_ECX;
  uint in_EDX;
  float10 fVar4;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar2 = (in_EAX & 0xffff) * 0x724;
  iVar3 = iVar2 + *(int *)(DAT_00880360 + 0x34);
  if (in_EDX == *(uint *)(iVar2 + 0x18 + *(int *)(DAT_00880360 + 0x34))) {
    local_c = *(float *)(iVar3 + 0x174);
    local_8 = *(float *)(iVar3 + 0x178);
    local_4 = *(float *)(iVar3 + 0x17c);
  }
  else {
    FUN_00569720();
  }
  sVar1 = *(short *)(in_ECX + 8);
  switch(sVar1) {
  case 0:
    *(float *)(in_ECX + 0xc) = local_c;
    *(float *)(in_ECX + 0x10) = local_8;
    *(float *)(in_ECX + 0x14) = local_4;
    return;
  case 1:
    *(float *)(in_ECX + 0xc) = -local_c;
    *(float *)(in_ECX + 0x10) = -local_8;
    *(float *)(in_ECX + 0x14) = -local_4;
    return;
  case 2:
  case 3:
    vector3d_cross_product(PTR_DAT_00696720);
    fVar4 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar4) {
      vector3d_cross_product
                (*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc) + 0x80);
      fVar4 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar4) {
        local_18 = *(float *)PTR_DAT_00696718;
        local_14 = *(float *)(PTR_DAT_00696718 + 4);
        local_10 = *(float *)(PTR_DAT_00696718 + 8);
      }
    }
    if (sVar1 == 2) {
      *(float *)(in_ECX + 0xc) = local_18;
      *(float *)(in_ECX + 0x10) = local_14;
      *(float *)(in_ECX + 0x14) = local_10;
      return;
    }
    *(float *)(in_ECX + 0xc) = -local_18;
    *(float *)(in_ECX + 0x10) = -local_14;
    *(float *)(in_ECX + 0x14) = -local_10;
  }
  return;
}
#endif
