// actor_get_squad_recent_attacker_target  (Ghidra: actor_get_squad_recent_attacker_target; named from out/phase2/results/ai_02.json)
// address 0x41f6b0, size 274 bytes, 0 callers in this build
// name confidence: 0.4   rewrite confidence: 0.95
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

extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack

// REWRITTEN from objdump 0x41f6b0..0x41f7c2. Stack: (actor, require_flag). Of the 4 recent damage records on the
//   actor's unit (+0x430, 0x10 each: +0x0 tick, +0x8 responsible object), picks the newest whose responsible unit
//   (its gunner +0x328, else driver +0x324, else itself) the actor has a kind 2..3 prop for (with +0x60 set when
//   require_flag). The draft called object_try_and_get and the prop lookup without the object or the actor and
//   skipped the unit itself.
// blam-cc: stack -> actor_index, require_is_unit
datum_index actor_get_squad_recent_attacker_target(datum_index actor_index, char require_is_unit)
{
    datum_index unit_index = *(datum_index *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724 + 0x18);
    datum_index best_prop = k_datum_index_none;     // [esp+0x14]
    uint32_t best_tick = 0;                         // ebp
    uint8_t *record;
    int32_t i;

    if (unit_index == k_datum_index_none) {
        return k_datum_index_none;
    }
    record = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data + 0x430;
    for (i = 4; i != 0; i--, record += 0x10) {
        datum_index responsible = *(datum_index *)(record + 0x8);
        uint8_t *unit;
        datum_index source;
        datum_index prop_index;
        uint8_t *p;

        if (responsible == k_datum_index_none) {
            continue;
        }
        unit = (uint8_t *)object_try_and_get(responsible, 3);
        if (unit == 0) {
            continue;
        }
        source = ((unit_object *)unit)->unit.gunner_unit_index;
        if (source == k_datum_index_none) {
            source = ((unit_object *)unit)->unit.driver_unit_index;
            if (source == k_datum_index_none) {
                source = responsible;
            }
        }
        if (source == k_datum_index_none) {
            continue;
        }
        prop_index = actor_find_prop_for_object(source, actor_index);
        if (prop_index == k_datum_index_none) {
            continue;
        }
        p = (uint8_t *)prop_data->data + (prop_index & 0xffff) * 0x138;
        if (*(int16_t *)(p + 0x24) < 2 || *(int16_t *)(p + 0x24) > 3) {
            continue;
        }
        if (!p[0x60] && require_is_unit) {
            continue;
        }
        if (*(uint32_t *)record > best_tick) {
            best_prop = prop_index;
            best_tick = *(uint32_t *)record;
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
