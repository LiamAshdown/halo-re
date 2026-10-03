// point_physics_interpolate  (Ghidra: FUN_0050b9e0)
// address 0x50b9e0, size 151 bytes
// name confidence: 0.55   rewrite confidence: 0.95
// evidence: objdump -d 0x50b9e0..0x50ba76. particle_update_physics_default (0x455467) blends the
//   current and next particle state's point_physics tags into a stack PointPhysics with it. The
//   flags are copied from `from`; mass_scale, water/air_gravity_scale, density, the three
//   frictions and elasticity become from * (1 - fraction) + to * fraction. The padding and the
//   last 12 bytes are left untouched. Stores are in the binary's order (0x20, 0x08, 0x0c, 0x04,
//   0x24, 0x28, 0x2c, 0x30).
// register convention: out in EAX, from in ECX, to in EDX, fraction on the stack.
//   // blam-cc: EAX -> out, ECX -> from, EDX -> to, stack -> fraction

#include "tags.h"
#include "memory.h"
#include "math.h"

// blam-cc: EAX -> out, ECX -> from, EDX -> to, stack -> fraction
// Linearly interpolates the tunable fields of two point_physics definitions.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void point_physics_interpolate(PointPhysics *out, const PointPhysics *from, const PointPhysics *to, float fraction)
{
    float inverse = 1.0f - fraction;

    out->flags = from->flags;
    out->density = inverse * from->density + fraction * to->density;
    out->water_gravity_scale = inverse * from->water_gravity_scale + fraction * to->water_gravity_scale;
    out->air_gravity_scale = inverse * from->air_gravity_scale + fraction * to->air_gravity_scale;
    out->mass_scale = inverse * from->mass_scale + fraction * to->mass_scale;
    out->air_friction = inverse * from->air_friction + fraction * to->air_friction;
    out->water_friction = inverse * from->water_friction + fraction * to->water_friction;
    out->surface_friction = inverse * from->surface_friction + fraction * to->surface_friction;
    out->elasticity = inverse * from->elasticity + fraction * to->elasticity;
}

#if 0
Disassembly (0x50b9e0): fld 1.0; fsub [esp+4]; out->flags = from->flags; then for each of
+0x20 +0x08 +0x0c +0x04 +0x24 +0x28 +0x2c +0x30:
  out[f] = (1 - t) * from[f] + t * to[f]
ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
