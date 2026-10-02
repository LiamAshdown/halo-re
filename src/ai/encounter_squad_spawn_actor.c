// encounter_squad_spawn_actor  (Ghidra: encounter_squad_spawn_actor, renamed)
// address 0x438e20, size 316 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/tags.h ScenarioEncounter.squads (0x80) / ScenarioSquad (0xe8, actor_type at
//   0x20, starting_locations reflexive at 0xd0) / ScenarioActorStartingLocation (0x1c,
//   actor_type at 0x18) / Scenario.actor_palette (0x420) / ScenarioActorPalette (0x10,
//   TagDependency at 0x0, tag_id at 0xc) -- all offsets confirmed against the compiled
//   layout with offsetof(). The palette entry's tag_id resolves an "actor_variant" tag
//   (ScenarioActorPalette's own comment); ActorVariant+0x30 is major_variant's TagDependency
//   tag_id (flags 0x4 + actor_definition TagDependency 0x10 + unit TagDependency 0x10 = 0x24,
//   + tag_fourcc/path_pointer/path_size = 0x30), so the guarded call is "if this variant has
//   a major/elite variant, consult the difficulty request first". phase-4 summary "spawns a
//   single actor unit for a squad at a resolved starting location, mapping the location to
//   its placed object type first." squad_pick_random_starting_location @0x437220 (already
//   named, not yet rewritten in this pass) and ai_get_difficulty_request @0x42a950 (already
//   rewritten, src/ai/ai_get_difficulty_request.c) and actor_place_new_unit @0x427080
//   (already rewritten, src/ai/actor_place_new_unit.c).
// register convention: EAX -> encounter_index, AX(low16 of param_2) -> squad_index, stack ->
//   an opaque undefined4 forwarded verbatim to actor_place_new_unit's unit_type_index slot.
//   // blam-cc: EAX -> encounter_index, ECX -> squad_index (Ghidra shows it as the second
//   //   formal parameter, `short param_2`, not a raw register name, so the exact source
//   //   register is inferred from calling convention order), stack -> unit_type_index
//
// UNSURE: three call sites in this function (squad_pick_random_starting_location, called
// twice, and ai_get_difficulty_request/actor_place_new_unit's leading EAX "placement"
// argument) show fewer live operands in Ghidra's decompilation than those callees' own
// established signatures take elsewhere in this module. This means part of their argument
// list was already sitting in a register from earlier in this function and Ghidra did not
// attribute it to the call. The declarations below match exactly what THIS call site shows;
// they are deliberately narrower than the canonical prototypes used in
// squad_pick_random_starting_location's own file (not yet rewritten) and in
// actor_place_new_unit.c / ai_get_difficulty_request.c, and the missing operands could not
// be recovered without reading raw disassembly. Also: `local_c`, the use_palette_entry
// argument, is read here with its top 24 bits left as whatever was already on the stack
// (only the low byte is cleared to 0 before use) -- kept verbatim from Ghidra rather than
// "fixed", since actor_place_new_unit only reads it as a single byte.

// REWRITTEN (from objdump 0x438e20..0x438f5b): all four arguments are on the stack (encounter, squad, unit type,
//   an unused fourth). A starting location comes from squad_pick_random_starting_location(ECX encounter, AX
//   squad); its actor type (+0x18) overrides the squad's (+0x20) unless -1. The actor palette entry (scenario
//   +0x420 count, 0x10 each at +0x424) must name a variant (+0xc). When the variant has a major variant (+0x30),
//   ai_get_difficulty_request(AX = squad +0x80, ECX &enabled, EDX &use_palette, ESI &bias) and, when enabled,
//   use_palette = ai_drift_zone_bias(EAX encounter, stack squad, bias). actor_place_new_unit(EAX = the starting
//   location, stack: variant, encounter, squad, use_palette, unit type) != -1 is returned. The draft had the
//   registers backwards and called the location picker and the difficulty request with no arguments.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario;   // 0x00746f8c
extern tag_instance *tag_instances; // 0x0087bc14

extern int16_t squad_pick_random_starting_location(datum_index encounter_index, int16_t squad_index);
    // 0x437220, blam-cc: AX -> squad_index, ECX -> encounter_index
