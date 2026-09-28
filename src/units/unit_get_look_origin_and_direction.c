// unit_get_look_origin_and_direction  (Ghidra: unit_get_look_origin_and_direction, renamed)
// address 0x55a390, size 364 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x55a390..0x55a4fb; the second argument receives the tag's autoaim width; offsets probed)
// evidence: Biped.pelvis_model_node_index (0x4e4) / head_model_node_index (0x4e6) per
//   types/tags.h; object.nodes stride 0x34, position at +0x28 (real_matrix4x3, types/math.h);
//   Biped.biped_flags 0x2f4; global_up3d indirect pointer 0x00696720 (see the ground-adjust
//   cluster's own notes for how that pointer table was resolved); Biped+0x458 is UNSURE, not
//   yet named -- passed straight through as a marker/graph index.
// register convention: object index passed as the recognized stack parameter param_1 (already
//   float-typed by Ghidra from register reuse, but only its low 16 bits, masked, are ever
//   used); param_2 is a Ghidra-recognized stack output parameter; the direction and origin
//   outputs are in ESI and EDI.
//   // blam-cc: stack -> object_index (as param_1), param_2; ESI -> out_direction, EDI -> out_origin

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_point3d *global_origin3d_pointer; // 0x00696714, math.h global_origin3d_pointer
extern real_vector3d *global_up3d_pointer;    // 0x00696720, indirect pointer to math.h global_up3d

extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
    float *pill_radius_out); // 0x55a2e0, EAX, ECX, stack, EBX // 0x55a2e0

// Computes a look/aim origin and direction for a unit, preferring its pelvis and head model
// nodes when the Biped tag names both: with the "average pelvis/head" flag (biped_flags bit
// 0x10) set, origin is their midpoint and direction is the zero vector (UNSURE -- see file
// header); otherwise origin is the pelvis position and direction is head minus pelvis
// (unnormalized). Falling back when either node index is unset, it uses
// unit_get_crouch_height_offset to derive a height-based origin and a proportional offset along
// global_up3d as the direction. Always echoes the Biped tag's marker/graph index (offset 0x458,
// UNSURE) through param_2.
void unit_get_look_origin_and_direction(uint32_t object_index, uint32_t *out_autoaim_width,
                                         real_vector3d *out_direction, real_point3d *out_origin)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    real_matrix4x3 *nodes = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);

    if (tag->pelvis_model_node_index != 0xffff && tag->head_model_node_index != 0xffff) {
        real_matrix4x3 *pelvis = &nodes[tag->pelvis_model_node_index];
        real_matrix4x3 *head = &nodes[tag->head_model_node_index];

        if ((tag->biped_flags & 0x10) != 0) {
            out_origin->x = (pelvis->position.x + head->position.x) * 0.5f;
            out_origin->y = (pelvis->position.y + head->position.y) * 0.5f;
            out_origin->z = (pelvis->position.z + head->position.z) * 0.5f;
            out_direction->i = global_origin3d_pointer->x; // UNSURE: zero vector, see file header
            out_direction->j = global_origin3d_pointer->y;
            out_direction->k = global_origin3d_pointer->z;
            *out_autoaim_width = *(uint32_t *)&tag->autoaim_width; // tag + 0x458
            return;
        }
        *out_origin = pelvis->position;
        out_direction->i = head->position.x - pelvis->position.x;
        out_direction->j = head->position.y - pelvis->position.y;
        out_direction->k = head->position.z - pelvis->position.z;
        *out_autoaim_width = *(uint32_t *)&tag->autoaim_width; // tag + 0x458
        return;
    }

    {
        float pill_height, pill_radius;
        // FIXED (objdump 0x55a4aa..0x55a4b9): EAX = out_origin (the callee writes the unit position there),
        //   ECX = the unit, stack = &pill_height, EBX = &pill_radius. The draft left out_origin unset.
        unit_get_crouch_height_offset(out_origin, object_index, &pill_height, &pill_radius);
        pill_height *= 0.5f;
        out_origin->z += pill_height;
        out_direction->i = pill_height * global_up3d_pointer->i;
        out_direction->j = pill_height * global_up3d_pointer->j;
        out_direction->k = pill_height * global_up3d_pointer->k;
    }
    *out_autoaim_width = *(uint32_t *)&tag->autoaim_width; // tag + 0x458
}

#if 0
Original Ghidra decompilation (0x55a390):

void FUN_0055a390(float param_1,undefined4 *param_2)

{
  int iVar1;
  undefined *puVar2;
  int iVar3;
  int iVar4;
  float *unaff_ESI;
  float *unaff_EDI;

  iVar3 = ((uint)param_1 & 0xffff) * 0xc;
  iVar1 = *(int *)((**(uint **)(iVar3 + 8 + *(int *)(DAT_008603b0 + 0x34)) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  if ((*(short *)(iVar1 + 0x4e4) != -1) && (*(short *)(iVar1 + 0x4e6) != -1)) {
    iVar3 = *(int *)(iVar3 + 8 + *(int *)(DAT_008603b0 + 0x34));
    iVar4 = (int)*(short *)(iVar3 + 0x1f2) + *(short *)(iVar1 + 0x4e4) * 0x34 + iVar3;
    iVar3 = (int)*(short *)(iVar3 + 0x1f2) + *(short *)(iVar1 + 0x4e6) * 0x34 + iVar3;
    if ((*(byte *)(iVar1 + 0x2f4) & 0x10) != 0) {
      *unaff_EDI = (*(float *)(iVar4 + 0x28) + *(float *)(iVar3 + 0x28)) * 0.5;
      unaff_EDI[1] = (*(float *)(iVar4 + 0x2c) + *(float *)(iVar3 + 0x2c)) * 0.5;
      puVar2 = PTR_DAT_00696714;
      unaff_EDI[2] = (*(float *)(iVar4 + 0x30) + *(float *)(iVar3 + 0x30)) * 0.5;
      *unaff_ESI = *(float *)puVar2;
      unaff_ESI[1] = *(float *)(puVar2 + 4);
      unaff_ESI[2] = *(float *)(puVar2 + 8);
      *param_2 = *(undefined4 *)(iVar1 + 0x458);
      return;
    }
    *unaff_EDI = *(float *)(iVar4 + 0x28);
    unaff_EDI[1] = *(float *)(iVar4 + 0x2c);
    unaff_EDI[2] = *(float *)(iVar4 + 0x30);
    *unaff_ESI = *(float *)(iVar3 + 0x28) - *(float *)(iVar4 + 0x28);
    unaff_ESI[1] = *(float *)(iVar3 + 0x2c) - *(float *)(iVar4 + 0x2c);
    unaff_ESI[2] = *(float *)(iVar3 + 0x30) - *(float *)(iVar4 + 0x30);
    *param_2 = *(undefined4 *)(iVar1 + 0x458);
    return;
  }
  FUN_0055a2e0(&param_1);
  puVar2 = PTR_DAT_00696720;
  param_1 = param_1 * 0.5;
  unaff_EDI[2] = param_1 + unaff_EDI[2];
  *unaff_ESI = param_1 * *(float *)puVar2;
  unaff_ESI[1] = param_1 * *(float *)(puVar2 + 4);
  unaff_ESI[2] = param_1 * *(float *)(puVar2 + 8);
  *param_2 = *(undefined4 *)(iVar1 + 0x458);
  return;
}
#endif
