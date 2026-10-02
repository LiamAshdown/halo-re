// physics_resolve_material_type  (Ghidra: FUN_00507a40; renamed)
// address 0x507a40, size 114 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/physics_functions.md summary ("Looks up the collision-material id for a
//   given vertex slot of an object, or a global default table entry when no object index is
//   supplied"); types/tags.h ModelCollisionGeometry.materials (TagReflexive at 0x238,
//   ModelCollisionGeometryMaterial stride 0x48, material_type at +0x24) and
//   ScenarioStructureBSP.collision_materials (TagReflexive at 0xa8,
//   ScenarioStructureBSPCollisionMaterial stride 0x14, material at +0x12) both already carry the
//   exact offsets this function's raw pointer arithmetic uses, which is why this rewrite
//   accesses them by field name instead. out/phase4/physics_types_notes.md's mass_point_state
//   table: "material_type | 0x507ac0 via 0x507a40" confirms this is the same lookup
//   object_physics's mass-point contact path (0x507ac0, this module, higher half) uses.
// register convention: in_EAX -> object_index, in_CX -> vertex_slot (both entirely hidden;
//   Ghidra recognized no parameters at all).
//   // blam-cc: EAX -> object_index, ECX (low 16 bits) -> vertex_slot

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;                     // 0x008603b0
extern tag_instance *tag_instances;                 // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

// Resolves a global MaterialType_t for vertex_slot: when object_index names a real object,
// indexes that object's ModelCollisionGeometry.materials (via its Object tag's collision_model
// dependency, tag +0x7c); when object_index is -1 (the world), indexes the current structure
// BSP's collision_materials instead. Returns -1 when vertex_slot itself is -1.
int16_t physics_resolve_material_type(uint32_t object_index, int16_t vertex_slot)
{
    if (vertex_slot == -1) {
        return -1;
    }

    if (object_index != 0xffffffff) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        void *object_tag_data = tag_instances[obj->definition_tag & 0xffff].data;
        int32_t collision_model_id = *(int32_t *)((uint8_t *)object_tag_data + 0x7c);
        ModelCollisionGeometry *geometry =
            (ModelCollisionGeometry *)tag_instances[(uint16_t)collision_model_id].data;

        return ((ModelCollisionGeometryMaterial *)geometry->materials.pointer)[vertex_slot].material_type;
    }

    return ((ScenarioStructureBSPCollisionMaterial *)
        global_structure_bsp->collision_materials.pointer)[vertex_slot].material;
}

#if 0
Original Ghidra decompilation (0x507a40):

undefined2 FUN_00507a40(void)

{
  uint in_EAX;
  short in_CX;

  if (in_CX == -1) {
    return 0xffff;
  }
  if (in_EAX != 0xffffffff) {
    return *(undefined2 *)
            (*(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                       (in_EAX & 0xffff) * 0xc) & 0xffff) *
                                           0x20 + 0x14 + DAT_0087bc14) + 0x7c) & 0xffff) *
                       0x20 + 0x14 + DAT_0087bc14) + 0x238) + 0x24 + in_CX * 0x48);
  }
  return *(undefined2 *)(*(int *)(DAT_00746f9c + 0xa8) + 0x12 + in_CX * 0x14);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
