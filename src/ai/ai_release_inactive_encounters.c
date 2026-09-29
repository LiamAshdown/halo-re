// ai_release_inactive_encounters  (Ghidra: ai_release_inactive_encounters, already named)
// address 0x42ae50, size 241 bytes
// name confidence: 0.9   rewrite confidence: 0.85 (release calls FIXED against objdump)
// evidence: cea-pdb hint via the two format strings "encounter %s (%d units)" and
// "encounterless-actor %s"; the caller-owned iterator record (count, cursor, then 12-byte
// entries) is not modeled in types/ai.h, so it is handled here with explicit pointer
// arithmetic exactly as Ghidra decompiled it, rather than inventing a local struct for a
// one-function record.
// register convention: plain __cdecl, all three arguments on the stack (buffer, has_more,
// state); the incoming EAX low byte is dead (only its top 24 bits, always zero on entry,
// survive into the low byte of the return value on the "did something" path).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;              // 0x00880360
extern data_array *encounter_data;          // 0x008802c8
extern tag_instance *tag_instances;         // 0x0087bc14
extern Scenario *global_scenario;           // 0x00746f8c

extern void ai_release_actors_filtered(datum_index encounter_index, int32_t platoon_index, int32_t squad_index,
    uint8_t is_dead); // 0x42ab00, EAX, EDI, stack, BL
    // UNSURE: the call site only supplies one visible stack argument (-1); the callee's own
    // decompile also reads unaff_EDI and in_EAX, which appear to be whatever this caller's
    // registers happen to hold rather than real inputs. Left as a single-argument prototype;
    // resolve together with 0x42ab00's own rewrite.
extern void actor_delete_or_release_unit(datum_index actor_index, uint8_t is_dead); // 0x4288e0, stack, AL

// Iterates a prioritized list of things AI cleanup still needs to release -- whole encounters
// and encounterless (unassigned) actors interleaved -- one entry per call. state points at a
// caller-owned iterator: a int16 count, a int16 cursor, then 12-byte entries (a kind word, 2
// pad bytes, and a datum index at +4..+7, 4 unused trailing bytes). Formats a status string
// describing what is about to be released, releases it, and reports through *has_more whether
// entries remain.
int32_t ai_release_inactive_encounters(char *buffer, uint8_t *has_more, int16_t *state)
{
    int16_t *entry;
    uint32_t index;
    ScenarioEncounter *scenario_encounter;
    encounter *runtime_encounter;
    actor *self;
    char *path;
    char *file_name;
    int32_t result;

    result = 0;
    if (state[1] < state[0]) {
        entry = state + state[1] * 6 + 2; // state + 4 bytes + cursor * 12 bytes
        index = *(uint32_t *)(entry + 2);
        if ((char)*entry == '\0') {
            scenario_encounter = &((ScenarioEncounter *)global_scenario->encounters.pointer)[index & 0xffff];
            runtime_encounter = &((encounter *)encounter_data->data)[index & 0xffff];
            sprintf(buffer, "encounter %s (%d units)", scenario_encounter->name.string,
                    runtime_encounter->weighted_actor_count);
            // FIXED (objdump 0x42af13..0x42af1d): EAX = the encounter, EDI = -1, stack = -1, BL = 1. The draft passed
            //   only -1, which released every actor in the level instead of this encounter.
            ai_release_actors_filtered((datum_index)index, -1, -1, 1);
        } else {
            self = &((actor *)actor_data->data)[index & 0xffff];
            path = tag_instances[(int16_t)self->actor_variant_tag].path;
            file_name = strrchr(path, '\\');
            if (file_name != 0) {
                file_name = file_name + 1;
            } else {
                file_name = path;
            }
            sprintf(buffer, "encounterless-actor %s", file_name);
            actor_delete_or_release_unit(index, 1); // FIXED: AL = 1 (0x42aec8)
        }
        state[1] = state[1] + 1;
        result = (result & 0xffffff00) | 1;
    }
    *has_more = (uint8_t)(state[1] < state[0]);
    return result;
}

#if 0
Original Ghidra decompilation (0x42ae50):

int __cdecl ai_release_inactive_encounters(char *buffer,uchar *has_more,short *state)

{
  short *psVar1;
  uint in_EAX;
  uint uVar2;
  char *pcVar3;
  char *_Str;
  undefined4 uVar4;

  uVar2 = in_EAX & 0xffffff00;
  if (state[1] < *state) {
    psVar1 = state + state[1] * 6 + 2;
    if ((char)*psVar1 == '\0') {
      _sprintf(buffer,"encounter %s (%d units)",
               (*(uint *)(psVar1 + 2) & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430),
               (int)*(short *)((*(uint *)(psVar1 + 2) & 0xffff) * 0x6c + 0x2a +
                              *(int *)(DAT_008802c8 + 0x34)));
      uVar4 = FUN_0042ab00(0xffffffff);
    }
    else {
      _Str = *(char **)(*(short *)((*(uint *)(psVar1 + 2) & 0xffff) * 0x724 + 0x5c +
                                  *(int *)(DAT_00880360 + 0x34)) * 0x20 + 0x10 + DAT_0087bc14);
      pcVar3 = _strrchr(_Str,0x5c);
      if (pcVar3 != (char *)0x0) {
        _Str = pcVar3 + 1;
      }
      _sprintf(buffer,"encounterless-actor %s",_Str);
      uVar4 = actor_delete_or_release_unit(*(undefined4 *)(psVar1 + 2));
    }
    state[1] = state[1] + 1;
    uVar2 = CONCAT31((int3)((uint)uVar4 >> 8),1);
  }
  *has_more = state[1] < *state;
  return uVar2;
}
#endif
