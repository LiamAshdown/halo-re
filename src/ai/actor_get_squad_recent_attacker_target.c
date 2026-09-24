// actor_get_squad_recent_attacker_target  (Ghidra: actor_get_squad_recent_attacker_target; named from out/phase2/results/ai_02.json)
// address 0x41f6b0, size 274 bytes, 0 callers in this build
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase2/results/ai_02.json -- nearly identical structure to
//   actor_get_relevant_squad_member_target (0x41f550) but iterates the actor's own controlled
//   unit's recent-attacker table (unit.recent_damage[4] at unit+0x430) instead of a specific
//   squad member's, and resolves the responsible object through object_try_and_get(3) rather
//   than a manual datum_get. Unlike the sibling, there is no fallback to the raw responsible
//   unit handle when neither gunner nor driver is set -- the entry is simply skipped.
// register convention: EAX -> actor_index (Ghidra's recognized param_1); param_2 (char,
//   require_is_unit) is the other recognized stack parameter.
//   // blam-cc: stack -> actor_index, require_is_unit (both are Ghidra-recognized parameters
//   //   here; no separate register operand is read)
//
// UNSURE: object_try_and_get(3) is called fresh on every loop iteration with the same literal
// argument, ignoring the per-entry responsible_unit value entirely; this is preserved exactly,
// odd as it looks -- see actor_get_relevant_squad_member_target.c for the sibling that does use
// the per-entry value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0
extern data_array *prop_data;   // 0x008802c0

extern datum_index actor_find_prop_for_object(datum_index object_index); // 0x43ea80, UNSURE signature
extern void *object_try_and_get(int32_t kind);              // 0x4f6ec0

// blam-cc: stack -> actor_index, require_is_unit
// Selects the most relevant recent-attacker prop from the actor's own object's short-term
// attacker memory. Returns the winning prop's datum index, or k_datum_index_none.
datum_index actor_get_squad_recent_attacker_target(datum_index actor_index, char require_is_unit)
{
    actor *self;
    datum_index unit_index;
    unit_data *self_unit;
    int32_t i;
    datum_index responsible;
    object *ctx_obj;
    datum_index gunner;
    datum_index driver;
    datum_index resolved_object;
    datum_index resolved_prop;
    prop *candidate;
    int32_t best_tick;
    datum_index best_prop;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    unit_index = self->unit_index;
    best_prop = k_datum_index_none;

    if (unit_index == k_datum_index_none) {
        return k_datum_index_none;
    }

    best_tick = 0;
    self_unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data +
                              k_unit_data_offset);

    for (i = 0; i < k_unit_recent_damage_count; i++) {
        responsible = self_unit->recent_damage[i].responsible_unit;
        resolved_object = k_datum_index_none;

        if (responsible != k_datum_index_none) {
            ctx_obj = (object *)object_try_and_get(3);
            if (ctx_obj != (object *)0) {
                // NOTE: object_try_and_get's result is read directly at +0x328/+0x324, exactly
                // as the original does -- these are unit_data.gunner_unit_index/
                // driver_unit_index offsets, but applied straight to whatever
                // object_try_and_get(3) returns rather than through k_unit_data_offset.
                gunner = *(datum_index *)((uint8_t *)ctx_obj + 0x328);
                if (gunner == k_datum_index_none) {
                    driver = *(datum_index *)((uint8_t *)ctx_obj + 0x324);
                    if (driver != k_datum_index_none) {
                        resolved_object = driver;
                    }
                } else {
                    resolved_object = gunner;
                }
            }
        }

        if (resolved_object != k_datum_index_none) {
            resolved_prop = actor_find_prop_for_object(resolved_object);
            if (resolved_prop != k_datum_index_none) {
                candidate = (prop *)((uint8_t *)prop_data->data + (resolved_prop & 0xffff) * sizeof(prop));
                if ((1 < candidate->kind && candidate->kind < 4) &&
                    ((candidate->is_unit != 0 || require_is_unit == 0) &&
                     (best_tick < self_unit->recent_damage[i].tick))) {
                    best_tick = self_unit->recent_damage[i].tick;
                    best_prop = resolved_prop;
                }
            }
        }
    }

    return best_prop;
}

#if 0
Original Ghidra decompilation (0x41f6b0):

uint FUN_0041f6b0(uint param_1,char param_2)

{
  short sVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint *puVar8;
  int local_8;
  uint local_4;

  iVar2 = DAT_008802c0;
  uVar6 = *(uint *)((param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34) + 0x18);
  local_4 = 0xffffffff;
  if (uVar6 != 0xffffffff) {
    uVar7 = 0;
    puVar8 = (uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc) + 0x430);
    local_8 = 4;
    do {
      uVar6 = puVar8[2];
      if ((((uVar6 != 0xffffffff) && (iVar3 = object_try_and_get(3), iVar3 != 0)) &&
          (((uVar4 = *(uint *)(iVar3 + 0x328), uVar4 == 0xffffffff &&
            (uVar5 = *(uint *)(iVar3 + 0x324), uVar4 = uVar6, uVar5 != 0xffffffff)) ||
           (uVar5 = uVar4, uVar4 != 0xffffffff)))) &&
         (uVar6 = FUN_0043ea80(uVar5), uVar6 != 0xffffffff)) {
        iVar3 = (uVar6 & 0xffff) * 0x138 + *(int *)(iVar2 + 0x34);
        sVar1 = *(short *)(iVar3 + 0x24);
        if ((((1 < sVar1) && (sVar1 < 4)) &&
            ((*(char *)(iVar3 + 0x60) != '\0' || (param_2 == '\0')))) && (uVar7 < *puVar8)) {
          uVar7 = *puVar8;
          local_4 = uVar6;
        }
      }
      puVar8 = puVar8 + 4;
      local_8 = local_8 + -1;
    } while (local_8 != 0);
    return local_4;
  }
  return 0xffffffff;
}
#endif
