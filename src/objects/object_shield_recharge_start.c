// object_shield_recharge_start
// address 0x4edba0, size 101 bytes
// name confidence: 0.75 (out/phase4/objects_types_notes.md names this function directly:
// "0x122 unknown_122 ... set by object_shield_recharge_start")
// rewrite confidence: 0.6
// evidence: types/objects.h object.shield_vitality (0xe4), object.vitality_flags (0x106,
// _object_shield_recharging_bit), object.shield_stun_ticks (0x104), object 0x122.
// register convention: uint32_t object_index in EAX (in_EAX). Returns a bool (1 when recharge
// bookkeeping actually ran) in EAX's low byte; the rest of the Ghidra return value is x87
// comparison-flag packing noise.
// UNSURE: the Ghidra guard `(fVar1 < 1.0) != (fVar1 == 1.0)` is algebraically identical to the
// plain C `shield_vitality <= 1.0f` for every real float and for NaN (both give false), so it is
// written that way below with no change in behaviour.
// blam-cc: EAX=object_index

// RETURN TYPE (phase-4 review pass): Ghidra returns this as CONCAT31(garbage, AL) / bool,
//   i.e. only the low byte is defined -- the top three bytes are whatever happened to be in
//   the register. The return type is uint8_t so no caller can depend on the garbage, which
//   is the same correction already recorded for object_type_definitions_query_0x44 0x4f41d0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

uint8_t object_shield_recharge_start(uint32_t object_index)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;

    if (obj->shield_vitality <= 1.0f) {
        obj->vitality_flags |= _object_shield_recharging_bit;
        if (obj->shield_vitality == 0.0f) {
            obj->shield_vitality = 0.01f;
        }
        obj->shield_stun_ticks = 0;
        *((uint8_t *)obj + 0x122) = 1; // UNSURE: object 0x122 has no name in objects.h beyond
                                       // "set by object_shield_recharge_start"
        return 1;
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x4edba0):

undefined4 FUN_004edba0(void)

{
  float fVar1;
  int iVar2;
  uint in_EAX;
  undefined2 uVar3;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  fVar1 = *(float *)(iVar2 + 0xe4);
  uVar3 = (undefined2)((in_EAX & 0xffff) * 3 >> 0x10);
  if (fVar1 < 1.0 != (fVar1 == 1.0)) {
    *(byte *)(iVar2 + 0x106) = *(byte *)(iVar2 + 0x106) | 0x10;
    fVar1 = *(float *)(iVar2 + 0xe4);
    if (fVar1 == 0.0) {
      *(undefined4 *)(iVar2 + 0xe4) = 0x3c23d70a;
    }
    *(undefined2 *)(iVar2 + 0x104) = 0;
    *(undefined1 *)(iVar2 + 0x122) = 1;
    return CONCAT31((int3)(CONCAT22(uVar3,(ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 |
                                          (ushort)(fVar1 == 0.0) << 0xe) >> 8),1);
  }
  return CONCAT22(uVar3,(ushort)(fVar1 < 1.0) << 8 | (ushort)NAN(fVar1) << 10 |
                        (ushort)(fVar1 == 1.0) << 0xe);
}
#endif
