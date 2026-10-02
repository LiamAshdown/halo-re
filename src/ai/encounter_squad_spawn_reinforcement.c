// encounter_squad_spawn_reinforcement  (Ghidra: encounter_squad_spawn_reinforcement, renamed)
// address 0x438f60, size 317 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x438f60..0x43909c; the two respawn timers rewritten)
// evidence: types/ai.h encounter (0x6c), encounter_squad_state (0x20, addressed as
//   encounter_squad_states[encounter.first_squad+squad_index]); types/tags.h ScenarioSquad
//   (respawn_total confirmed at +0x88 with offsetof()). Calls
//   encounter_squad_spawn_actor @0x438e20 (this rewrite). The two `__ftol()`-with-no-visible-
//   operand truncations reuse the exact k_random_scale_65536 / ticks_per_second globals from
//   the same reconstruction already used in actor_recompute_grenade_eligibility.c, and this
//   function's own "globals referenced" list independently names both (0x672b84, 0x672ac8),
//   which supports that reconstruction here too.
// register convention: ECX -> encounter_index, AX -> squad_index (both `in_`-prefixed
//   unaffected registers in Ghidra's decompile, i.e. not recovered as named parameters).
//   // blam-cc: ECX -> encounter_index, EAX(low16) -> squad_index
//
// UNSURE: the low byte of the return value is unconditionally masked to zero by
// `& 0xffffff00` on every path, including the success path (where the pre-mask value is a
// small 0/1 bool from encounter_squad_spawn_actor, so ANDing with 0xffffff00 always yields
// 0 there too). Only the "spawn suppressed" early-out path can return something nonzero (the
// raw ai_globals pointer value with its low byte cleared). This looks like a real quirk of
// the compiled function rather than a decompilation error -- every caller that stores the
// result in a `char` (see encounter_process_squad_reinforcements / encounter_decay_squad_spawn_delays-style callers) observes 0 in the
// common case regardless of whether the spawn actually succeeded. Preserved verbatim rather
// than "fixed" to a clean bool.
// UNSURE: encounter_squad_state.maneuver_distance (+0x0c) is decremented here as a spawn
// budget counter, which does not match the name/evidence recorded for that field elsewhere
// in types/ai.h (a scenario-derived distance threshold). Either the field is reused for two
// purposes at runtime, or the two attributions conflict; not resolved here.
// UNSURE: encounter_squad_spawn_actor's own signature takes 3 parameters (encounter_index, squad_index,
// unit_type_index) but this call site shows none of them explicitly (register-forwarding,
// as ECX/AX above), and the third (unit_type_index) has no traceable source in this
// function at all. Declared and called here with only the two register-forwarded values.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern ai_globals *ai_globals_ptr;  // 0x00880354
extern data_array *encounter_data;  // 0x008802c8
extern Scenario *global_scenario;   // 0x00746f8c
extern encounter_squad_state *encounter_squad_states; // 0x008802cc
extern uint32_t random_seed_global; // 0x00719cd0
extern float k_random_scale_65536;  // 0x00672b84, 1.5259022e-05 = 1/65536
extern float ticks_per_second;      // 0x00672ac8, 30.0

extern uint8_t encounter_squad_spawn_actor(datum_index encounter_index, int16_t squad_index, uint32_t unit_type_index,
    uint32_t unused); // 0x438e20, all stack

// blam-cc: ECX -> encounter_index, EAX(low16) -> squad_index
uint32_t encounter_squad_spawn_reinforcement(datum_index encounter_index, int16_t squad_index)
{
    encounter *self;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    encounter_squad_state *squad_state;
    uint32_t result;
    float randomized;

    result = (uint32_t)ai_globals_ptr;
    if (ai_globals_ptr->actors_valid != 0) {
        result = encounter_squad_spawn_actor(encounter_index, squad_index, 0, 1); // FIXED: pushes (ECX, EAX, 0, 1) at 0x438f7a
        if ((int8_t)result != 0) {
            self = (encounter *)((uint8_t *)encounter_data->data + (encounter_index & 0xffff) * sizeof(encounter));
            encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index & 0xffff];
            squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];
            squad_state = &encounter_squad_states[(int16_t)(self->first_squad + squad_index)];

            self->living_count = self->living_count + 1;
            squad_state->living_count = squad_state->living_count + 1;
            if (0 < squad_definition->respawn_total) {
                squad_state->respawn_budget = squad_state->respawn_budget - 1; // see header UNSURE
            }

            // 0x438ff8..0x439040: encounter timer = (r * (max - min) + min) * 30 ticks with the ENCOUNTER definition's
            // range at +0x2c / +0x30; 0x439044..0x439090: squad timer likewise from the SQUAD definition's +0x8c / +0x90.
            // FIXED 2026-09-27: the draft computed r + 30.0 (about one second) for both.
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            {
                float lo = *(float *)((uint8_t *)encounter_definition + 0x2c);
                float hi = *(float *)((uint8_t *)encounter_definition + 0x30);
                float r = (float)((uint32_t)random_seed_global >> 0x10) * k_random_scale_65536;

                randomized = (r * (hi - lo) + lo) * ticks_per_second;
                self->respawn_delay_ticks = (int16_t)(int32_t)randomized;
            }

            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            {
                float lo = *(float *)((uint8_t *)squad_definition + 0x8c);
                float hi = *(float *)((uint8_t *)squad_definition + 0x90);
                float r = (float)((uint32_t)random_seed_global >> 0x10) * k_random_scale_65536;

                randomized = (r * (hi - lo) + lo) * ticks_per_second;
                squad_state->respawn_delay_ticks = (int16_t)(int32_t)randomized;
            }
        }
    }
    return result & 0xffffff00;
}

#if 0
// ---- original Ghidra decompilation (FUN_00438f60 @ 0x438f60) ----
uint FUN_00438f60(void)

{
  int iVar1;
  short in_AX;
  undefined2 uVar2;
  uint uVar3;
  uint in_ECX;
  int iVar4;
  int iVar5;

  uVar3 = DAT_00880354;
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    uVar3 = FUN_00438e20();
    if ((char)uVar3 != '\0') {
      iVar5 = (in_ECX & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
      iVar1 = *(int *)((in_ECX & 0xffff) * 0xb0 + 0x84 + *(int *)(global_scenario + 0x430));
      iVar4 = (short)(*(short *)(iVar5 + 4) + in_AX) * 0x20 + DAT_008802cc;
      *(short *)(iVar5 + 0x2a) = *(short *)(iVar5 + 0x2a) + 1;
      *(short *)(iVar4 + 0x18) = *(short *)(iVar4 + 0x18) + 1;
      if (0 < *(short *)(in_AX * 0xe8 + iVar1 + 0x88)) {
        *(short *)(iVar4 + 0xc) = *(short *)(iVar4 + 0xc) + -1;
      }
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      uVar2 = __ftol();
      *(undefined2 *)(iVar5 + 0x3e) = uVar2;
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      uVar3 = __ftol();
      *(short *)(iVar4 + 0xe) = (short)uVar3;
    }
  }
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
