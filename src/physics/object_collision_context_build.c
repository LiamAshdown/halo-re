// object_collision_context_build  (Ghidra: FUN_00504e10, still unnamed there; phase-2 guessed
// object_get_physics_shapes, which types/physics.h's own object_collision_context struct
// comment corrects: "built by 0x00504e10, consumed by 0x00504e90, 0x00504f60, 0x005050b0 and
// 0x00505200 ... 0x00504e10 fails when the Object tag has no collision_model, that is when tag
// offset 0x7c is -1.")
// address 0x504e10, size 117 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x504e10..0x504e84.)
// evidence: out/phase4/physics_types_notes.md section 2 (object_collision_context field map,
//   confirmed against this function's own stores) and section 4 (Object tag +0x7c
//   collision_model tag id); types/tags.h Object.collision_model (a TagDependency starting at
//   +0x70, whose tag_id lands at +0x7c) and ModelCollisionGeometry (tag_instances[...].data);
//   types/objects.h object.region_permutations (+0x180) and the int16 "nodes offset" at +0x1f2
//   both match the two stores this function makes into out_context->region_permutations and
//   out_context->nodes exactly.
// register convention: unaff_EDI -> object_index, in_ECX -> out_context
//   (object_collision_context *). No stack parameters.
//   // blam-cc: EDI -> object_index, ECX -> out_context
// Two already-merged sibling files in this module (collision_gather_nearby_object_shapes.c,
// object_physics_handle_nearby_object_impacts.c) already declared this function as
// `FUN_00504e10(uint32_t object_index, object_collision_context *out_context)` -- object_index
// first -- even though the raw register discovery order here (ECX before EDI) would put
// out_context first under this module's usual EAX/ECX/EDX/EBX/ESI/EDI rule. This file follows
// that established call-site precedent (object_index first) for consistency across the module,
// the same way object_physics_context_build (0x5074b0) orders its own (object_index, out_context)
// pair despite EBX/EAX register discovery order.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Builds an object_collision_context for object_index. Fails (returns 0) when the Object tag has
// no collision_model reference (TagID at Object+0x7c invalid); otherwise fills object_index, the
// ModelCollisionGeometry definition, the per-region active-permutation byte array
// (object+0x180) and the node matrix array (object + the int16 "nodes offset" at object+0x1f2).
// blam-cc: EDI -> object_index, ECX -> out_context
uint8_t object_collision_context_build(uint32_t object_index, object_collision_context *out_context)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    if (object_tag->collision_model.tag_id.index != 0xffff ||
        object_tag->collision_model.tag_id.id != 0xffff) {
        out_context->object_index = object_index;
        out_context->definition =
            tag_instances[object_tag->collision_model.tag_id.index & 0xffff].data;
        out_context->region_permutations = (uint8_t *)obj + 0x180;
        out_context->nodes = (uint8_t *)obj + *(int16_t *)((uint8_t *)obj + 0x1f2);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x504e10):

int FUN_00504e10(void)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  uint *in_ECX;
  uint unaff_EDI;

  iVar4 = DAT_0087bc14;
  iVar3 = DAT_008603b0;
  iVar5 = (unaff_EDI & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(int *)(iVar2 + 0x7c) != -1) {
    *in_ECX = unaff_EDI;
    in_ECX[1] = *(uint *)((*(uint *)(iVar2 + 0x7c) & 0xffff) * 0x20 + 0x14 + iVar4);
    in_ECX[2] = (uint)(puVar1 + 0x60);
    iVar2 = *(int *)(*(int *)(iVar3 + 0x34) + 8 + iVar5);
    in_ECX[3] = *(short *)(iVar2 + 0x1f2) + iVar2;
    return CONCAT31((int3)((uint)iVar2 >> 8),1);
  }
  return ((unaff_EDI & 0xffff) * 3 >> 6) << 8;
}
#endif
