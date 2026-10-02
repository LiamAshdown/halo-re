// particle_system_advance_particle_state  (Ghidra: FUN_00454450, still unnamed there; named
//   directly by out/phase4/effects_types_notes.md: "particle_system_advance_particle_state
//   0x454450 ... mirroring FUN_004543b0 for a different field")
// address 0x454450, size 147 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/effects.h particle_system_particle (ping_pong_forward +0x02, state_index
//   +0x08, next_state_index +0x0a); types/tags.h ParticleSystemType (particle_states
//   TagReflexive +0x74, flags +0x20, particle_states_loop bit 0x04, forward_backward_1 bit 0x08).
// register convention: particle pointer in EAX (in_EAX); ParticleSystemType tag pointer in ECX
//   (in_ECX).
//   // blam-cc: in_EAX -> particle, in_ECX -> type

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void particle_system_advance_particle_state(particle_system_particle *particle,
    ParticleSystemType *type) // blam-cc: in_EAX, in_ECX
{
    int16_t direction = particle->ping_pong_forward ? 1 : -1;
    int16_t next = particle->state_index + direction;

    particle->next_state_index = next;

    if (next < 0 || (int32_t)type->particle_states.count <= next) {
        if ((type->flags & 0x04) != 0 && 0 < (int32_t)type->particle_states.count) {
            if ((type->flags & 0x08) == 0) {
                particle->next_state_index = 0;
                return;
            }

            {
                int32_t reflected = (int32_t)particle->state_index - direction;

                if (reflected < 0) {
                    particle->next_state_index = 0;
                    particle->ping_pong_forward = (particle->ping_pong_forward == 0);
                    return;
                }

                {
                    int32_t last = (int32_t)type->particle_states.count - 1;

                    if (last < reflected) {
                        reflected = last;
                    }
                    particle->next_state_index = (int16_t)reflected;
                    particle->ping_pong_forward = (particle->ping_pong_forward == 0);
                }
            }
        } else {
            particle->state_index = -1;
            particle->next_state_index = -1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x454450):

void FUN_00454450(void)

{
  int iVar1;
  short sVar2;
  short sVar3;
  char cVar4;
  int in_EAX;
  int in_ECX;
  int iVar5;

  cVar4 = *(char *)(in_EAX + 2);
  sVar3 = (ushort)(cVar4 != '\0') * 2 + -1;
  sVar2 = *(short *)(in_EAX + 8) + sVar3;
  *(short *)(in_EAX + 10) = sVar2;
  if ((sVar2 < 0) || (*(int *)(in_ECX + 0x74) <= (int)sVar2)) {
    if (((*(uint *)(in_ECX + 0x20) & 4) != 0) && (0 < *(int *)(in_ECX + 0x74))) {
      if ((*(uint *)(in_ECX + 0x20) & 8) == 0) {
        *(undefined2 *)(in_EAX + 10) = 0;
        return;
      }
      iVar5 = (int)*(short *)(in_EAX + 8) - (int)sVar3;
      if (iVar5 < 0) {
        *(undefined2 *)(in_EAX + 10) = 0;
        *(bool *)(in_EAX + 2) = cVar4 == '\0';
        return;
      }
      iVar1 = *(int *)(in_ECX + 0x74) + -1;
      if (iVar1 < iVar5) {
        iVar5 = iVar1;
      }
      *(short *)(in_EAX + 10) = (short)iVar5;
      *(bool *)(in_EAX + 2) = cVar4 == '\0';
      return;
    }
    *(undefined2 *)(in_EAX + 8) = 0xffff;
    *(undefined2 *)(in_EAX + 10) = 0xffff;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
