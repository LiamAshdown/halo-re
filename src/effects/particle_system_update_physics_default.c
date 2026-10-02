// particle_system_update_physics_default  (Ghidra: no function created; the phase-4 types agent
//   carved the stub name "missed_4552a0" from the .rdata dispatch-table evidence)
// address 0x4552a0, size 98 bytes
// VERIFIED against disassembly 0x4552a0..0x455302 (2026-09-30). point_physics_tick's velocity (ESI) is
//   &system->velocity (0x4552df)
// name confidence 0.6, rewrite confidence 0.65
// evidence: out/phase4/effects_types_notes.md section 2: the two-entry `.rdata` table at
//   0x0065743c (indexed by ParticleSystem.system_update_physics, tag +0x48) lists 0x4552a0 as
//   entry [0], "system update physics, default"; the call site is particle_system_update 0x4544f0.
//   Every field this function touches matches types/effects.h's particle_system exactly:
//   object_index (+0x0c, -1 test), definition_index (+0x08, a direct tag reference despite its
//   name, matching every other `_index` datum field this module treats as a tag id), location
//   (+0x18), position (+0x20) and velocity (+0x2c); types/tags.h's ParticleSystem.point_physics
//   (a TagDependency at +0x38, whose tag_id lands exactly on the +0x44 this function reads) is
//   the second tag lookup, feeding point_physics_tick's PointPhysics parameter directly
//   (src/physics/point_physics_tick.c, already rewritten).
// register convention: system pointer and delta time in two register arguments (matches
//   particle_system_update's own per-system-state loop); blam-cc: system, dt. UNSURE: which
//   physical registers carry them -- Ghidra recovered both as plain stack parameters here, and
//   objdump shows the second one only ever read once, early, to seed the `dt` stack argument of
//   point_physics_tick, so a register/stack distinction makes no observable difference.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14

extern uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition,
    bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind,
    real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt); // 0x50b530, foreign (physics)

// ParticleSystem.system_update_physics dispatch table entry 0, the default implementation.
// For a free-standing system (object_index == -1) whose ParticleSystem tag references a
// point_physics tag, runs one point_physics_tick on the system's own position/velocity/location
// with a fixed 1.0 radius, no wind probe and no collision normal/material output.
void particle_system_update_physics_default(particle_system *system, real dt)
{
    ParticleSystem *definition_tag;
    uint32_t point_physics_tag_id;

    if (system->object_index != (datum_index)-1) {
        return;
    }

    definition_tag = (ParticleSystem *)tag_instances[system->definition_index & 0xffff].data;
    point_physics_tag_id = *(uint32_t *)&definition_tag->point_physics.tag_id;
    if (point_physics_tag_id == (uint32_t)-1) {
        return;
    }

    point_physics_tick(&system->velocity, 0,
        (PointPhysics *)tag_instances[point_physics_tag_id & 0xffff].data,
        &system->location, (uint32_t)-1, &system->position, (real_vector3d *)0,
        (real_vector3d *)0, (int16_t *)0, 1.0f, dt);
}

#if 0
Original Ghidra decompilation (0x4552a0):

void missed_4552a0(int param_1,undefined4 param_2)

{
  uint uVar1;

  if ((*(int *)(param_1 + 0xc) == -1) &&
     (uVar1 = *(uint *)(*(int *)((*(uint *)(param_1 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                       0x44), uVar1 != 0xffffffff)) {
    point_physics_tick(0,*(undefined4 *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
                       param_1 + 0x18,0xffffffff,param_1 + 0x20,0,0,0,0x3f800000,param_2);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
