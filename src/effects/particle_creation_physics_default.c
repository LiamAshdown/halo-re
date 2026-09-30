// particle_creation_physics_default  (Ghidra: no function created; the phase-4 types agent
//   carved a placeholder "missed_455310" from the .rdata dispatch-table evidence)
// address 0x455310, size 59 bytes
// VERIFIED against disassembly 0x455310..0x45534b (2026-09-30)
// name confidence 0.6, rewrite confidence 0.85
// evidence: out/phase4/effects_types_notes.md section 2: the three-entry `.rdata` table at
//   0x00657444 lists 0x455310 as entry [0], "particle creation physics, default". Matches
//   particle_system_spawn.c's (0x453b10, already rewritten) established dispatch-table signature
//   `(particle_system *system, int32_t type_index, particle_system_particle *particle,
//   object_marker *marker)` exactly. Field identity: marker->node_transform (types/objects.h
//   object_marker +0x38) is a real_matrix4x3 whose .position sub-field (math.h, +0x28) lands
//   exactly on marker+0x60/0x64/0x68, matching this function's three reads; particle->position
//   (effects.h particle_system_particle +0x1c) and particle->unknown_28 (+0x28) are the two
//   writes, and system->velocity (+0x2c) is the source for the second one.
// register convention: identical to every other entry of this dispatch table (system, type_index,
//   particle, marker); blam-cc: system, type_index, particle, marker (type_index unused here).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"

// Spawns the particle at the marker's world position with an initial "unknown_28" vector copied
// straight from the system's own velocity (world units per second).
void particle_creation_physics_default(particle_system *system, int32_t type_index,
    particle_system_particle *particle, object_marker *marker)
{
    (void)type_index;

    particle->position.x = marker->node_transform.position.x;
    particle->position.y = marker->node_transform.position.y;
    particle->position.z = marker->node_transform.position.z;

    particle->velocity.x = system->velocity.i;
    particle->velocity.y = system->velocity.j;
    particle->velocity.z = system->velocity.k;
}

#if 0
Original Ghidra decompilation (0x455310):

void missed_455310(int param_1,undefined4 param_2,int param_3,int param_4)

{
  *(undefined4 *)(param_3 + 0x1c) = *(undefined4 *)(param_4 + 0x60);
  *(undefined4 *)(param_3 + 0x20) = *(undefined4 *)(param_4 + 100);
  *(undefined4 *)(param_3 + 0x24) = *(undefined4 *)(param_4 + 0x68);
  *(undefined4 *)(param_3 + 0x28) = *(undefined4 *)(param_1 + 0x2c);
  *(undefined4 *)(param_3 + 0x2c) = *(undefined4 *)(param_1 + 0x30);
  *(undefined4 *)(param_3 + 0x30) = *(undefined4 *)(param_1 + 0x34);
  return;
}
#endif
