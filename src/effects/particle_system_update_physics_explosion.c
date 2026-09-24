// particle_system_update_physics_explosion  (Ghidra: no function created; the phase-4 types
//   agent carved a placeholder "missed_4554d0" from the .rdata dispatch-table evidence)
// address 0x4554d0, size 5 bytes
// name confidence 0.6, rewrite confidence 0.9
// evidence: out/phase4/effects_types_notes.md section 2: the two-entry `.rdata` table at
//   0x0065743c lists 0x4554d0 as entry [1], "system update physics, explosion". objdump shows
//   its entire body is a single `jmp 0x4552a0` (five bytes, `e9 cb fd ff ff`) -- a true tail call
//   with no prologue of its own, byte-identical in observable behaviour to entry [0]
//   (particle_system_update_physics_default.c, this batch). Reproduced as a forwarding call
//   rather than a duplicated body, since that is exactly what the jmp does.
// register convention: identical to particle_system_update_physics_default (system, dt);
//   the jmp reuses the caller's frame unchanged.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"

extern void particle_system_update_physics_default(particle_system *system, real dt); // 0x4552a0, this batch

// ParticleSystem.system_update_physics dispatch table entry 1, "explosion". Identical to the
// default implementation (a plain jmp to it in the original binary).
void particle_system_update_physics_explosion(particle_system *system, real dt)
{
    particle_system_update_physics_default(system, dt);
}

#if 0
Original Ghidra decompilation (0x4554d0):

// (not exported; 5 bytes, lib=True per pack.py's export scan)

Raw objdump (bin/halo.exe):

004554d0:  e9 cb fd ff ff    jmp    0x4552a0
#endif
