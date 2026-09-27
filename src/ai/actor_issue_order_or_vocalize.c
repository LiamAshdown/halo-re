// actor_issue_order_or_vocalize  (Ghidra: actor_issue_order_or_vocalize; named for this rewrite)
// address 0x4302e0, size 180 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: phase-4 summary ("issues an AI order (e.g. 'go to' or 'attack') targeting a
// specific unit, or a randomly-chosen fallback target if the unit is unsuitable"); the third
// argument to actor_begin_vocalization (actor_begin_vocalization, already rewritten in this repo) is
// built here as that function's own actor_vocalization_context (kind 1 = prop handle, kind
// 3 = the marker-position fallback unit_get_primary_eye_marker_position retrieves).
// register convention: EAX -> prop_index, EBX -> actor_index, EDI -> vehicle_object_index
// (all unresolved registers in Ghidra's own decompile).
// blam-cc: EAX -> prop_index, EBX -> actor_index, EDI -> vehicle_object_index, stack -> line, variant

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0

extern void *object_try_and_get(datum_index object_index, int32_t kind); // 0x4f6ec0
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX
extern void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out); // 0x568f50, ECX, ESI
extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant,
                                        actor_vocalization_context *context); // 0x4142d0

// blam-cc: EAX -> prop_index, EBX -> actor_index, EDI -> vehicle_object_index, stack -> line, variant
// If actor_index, vehicle_object_index and variant are all valid and vehicle_object_index
// resolves to a vehicle, vocalizes about prop_index (kind 1) when it names a nearby (kind
// 2 or 3) prop, or falls back to a fixed marker position (kind 3, via unit_get_primary_eye_marker_position)
// otherwise.
void actor_issue_order_or_vocalize(datum_index prop_index, datum_index actor_index,
                                    datum_index vehicle_object_index, int16_t line, int16_t variant)
{
    void *vehicle_obj;
    actor_vocalization_context context;
    prop *p;
    int16_t kind;

    if (actor_index == (datum_index)k_datum_index_none) {
        return;
    }
    if (variant < 1) {
        return;
    }
    if (vehicle_object_index == (datum_index)k_datum_index_none) {
        return;
    }
    vehicle_obj = object_try_and_get(vehicle_object_index, 3);
    if (vehicle_obj == 0) {
        return;
    }

    if (prop_index == (datum_index)k_datum_index_none) {
        prop_index = actor_find_prop_for_object(vehicle_object_index, actor_index); // 0x43031b: ECX = actor (ebx)
    }
    kind = -1;
    if (prop_index != (datum_index)k_datum_index_none) {
        p = &((prop *)prop_data->data)[prop_index & 0xffff];
        kind = p->kind;
    }

    if (prop_index == (datum_index)k_datum_index_none || kind < 2 || 3 < kind) {
        context.kind = 3;
        unit_get_primary_eye_marker_position(vehicle_object_index, (real_point3d *)&context.handle); // 0x430367: ESI = ctx + 4
    } else {
        context.kind = 1;
        context.handle = prop_index;
    }
    actor_begin_vocalization(actor_index, line, variant, &context);
}

#if 0
Original Ghidra decompilation (0x4302e0):

void FUN_004302e0(undefined4 param_1,undefined4 param_2)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  int unaff_EBX;
  int unaff_EDI;
  undefined2 local_10 [2];
  uint local_c;

  if (unaff_EBX == -1) {
    return;
  }
  if ((short)param_2 < 1) {
    return;
  }
  if (unaff_EDI == -1) {
    return;
  }
  iVar2 = object_try_and_get(3);
  if (iVar2 != 0) {
    if ((((in_EAX == 0xffffffff) && (in_EAX = FUN_0043ea80(), in_EAX == 0xffffffff)) ||
        (sVar1 = *(short *)((in_EAX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34) + 0x24),
        sVar1 < 2)) || ((3 < sVar1 || (in_EAX == 0xffffffff)))) {
      local_10[0] = 3;
      FUN_00568f50();
    }
    else {
      local_10[0] = 1;
      local_c = in_EAX;
    }
    FUN_004142d0(param_1,param_2,local_10);
    return;
  }
  return;
}
#endif