extern void ai_get_difficulty_request(int16_t request_code, uint8_t *out_flag_a, uint8_t *out_flag_b, float *out_value);
    // 0x42a950, blam-cc: AX, ECX, EDX, ESI
extern uint8_t ai_drift_zone_bias(datum_index encounter_index, int16_t squad_offset, float bias);
    // 0x42a9d0, blam-cc: EAX -> encounter_index, stack -> squad_offset, bias
extern datum_index actor_place_new_unit(datum_index actor_variant_or_palette_tag, datum_index encounter_index,
    int16_t squad_index, uint8_t use_palette_entry, uint16_t unit_type_index,
    const actor_placement_request *placement_request); // 0x427080, blam-cc: EAX -> placement_request, stack -> rest

uint8_t encounter_squad_spawn_actor(datum_index encounter_index, int16_t squad_index, uint32_t unit_type_index,
    uint32_t unused)
{
    uint8_t *encounter_definition = (uint8_t *)global_scenario->encounters.pointer + (encounter_index & 0xffff) * 0xb0;
    uint8_t *squad = *(uint8_t **)(encounter_definition + 0x84) + squad_index * 0xe8;
    uint8_t *starting_location;
    uint8_t *palette_entry;
    datum_index variant_tag;
    int16_t location_index;
    int16_t palette_index;
    uint8_t use_palette = 0;

    location_index = squad_pick_random_starting_location(encounter_index, squad_index);
    if (location_index == -1) {
        return 0;
    }
    starting_location = *(uint8_t **)(squad + 0xd4) + location_index * 0x1c;
    palette_index = *(int16_t *)(squad + 0x20);
    if (*(int16_t *)(starting_location + 0x18) != -1) {
        palette_index = *(int16_t *)(starting_location + 0x18);
    }
    if (palette_index < 0 || palette_index >= (int32_t)global_scenario->actor_palette.count) {
        return 0;
    }
    palette_entry = (uint8_t *)global_scenario->actor_palette.pointer + palette_index * 0x10;
    variant_tag = *(datum_index *)(palette_entry + 0xc);
    if (variant_tag == k_datum_index_none) {
        return 0;
    }
    if (*(datum_index *)((uint8_t *)tag_instances[variant_tag & 0xffff].data + 0x30) != k_datum_index_none) {
        uint8_t enabled = 0;
        float bias = 0.0f;

        ai_get_difficulty_request(*(int16_t *)(squad + 0x80), &enabled, &use_palette, &bias);
        if (enabled) {
            use_palette = ai_drift_zone_bias(encounter_index, squad_index, bias);
        }
    }
    return actor_place_new_unit(variant_tag, encounter_index, squad_index, use_palette, (uint16_t)unit_type_index,
        (const actor_placement_request *)starting_location) != k_datum_index_none;
}

#if 0
// ---- original Ghidra decompilation (FUN_00438e20 @ 0x438e20) ----
/* WARNING: Removing unreachable block (ram,0x00438f08) */

bool FUN_00438e20(uint param_1,short param_2,undefined4 param_3)

{
  uint uVar1;
  short sVar2;
  int iVar3;
  short sVar4;
  int iVar5;
  uint local_c;

  iVar3 = global_scenario;
  iVar5 = param_2 * 0xe8 +
          *(int *)((param_1 & 0xffff) * 0xb0 + 0x84 + *(int *)(global_scenario + 0x430));
  sVar2 = squad_pick_random_starting_location();
  if (sVar2 != -1) {
    sVar2 = *(short *)(sVar2 * 0x1c + *(int *)(iVar5 + 0xd4) + 0x18);
    sVar4 = *(short *)(iVar5 + 0x20);
    if (sVar2 != -1) {
      sVar4 = sVar2;
    }
    if ((-1 < sVar4) && ((int)sVar4 < *(int *)(iVar3 + 0x420))) {
      iVar3 = sVar4 * 0x10 + *(int *)(iVar3 + 0x424);
      uVar1 = *(uint *)(iVar3 + 0xc);
      if (uVar1 != 0xffffffff) {
        local_c = local_c & 0xffffff00;
        if (*(int *)(*(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x30) != -1) {
          FUN_0042a950();
        }
        iVar3 = actor_place_new_unit
                          (*(undefined4 *)(iVar3 + 0xc),param_1,(int)param_2,local_c,param_3);
        return iVar3 != -1;
      }
    }
  }
  return false;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
