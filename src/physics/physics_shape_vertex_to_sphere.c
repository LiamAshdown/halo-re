// physics_shape_vertex_to_sphere  (Ghidra: FUN_00503360, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503360, size 291 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase4/physics_types_notes.md section 2 derives physics_model_sphere and
//   physics_model_pill's field layout directly from this function's stores; the struct comment
//   "the second sphere of a pair sits at center_z minus the caller height, which is how a
//   standing capsule is approximated" matches the second and third blocks here exactly (a
//   second sphere offset down by height_offset, plus a connecting vertical pill).
// register convention: in_ECX -> model (physics_model *), unaff_ESI -> vertex (real_point3d *),
//   unaff_DI -> material_type (the low 16 bits of EDI). param_1..param_6 are Ghidra-recognized
//   stack parameters.
//   // blam-cc: ECX -> model, ESI -> vertex, DI -> material_type,
//   //           stack -> height_offset, radius, object_index, surface_index, surface_flags,
//   //           breakable_surface_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

// blam-cc: ECX -> model, ESI -> vertex, DI -> material_type,
//          stack -> height_offset, radius, object_index, surface_index, surface_flags,
//          breakable_surface_index
void physics_shape_vertex_to_sphere(physics_model *model, real_point3d *vertex,
                                     int16_t material_type, float height_offset, float radius,
                                     uint32_t object_index, int32_t surface_index,
                                     uint8_t surface_flags, int8_t breakable_surface_index)
{
    // Memory order follows objdump -d 0x503360..0x503482 exactly (it matters when `vertex`
    // aliases the model, which difftest's random pointers do): the first sphere copies the
    // vertex one coordinate at a time, the lowered z is read before the second sphere is
    // written, and both later records read x and y before writing them.
    float lowered_z;

    if (model->sphere_count < 0x100) {
        physics_model_sphere *sphere = &model->spheres[model->sphere_count];
        model->sphere_count += 1;
        sphere->object_index = object_index;
        sphere->surface_flags = surface_flags;
        sphere->breakable_surface_index = breakable_surface_index;
        sphere->surface_index = surface_index;
        sphere->material_type = material_type;
        sphere->center_x = vertex->x;
        sphere->center_y = vertex->y;
        sphere->center_z = vertex->z;
        sphere->radius = radius;
    }

    if (!(0.0f < height_offset)) {
        return;
    }
    lowered_z = vertex->z - height_offset;

    if (model->sphere_count < 0x100) {
        physics_model_sphere *sphere = &model->spheres[model->sphere_count];
        float x, y;
        model->sphere_count += 1;
        sphere->object_index = object_index;
        sphere->surface_flags = surface_flags;
        sphere->surface_index = surface_index;
        sphere->breakable_surface_index = breakable_surface_index;
        sphere->material_type = material_type;
        x = vertex->x;
        y = vertex->y;
        sphere->center_x = x;
        sphere->center_y = y;
        sphere->center_z = lowered_z;
        sphere->radius = radius;
    }
    if (model->pill_count < 0x100) {
        physics_model_pill *pill = &model->pills[model->pill_count];
        float x, y;
        model->pill_count += 1;
        pill->object_index = object_index;
        pill->surface_index = surface_index;
        pill->surface_flags = surface_flags;
        pill->breakable_surface_index = breakable_surface_index;
        pill->material_type = material_type;
        x = vertex->x;
        y = vertex->y;
        pill->origin_x = x;
        pill->origin_y = y;
        pill->origin_z = lowered_z;
        pill->extent_i = 0.0f;
        pill->extent_j = 0.0f;
        pill->extent_k = height_offset;
        pill->radius = radius;
    }
}


#if 0
Original Ghidra decompilation (0x503360):

void FUN_00503360(float param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined1 param_5,undefined1 param_6)

{
  short *psVar1;
  float fVar2;
  undefined4 uVar3;
  short sVar4;
  short *in_ECX;
  undefined4 *unaff_ESI;
  short unaff_DI;

  sVar4 = *in_ECX;
  if (sVar4 < 0x100) {
    *in_ECX = sVar4 + 1;
    psVar1 = in_ECX + sVar4 * 0xe + 4;
    *(undefined4 *)psVar1 = param_3;
    *(undefined1 *)(psVar1 + 4) = param_5;
    *(undefined1 *)((int)psVar1 + 9) = param_6;
    *(undefined4 *)(psVar1 + 2) = param_4;
    psVar1[5] = unaff_DI;
    *(undefined4 *)(psVar1 + 6) = *unaff_ESI;
    *(undefined4 *)(psVar1 + 8) = unaff_ESI[1];
    *(undefined4 *)(psVar1 + 10) = unaff_ESI[2];
    *(undefined4 *)(psVar1 + 0xc) = param_2;
  }
  if (0.0 < param_1) {
    fVar2 = (float)unaff_ESI[2];
    sVar4 = *in_ECX;
    if (sVar4 < 0x100) {
      psVar1 = in_ECX + sVar4 * 0xe + 4;
      *in_ECX = sVar4 + 1;
      *(undefined4 *)psVar1 = param_3;
      *(undefined1 *)(psVar1 + 4) = param_5;
      *(undefined4 *)(psVar1 + 2) = param_4;
      *(undefined1 *)((int)psVar1 + 9) = param_6;
      psVar1[5] = unaff_DI;
      uVar3 = unaff_ESI[1];
      *(undefined4 *)(psVar1 + 6) = *unaff_ESI;
      *(undefined4 *)(psVar1 + 8) = uVar3;
      *(float *)(psVar1 + 10) = fVar2 - param_1;
      *(undefined4 *)(psVar1 + 0xc) = param_2;
    }
    sVar4 = in_ECX[1];
    if (sVar4 < 0x100) {
      in_ECX[1] = sVar4 + 1;
      psVar1 = in_ECX + sVar4 * 0x14 + 0xe04;
      *(undefined4 *)psVar1 = param_3;
      *(undefined4 *)(psVar1 + 2) = param_4;
      *(undefined1 *)(psVar1 + 4) = param_5;
      *(undefined1 *)((int)psVar1 + 9) = param_6;
      psVar1[5] = unaff_DI;
      uVar3 = unaff_ESI[1];
      *(undefined4 *)(psVar1 + 6) = *unaff_ESI;
      *(undefined4 *)(psVar1 + 8) = uVar3;
      *(float *)(psVar1 + 10) = fVar2 - param_1;
      psVar1[0xc] = 0;
      psVar1[0xd] = 0;
      psVar1[0xe] = 0;
      psVar1[0xf] = 0;
      *(float *)(psVar1 + 0x10) = param_1;
      *(undefined4 *)(psVar1 + 0x12) = param_2;
      return;
    }
  }
  return;
}
#endif
