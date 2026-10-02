// ai_squad_resolve_actor_type  (Ghidra: ai_squad_resolve_actor_type; named for this rewrite)
// address 0x4374a0, size 97 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: resolves ScenarioSquad.actor_type (+0x20, the same field ai_reference_spawn_
// starting_location_object and ai_squad_find_best_matching_member already use) through
// Scenario.actor_palette to the ActorVariant tag, through its actor_definition (+0x10) to
// the Actor tag, and returns Actor+0x14 (types/ai.h's own established source for
// actor.type), defaulting to 14 if any step is invalid. Matches the phase-4 summary
// ("resolves the activity/behavior type ... defaulting to a fallback type when
// unresolved").
// register convention: Ghidra could not resolve the parameter at all.
//   // blam-cc: ECX -> squad

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

int16_t ai_squad_resolve_actor_type(ScenarioSquad *squad)
{
    int16_t actor_palette_index = (int16_t)squad->actor_type;

    if (actor_palette_index >= 0 && actor_palette_index < global_scenario->actor_palette.count) {
        TagDependency *entry = &((TagDependency *)global_scenario->actor_palette.pointer)[actor_palette_index];
        datum_index actor_variant_tag = *(datum_index *)&entry->tag_id;
        if (actor_variant_tag != (datum_index)k_datum_index_none) {
            uint8_t *actor_variant_data = (uint8_t *)tag_instances[actor_variant_tag & 0xffff].data;
            datum_index actor_definition_tag = *(datum_index *)(actor_variant_data + 0x10);
            if (actor_definition_tag != (datum_index)k_datum_index_none) {
                uint8_t *actor_tag_data = (uint8_t *)tag_instances[actor_definition_tag & 0xffff].data;
                return *(int16_t *)(actor_tag_data + 0x14);
            }
        }
    }
    return 0xe;
}

#if 0
Original Ghidra decompilation (0x4374a0):

undefined2 FUN_004374a0(void)

{
  short sVar1;
  uint uVar2;
  undefined2 uVar3;
  int in_ECX;

  sVar1 = *(short *)(in_ECX + 0x20);
  uVar3 = 0xe;
  if ((((-1 < sVar1) && ((int)sVar1 < *(int *)(global_scenario + 0x420))) &&
      (uVar2 = *(uint *)(sVar1 * 0x10 + *(int *)(global_scenario + 0x424) + 0xc),
      uVar2 != 0xffffffff)) &&
     (uVar2 = *(uint *)(*(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x10),
     uVar2 != 0xffffffff)) {
    uVar3 = *(undefined2 *)(*(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x14);
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
