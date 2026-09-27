// squad_recent_object_get_or_create  (Ghidra: squad_recent_object_get_or_create, already named)
// address 0x436c60, size 223 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED against objdump 0x436c60..0x436d3e (walk the encounter pursuit list +0x38 by type; stale = last tick < min; create via datum_new (EDX pursuit data) linked at the head; a stale or new record is reset and returned only when creating))
// evidence: types/ai.h ai_pursuit (type 0x02, last_tick 0x04, count 0x08, cursor 0x0a,
// object_index[6] 0x0c, next 0x24) and encounter.first_pursuit (0x38), both already
// established. Finds the encounter's ai_pursuit record of a given type, optionally creating
// one, and resets it (clearing its object slots) if it was stale (last_tick before the
// caller's threshold) or freshly created.
// register convention: Ghidra fully resolved all three parameters and left the encounter
// index in EAX.
//   // blam-cc: EAX -> encounter_index, stack -> type, min_last_tick, create_if_missing
//
// UNSURE: the exact return value on the "found but not stale, or not found and not
// creating" paths is preserved via goto exactly as Ghidra decompiled it, rather than
// restructured, since the original's shared labels make the true intent ambiguous.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *encounter_data;  // 0x008802c8
extern data_array *ai_pursuit_data; // 0x008802d0

extern datum_index datum_new(data_array *array); // 0x4d0480

// blam-cc: EAX -> encounter_index, stack -> type, min_last_tick, create_if_missing
datum_index squad_recent_object_get_or_create(datum_index encounter_index, int16_t type, int32_t min_last_tick,
                                               char create_if_missing)
{
    encounter *enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    datum_index cursor = enc->first_pursuit;
    uint8_t stale = 0;
    ai_pursuit *pursuit = 0;

    while (cursor != (datum_index)k_datum_index_none) {
        pursuit = &((ai_pursuit *)ai_pursuit_data->data)[cursor & 0xffff];
        if (pursuit->type == type) {
            stale = pursuit->last_tick < min_last_tick;
            goto found;
        }
        cursor = pursuit->next;
    }

    if (create_if_missing != 0) {
        datum_index new_handle = datum_new(ai_pursuit_data);
        if (new_handle != (datum_index)k_datum_index_none) {
            ai_pursuit *new_pursuit = &((ai_pursuit *)ai_pursuit_data->data)[new_handle & 0xffff];
            new_pursuit->type = type;
            new_pursuit->next = enc->first_pursuit;
            enc->first_pursuit = new_handle;
            cursor = new_handle;
            pursuit = new_pursuit;
            goto reset;
        }
    }

found:
    if (!stale) {
        return cursor;
    }

reset:
    pursuit->count = 0;
    pursuit->cursor = 0;
    pursuit->last_tick = -1;
    pursuit->object_index[0] = (datum_index)k_datum_index_none;
    pursuit->object_index[1] = (datum_index)k_datum_index_none;
    pursuit->object_index[2] = (datum_index)k_datum_index_none;
    pursuit->object_index[3] = (datum_index)k_datum_index_none;
    pursuit->object_index[4] = (datum_index)k_datum_index_none;
    pursuit->object_index[5] = (datum_index)k_datum_index_none;
    if (create_if_missing == 0) {
        return (datum_index)k_datum_index_none;
    }
    return cursor;
}

#if 0
Original Ghidra decompilation (0x436c60):

uint squad_recent_object_get_or_create(short param_1,int param_2,char param_3)

{
  int iVar1;
  bool bVar2;
  uint in_EAX;
  int iVar3;
  uint uVar4;
  int iVar5;
  undefined8 uVar6;

  iVar3 = (in_EAX & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  uVar4 = *(uint *)(iVar3 + 0x38);
  bVar2 = false;
  iVar5 = DAT_008802d0;
  if (uVar4 != 0xffffffff) {
    do {
      iVar1 = *(int *)(DAT_008802d0 + 0x34) + (uVar4 & 0xffff) * 0x28;
      if (*(short *)(*(int *)(DAT_008802d0 + 0x34) + 2 + (uVar4 & 0xffff) * 0x28) == param_1) {
        bVar2 = *(int *)(iVar1 + 4) < param_2;
        if (uVar4 != 0xffffffff) goto LAB_00436cf2;
        break;
      }
      uVar4 = *(uint *)(iVar1 + 0x24);
    } while (uVar4 != 0xffffffff);
  }
  if (param_3 != '\0') {
    uVar6 = datum_new();
    iVar5 = (int)((ulonglong)uVar6 >> 0x20);
    uVar4 = (uint)uVar6;
    if (uVar4 != 0xffffffff) {
      iVar5 = *(int *)(iVar5 + 0x34) + (uVar4 & 0xffff) * 0x28;
      *(short *)(iVar5 + 2) = param_1;
      *(undefined4 *)(iVar5 + 0x24) = *(undefined4 *)(iVar3 + 0x38);
      *(uint *)(iVar3 + 0x38) = uVar4;
      goto LAB_00436cf6;
    }
  }
LAB_00436cf2:
  uVar6 = CONCAT44(iVar5,uVar4);
  if (!bVar2) {
    return uVar4;
  }
LAB_00436cf6:
  uVar4 = (uint)uVar6;
  iVar5 = *(int *)((int)((ulonglong)uVar6 >> 0x20) + 0x34) + (uVar4 & 0xffff) * 0x28;
  *(undefined2 *)(iVar5 + 8) = 0;
  *(undefined2 *)(iVar5 + 10) = 0;
  *(undefined4 *)(iVar5 + 4) = 0xffffffff;
  *(undefined4 *)(iVar5 + 0xc) = 0xffffffff;
  *(undefined4 *)(iVar5 + 0x10) = 0xffffffff;
  *(undefined4 *)(iVar5 + 0x14) = 0xffffffff;
  *(undefined4 *)(iVar5 + 0x18) = 0xffffffff;
  *(undefined4 *)(iVar5 + 0x1c) = 0xffffffff;
  *(undefined4 *)(iVar5 + 0x20) = 0xffffffff;
  if (param_3 == '\0') {
    uVar4 = 0xffffffff;
  }
  return uVar4;
}
#endif
