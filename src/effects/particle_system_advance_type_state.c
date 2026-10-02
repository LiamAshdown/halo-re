// particle_system_advance_type_state  (Ghidra: FUN_004543b0, still unnamed there; named directly
//   by out/phase4/effects_types_notes.md: "particle_system_advance_type_state 0x4543b0")
// address 0x4543b0, size 159 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/effects.h particle_system_type_state (state_index +0x00, next_state_index
//   +0x02, ping_pong_forward +0x38) and particle_system.object_index (+0x0c); types/tags.h
//   ParticleSystemType (states TagReflexive +0x68, flags +0x20, type_states_loop bit 0x01,
//   forward_backward bit 0x02).
// register convention: type_state pointer in EAX (in_EAX); ParticleSystemType tag pointer in ECX
//   (in_ECX); the owning particle_system as the recognized stack parameter (param_1).
//   // blam-cc: in_EAX -> state, in_ECX -> type, stack -> system

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void particle_system_advance_type_state(particle_system_type_state *state, ParticleSystemType *type,
    particle_system *system) // blam-cc: in_EAX, in_ECX, stack
{
    int16_t direction = state->ping_pong_forward ? 1 : -1;
    int16_t next = state->state_index + direction;

    state->next_state_index = next;

    if (next < 0 || (int32_t)type->states.count <= next) {
        if ((type->flags & 0x01) != 0 && system->object_index != (datum_index)0xffffffff &&
            0 < (int32_t)type->states.count) {
            if ((type->flags & 0x02) == 0) {
                state->next_state_index = 0;
                return;
            }

            {
                int32_t reflected = (int32_t)state->state_index - direction;

                if (reflected < 0) {
                    state->next_state_index = 0;
                    state->ping_pong_forward = (state->ping_pong_forward == 0);
                    return;
                }

                {
                    int32_t last = (int32_t)type->states.count - 1;

                    if (last < reflected) {
                        reflected = last;
                    }
                    state->next_state_index = (int16_t)reflected;
                    state->ping_pong_forward = (state->ping_pong_forward == 0);
                }
            }
        } else {
            state->state_index = -1;
            state->next_state_index = -1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4543b0):

void FUN_004543b0(int param_1)

{
  int iVar1;
  short sVar2;
  short sVar3;
  char cVar4;
  short *in_EAX;
  int in_ECX;
  int iVar5;

  cVar4 = (char)in_EAX[0x1c];
  sVar3 = (ushort)(cVar4 != '\0') * 2 + -1;
  sVar2 = *in_EAX + sVar3;
  in_EAX[1] = sVar2;
  if ((sVar2 < 0) || (*(int *)(in_ECX + 0x68) <= (int)sVar2)) {
    if (((*(uint *)(in_ECX + 0x20) & 1) != 0) &&
       ((*(int *)(param_1 + 0xc) != -1 && (0 < *(int *)(in_ECX + 0x68))))) {
      if ((*(uint *)(in_ECX + 0x20) & 2) == 0) {
        in_EAX[1] = 0;
        return;
      }
      iVar5 = (int)*in_EAX - (int)sVar3;
      if (iVar5 < 0) {
        in_EAX[1] = 0;
        *(bool *)(in_EAX + 0x1c) = cVar4 == '\0';
        return;
      }
      iVar1 = *(int *)(in_ECX + 0x68) + -1;
      if (iVar1 < iVar5) {
        iVar5 = iVar1;
      }
      in_EAX[1] = (short)iVar5;
      *(bool *)(in_EAX + 0x1c) = cVar4 == '\0';
      return;
    }
    *in_EAX = -1;
    in_EAX[1] = -1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
