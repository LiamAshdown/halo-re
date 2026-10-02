// model_collision_geometry_resolve_material_type  (Ghidra: FUN_00505330, still unnamed there;
// phase-2 guessed object_marker_get_node_index / out/phase4/physics_functions.md's own summary
// guessed the same "marker index -> node index" shape -- both wrong: out/phase4/
// physics_types_notes.md section 6 identifies the actual field read (materials TagReflexive at
// ModelCollisionGeometry+0x238, stride 0x48, field 0x24) as MaterialType_t, not a node index,
// and names this function alongside physics_resolve_material_type (0x507a40) as doing "the same
// lookup" for the object side. This rewrite follows that struct evidence, not the auto-generated
// summary.)
// address 0x505330, size 28 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/tags.h ModelCollisionGeometry.materials (TagReflexive at +0x238) and
//   ModelCollisionGeometryMaterial.material_type (MaterialType_t at +0x24, stride 0x48) match
//   this function's raw pointer arithmetic exactly; physics_resolve_material_type's own object
//   half (this module) performs the identical indexing once it has resolved the same
//   ModelCollisionGeometry pointer from an object index, confirming the field role.
// register convention: in_AX -> material_index, in_ECX -> definition (ModelCollisionGeometry *).
//   No stack parameters.
//   // blam-cc: AX -> material_index, ECX -> definition

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

// Resolves a global MaterialType_t out of definition's per-object materials table. Returns -1
// when material_index itself is -1 (no material).
// blam-cc: AX -> material_index, ECX -> definition
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int16_t model_collision_geometry_resolve_material_type(int16_t material_index,
                                                         ModelCollisionGeometry *definition)
{
    if (material_index != -1) {
        return ((ModelCollisionGeometryMaterial *)definition->materials.pointer)[material_index]
                   .material_type;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x505330):

int FUN_00505330(void)

{
  short in_AX;
  int in_ECX;

  if (in_AX != -1) {
    return (int)*(short *)(*(int *)(in_ECX + 0x238) + 0x24 + in_AX * 0x48);
  }
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
