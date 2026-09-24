// object_restore_full_body_vitality
// address 0x4ed9d0, size 80 bytes
// name confidence: 0.35 (still FUN_004ed9d0 in Ghidra; named from behaviour per
// out/phase4/objects_functions.md: "If an object's health-frozen status bit is clear and its
// health fraction is below full, restores full health and marks the object as changed.")
// rewrite confidence: 0.6
// evidence: types/objects.h object.vitality_flags (0x106, _object_health_frozen_bit 0x0004),
// object.body_vitality (0xe0), object.flags (0x10, _object_changed_bit 0x04000000).
// register convention: uint32_t object_index in EAX (in_EAX). Returns a bool (1 when the
// health was actually restored) in EAX's low byte; the upper bytes in the Ghidra output are
// x87 comparison-flag packing noise and carry no meaning.
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

uint8_t object_restore_full_body_vitality(uint32_t object_index)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;

    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        return 0;
    }

    if (obj->body_vitality < 1.0f) {
        obj->body_vitality = 1.0f;
        obj->flags |= _object_changed_bit;
        return 1;
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x4ed9d0):

uint FUN_004ed9d0(void)

{
  float fVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  uint uVar4;

  iVar3 = (in_EAX & 0xffff) * 3;
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar4 = CONCAT31((int3)((uint)iVar3 >> 8),*(byte *)(iVar2 + 0x106));
  if ((*(byte *)(iVar2 + 0x106) & 4) == 0) {
    fVar1 = *(float *)(iVar2 + 0xe0);
    uVar4 = CONCAT22((short)((uint)iVar3 >> 0x10),
                     (ushort)(fVar1 < 1.0) << 8 | (ushort)NAN(fVar1) << 10 |
                     (ushort)(fVar1 == 1.0) << 0xe);
    if (fVar1 < 1.0) {
      *(undefined4 *)(iVar2 + 0xe0) = 0x3f800000;
      *(uint *)(iVar2 + 0x10) = *(uint *)(iVar2 + 0x10) | 0x4000000;
      return CONCAT31((int3)(uVar4 >> 8),1);
    }
  }
  return uVar4 & 0xffffff00;
}
#endif
