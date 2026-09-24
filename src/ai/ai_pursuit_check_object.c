// ai_pursuit_check_object  (Ghidra: ai_pursuit_check_object; named for this rewrite)
// address 0x436b90, size 122 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: calls squad_recent_object_get_or_create (0x436c60, this batch) with all of its
// own parameters forwarded (Ghidra shows a zero-argument call), then reports whether a
// caller-supplied object is present in that ai_pursuit's ring buffer (or, once the record
// has seen 7 or more sightings, unconditionally true), plus the record's count and
// last_tick through the two out-parameters. Matches the phase-4 summary.
// register convention: Ghidra resolved param_2/param_3 as ordinary parameters and left the
// object to check in EBX.
//   // blam-cc: EBX -> object_index, stack -> encounter_index, type, min_last_tick,
//   create_if_missing, out_count, out_last_tick

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *ai_pursuit_data; // 0x008802d0

extern datum_index squad_recent_object_get_or_create(datum_index encounter_index, int16_t type,
    int32_t min_last_tick, char create_if_missing); // 0x436c60, this batch

// blam-cc: EBX -> object_index, stack -> encounter_index, type, min_last_tick,
//   create_if_missing, out_count, out_last_tick
uint8_t ai_pursuit_check_object(datum_index object_index, datum_index encounter_index, int16_t type,
                                 int32_t min_last_tick, char create_if_missing, int16_t *out_count,
                                 uint32_t *out_last_tick)
{
    uint8_t found = 0;
    int16_t count = 0;
    uint32_t last_tick = (uint32_t)k_datum_index_none;
    datum_index handle = squad_recent_object_get_or_create(encounter_index, type, min_last_tick, create_if_missing);

    if (handle != (datum_index)k_datum_index_none) {
        ai_pursuit *pursuit = &((ai_pursuit *)ai_pursuit_data->data)[handle & 0xffff];
        count = pursuit->count;
        last_tick = *(uint32_t *)&pursuit->last_tick;

        if (count < 7) {
            int16_t i;
            for (i = 0; i < k_ai_pursuit_object_count; i++) {
                if (pursuit->object_index[i] == object_index) {
                    found = 1;
                    break;
                }
            }
        } else {
            found = 1;
        }
    }

    if (out_count != 0) {
        *out_count = count;
    }
    if (out_last_tick != 0) {
        *out_last_tick = last_tick;
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x436b90):

undefined4 FUN_00436b90(undefined4 param_1,short *param_2,undefined4 *param_3)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  short sVar4;
  int unaff_EBX;
  short sVar5;
  undefined4 uVar6;

  uVar2 = squad_recent_object_get_or_create();
  uVar6 = 0xffffffff;
  uVar3 = 0;
  sVar5 = 0;
  if (uVar2 != 0xffffffff) {
    uVar2 = uVar2 & 0xffff;
    iVar1 = *(int *)(DAT_008802d0 + 0x34);
    sVar5 = *(short *)(iVar1 + 8 + uVar2 * 0x28);
    uVar6 = *(undefined4 *)(iVar1 + 4 + uVar2 * 0x28);
    if (sVar5 < 7) {
      sVar4 = 0;
      do {
        if (*(int *)(iVar1 + uVar2 * 0x28 + 0xc + sVar4 * 4) == unaff_EBX) {
          uVar3 = 1;
          break;
        }
        sVar4 = sVar4 + 1;
      } while (sVar4 < 6);
    }
    else {
      uVar3 = 1;
    }
  }
  if (param_2 != (short *)0x0) {
    *param_2 = sVar5;
  }
  if (param_3 != (undefined4 *)0x0) {
    *param_3 = uVar6;
  }
  return uVar3;
}
#endif
