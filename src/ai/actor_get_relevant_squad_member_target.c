// actor_get_relevant_squad_member_target  (Ghidra: actor_get_relevant_squad_member_target; named from out/phase2/results/ai_02.json)
// address 0x41f550, size 345 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase2/results/ai_02.json -- iterates a squad member's fixed 4-entry
//   recent-attacker table (unit.recent_damage[4] at unit+0x430, types/units.h), resolving each
//   attacker (preferring a vehicle's gunner, then driver, then the attacker itself) via
//   actor_find_prop_for_object to a prop (target-data) record, and keeping the entry with the highest tick
//   whose prop kind is 2/3 and (is_unit or !require_is_unit).
// register convention: EAX -> member_prop_index; param_1 (unused by this function's own body,
//   kept for signature fidelity) and param_2 (char, require_is_unit) are Ghidra's recognized
//   stack parameters.
//   // blam-cc: EAX -> member_prop_index, stack -> unused_param, require_is_unit
//
// UNSURE: the datum-validity check inlined here (object_header identifier vs a salt taken from
// the high 16 bits of the recent_damage responsible_unit field) is a manual datum_get; written
// out explicitly rather than calling datum_get, since the salt handling (wildcard 0) differs
// slightly from that helper's own.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *prop_data;   // 0x008802c0
extern data_array *object_data; // 0x008603b0

extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, ECX actor, stack object

// blam-cc: EAX -> member_prop_index, stack -> unused_param, require_is_unit
// Selects the most relevant recent-attacker prop from a specific squad member's short-term
// memory table (its tracked unit's recent_damage entries), preferring the most recent hit.
// Returns the winning prop's datum index, or k_datum_index_none if none qualified.
datum_index actor_get_relevant_squad_member_target(uint32_t unused_param, datum_index member_prop_index, char require_is_unit)
{
    prop *member;
    object_header *headers;
    unit_data *member_unit;
    int32_t i;
    datum_index responsible;
    int16_t responsible_index;
    int16_t responsible_salt;
    int16_t header_identifier;
    object_header *attacker_header;
    object *attacker_obj;
    unit_data *attacker_unit;
    datum_index resolved_object;
    datum_index resolved_prop;
    prop *candidate;
    int32_t best_tick;
    datum_index best_prop;

    (void)unused_param;

    member = (prop *)((uint8_t *)prop_data->data + (member_prop_index & 0xffff) * sizeof(prop));
    member_unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[member->object_index & 0xffff].data +
                                 k_unit_data_offset);

    best_prop = k_datum_index_none;
    best_tick = 0;

    for (i = 0; i < k_unit_recent_damage_count; i++) {
        responsible = member_unit->recent_damage[i].responsible_unit;
        if (responsible != k_datum_index_none) {
            resolved_object = k_datum_index_none;
            responsible_index = (int16_t)responsible;
            if (-1 < responsible_index && responsible_index < object_data->maximum_count) {
                headers = (object_header *)object_data->data;
                header_identifier = headers[responsible_index].identifier;
                responsible_salt = (int16_t)(responsible >> 16);
                if (header_identifier != 0 && (responsible_salt == 0 || header_identifier == responsible_salt)) {
                    resolved_object = responsible; // UNSURE: the original keeps the *combined*
                                                   // handle (iVar6), not just the validated index
                    attacker_header = &headers[responsible_index];

                    if ((1 << (attacker_header->type & 0x1f) & 3) != 0) {
                        attacker_obj = attacker_header->data;
                        if (attacker_obj != (object *)0) {
                            attacker_unit = (unit_data *)((uint8_t *)attacker_obj + k_unit_data_offset);
                            resolved_object = attacker_unit->gunner_unit_index;
                            if (resolved_object == k_datum_index_none) {
                                resolved_object = responsible;
                                if (attacker_unit->driver_unit_index != k_datum_index_none) {
                                    resolved_object = attacker_unit->driver_unit_index;
                                }
                            }
                        } else {
                            resolved_object = k_datum_index_none;
                        }
                    } else {
                        resolved_object = k_datum_index_none;
                    }
                }
            }

            if (resolved_object != k_datum_index_none) {
                resolved_prop = actor_find_prop_for_object(resolved_object, (datum_index)unused_param); // 0x41f62c: ECX = stack arg 1 (the asking actor)
                if (resolved_prop != k_datum_index_none) {
                    candidate = (prop *)((uint8_t *)prop_data->data + (resolved_prop & 0xffff) * sizeof(prop));
                    if ((1 < candidate->kind && candidate->kind < 4) &&
                        ((candidate->is_unit != 0 || require_is_unit == 0) &&
                         (best_tick < member_unit->recent_damage[i].tick))) {
                        best_tick = member_unit->recent_damage[i].tick;
                        best_prop = resolved_prop;
                    }
                }
            }
        }
    }

    return best_prop;
}

#if 0
Original Ghidra decompilation (0x41f550):

uint FUN_0041f550(undefined4 param_1,char param_2)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  int *piVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  short sVar7;
  short sVar8;
  int iVar9;
  int local_c;
  int local_8;
  uint local_4;

  iVar1 = *(int *)(DAT_008802c0 + 0x34);
  iVar2 = *(int *)(DAT_008603b0 + 0x34);
  piVar3 = (int *)(*(int *)(iVar2 + 8 +
                           (*(uint *)((in_EAX & 0xffff) * 0x138 + 0x18 + iVar1) & 0xffff) * 0xc) +
                  0x438);
  local_4 = 0xffffffff;
  local_c = 0;
  local_8 = 4;
  do {
    iVar6 = *piVar3;
    if (iVar6 != -1) {
      iVar9 = 0;
      sVar7 = (short)iVar6;
      if ((-1 < sVar7) && (sVar7 < *(short *)(DAT_008603b0 + 0x20))) {
        iVar4 = (int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar7;
        sVar7 = *(short *)(iVar4 + iVar2);
        if ((sVar7 != 0) && ((sVar8 = (short)((uint)iVar6 >> 0x10), sVar8 == 0 || (sVar7 == sVar8)))
           ) {
          iVar9 = iVar4 + iVar2;
        }
      }
      if (((iVar9 != 0) && ((1 << (*(byte *)(iVar9 + 3) & 0x1f) & 3U) != 0)) &&
         (iVar9 = *(int *)(iVar9 + 8), iVar9 != 0)) {
        iVar4 = *(int *)(iVar9 + 0x328);
        if ((iVar4 == -1) && (iVar4 = iVar6, *(int *)(iVar9 + 0x324) != -1)) {
          iVar4 = *(int *)(iVar9 + 0x324);
        }
        if ((iVar4 != -1) && (uVar5 = FUN_0043ea80(iVar4), uVar5 != 0xffffffff)) {
          iVar6 = (uVar5 & 0xffff) * 0x138;
          sVar7 = *(short *)(iVar6 + 0x24 + iVar1);
          if (((1 < sVar7) && (sVar7 < 4)) &&
             (((*(char *)(iVar6 + iVar1 + 0x60) != '\0' || (param_2 == '\0')) &&
              (local_c < piVar3[-2])))) {
            local_c = piVar3[-2];
            local_4 = uVar5;
          }
        }
      }
    }
    piVar3 = piVar3 + 4;
    local_8 = local_8 + -1;
  } while (local_8 != 0);
  return local_4;
}
#endif
