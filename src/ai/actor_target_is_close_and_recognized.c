// actor_target_is_close_and_recognized  (Ghidra: actor_target_is_close_and_recognized; named for this rewrite)
// address 0x42f480, size 100 bytes
// name confidence: 0.3   rewrite confidence: 0.95
// evidence: phase-4 summary ("checks whether a given unit is currently a close,
// appropriately-categorized recognized object").
// register convention: plain __cdecl (param_1/param_2 recognized by Ghidra but never read).
// blam-cc: stack -> object_index, param_2 (unused), actor_index
// 0x42f491: EAX = the first argument (the object), stack = (actor, 1, 1). The draft passed the actor as the
//   object and 1 as the actor.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *prop_data; // 0x008802c0

extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag); // 0x43eb30, EAX, stack

// blam-cc: stack -> param_1 (unused), param_2 (unused), actor_index
// Resolves actor_index to its prop record and returns 1 if it is within 5 world units and
// its unknown_38 field is 0 or 1, else 0.
uint8_t actor_target_is_close_and_recognized(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    datum_index prop_index;
    prop *p;

    if (actor_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    prop_index = actor_find_or_create_shared_prop(object_index, actor_index, 1, 1);
    if (prop_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    p = &((prop *)prop_data->data)[prop_index & 0xffff];
    if (p->distance < 5.0f && (p->obstruction == 0 || p->obstruction == 1)) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x42f480):

uint FUN_0042f480(undefined4 param_1,undefined4 param_2,uint param_3)

{
  float fVar1;
  short sVar2;
  int iVar3;
  undefined2 uVar4;

  if (param_3 != 0xffffffff) {
    param_3 = FUN_0043eb30(param_3,1,1);
    if (param_3 != 0xffffffff) {
      iVar3 = (param_3 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
      fVar1 = *(float *)(iVar3 + 0x11c);
      uVar4 = (undefined2)((uint)iVar3 >> 0x10);
      param_3 = CONCAT22(uVar4,(ushort)(fVar1 < 5.0) << 8 | (ushort)NAN(fVar1) << 10 |
                               (ushort)(fVar1 == 5.0) << 0xe);
      if (fVar1 < 5.0) {
        sVar2 = *(short *)(iVar3 + 0x38);
        param_3 = CONCAT22(uVar4,sVar2);
        if ((sVar2 == 0) || (sVar2 == 1)) {
          return CONCAT31((int3)(param_3 >> 8),1);
        }
      }
    }
  }
  return param_3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
