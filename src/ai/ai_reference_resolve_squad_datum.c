// ai_reference_resolve_squad_datum  (Ghidra: ai_reference_resolve_squad_datum; named for this rewrite)
// address 0x432c80, size 171 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// evidence: resolves a packed ai reference to a concrete actor-placement slot index within
// the owning encounter's squad definition (matching the phase-4 summary) by calling
// encounter_squad_spawn_reinforcement (outside this rewrite's range) once the squad is known. Two conditions in
// Ghidra's own decompile are never actually reachable: `param_1._2_1_ != 0xffff` compares a
// (sign-extended) byte against a 16-bit constant a byte can never equal, so the kind==2
// path always falls into the encounter_squad_spawn_reinforcement call; and `(short)iVar4 == -1` tests a small loop
// counter that can never realistically reach 0xffff. Both are kept exactly as decompiled
// rather than simplified away, since Ghidra's rendering, however oddly, is a faithful
// (if redundant) copy of the real comparisons the compiler emitted.
// UNSURE, substantially: on every early-exit path (invalid globals/reference, or a kind==1
// reference whose platoon has no matching squad), the original returns whatever was in
// local iVar2 at that point, which for the "no match" case is never reassigned from its
// initial value of DAT_00880354 itself (the ai_globals pointer, not a squad-derived value).
// This looks like a genuine decompiler/register-reuse artifact rather than an intentional
// sentinel, but it is reproduced exactly as a defensive default rather than guessed away.
// register convention: Ghidra fully resolved the parameter.
//   // blam-cc: stack -> packed_reference (EAX, per sibling convention; not independently
//   confirmed with objdump for this file)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354
extern Scenario *global_scenario;  // 0x00746f8c

extern uint32_t encounter_squad_spawn_reinforcement(datum_index encounter_index, int16_t squad_index); // 0x438f60, ECX, AX

// blam-cc: stack -> packed_reference
int32_t ai_reference_resolve_squad_datum(uint32_t packed_reference)
{
    int32_t fallback = (int32_t)ai_globals_ptr; // see UNSURE note above

    if (ai_globals_ptr->actors_valid != 0 && packed_reference != (uint32_t)k_datum_index_none) {
        if (packed_reference >> 0x1e == 2) {
            // 0x432cb2: the squad byte, zero-extended (never 0xffff); tail jump with ECX = encounter, AX = squad
            return (int32_t)encounter_squad_spawn_reinforcement(packed_reference & 0xffff,
                (int16_t)((packed_reference >> 0x10) & 0xff));
        }
        if (packed_reference >> 0x1e == 1) {
            ScenarioEncounter *encounter_definition =
                &((ScenarioEncounter *)global_scenario->encounters.pointer)[packed_reference & 0xffff];
            int32_t squad_count = encounter_definition->squads.count;

            if (squad_count > 0) {
                ScenarioSquad *squads = (ScenarioSquad *)encounter_definition->squads.pointer;
                int32_t squad_index = 0;
                do {
                    if ((int32_t)squads[squad_index].platoon == (int32_t)((packed_reference >> 0x10) & 0xff)) {
                        if ((int16_t)squad_index == -1) { // never true in practice; see file header
                            return squad_index;
                        }
                        return (int32_t)encounter_squad_spawn_reinforcement(packed_reference & 0xffff,
                            (int16_t)squad_index); // 0x432d26: ECX = encounter, AX = the first matching squad
                    }
                    squad_index = squad_index + 1;
                } while (squad_index < squad_count);
            }
        }
    }
    return fallback;
}

#if 0
Original Ghidra decompilation (0x432c80):

int FUN_00432c80(uint param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;

  iVar2 = DAT_00880354;
  if ((*(char *)(DAT_00880354 + 1) != '\0') && (param_1 != 0xffffffff)) {
    if (param_1 >> 0x1e == 2) {
      iVar2 = CONCAT22((short)((uint)DAT_00880354 >> 0x10),(ushort)param_1._2_1_);
      if (param_1._2_1_ != 0xffff) {
LAB_00432d23:
        iVar2 = FUN_00438f60();
        return iVar2;
      }
    }
    if (param_1 >> 0x1e == 1) {
      iVar2 = (param_1 & 0xffff) * 0xb0;
      iVar1 = *(int *)(iVar2 + 0x80 + *(int *)(global_scenario + 0x430));
      iVar2 = iVar2 + *(int *)(global_scenario + 0x430);
      iVar4 = 0;
      if (0 < iVar1) {
        iVar2 = *(int *)(iVar2 + 0x84);
        iVar3 = 0;
        do {
          if ((int)*(short *)(iVar3 * 0xe8 + 0x22 + iVar2) == (param_1 >> 0x10 & 0xff)) {
            if ((short)iVar4 == -1) {
              return iVar4;
            }
            goto LAB_00432d23;
          }
          iVar4 = iVar4 + 1;
          iVar3 = (int)(short)iVar4;
        } while (iVar3 < iVar1);
      }
    }
  }
  return iVar2;
}
#endif
