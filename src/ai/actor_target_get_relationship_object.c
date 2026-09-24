// actor_target_get_relationship_object  (Ghidra: actor_target_get_relationship_object, already named)
// address 0x41f3a0, size 112 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase2/results/ai_02.json -- lazily resolves target-data field +0xec: if a
//   linked relationship (+0x110) exists uses (the function now named)
//   unit_predict_aim_target_position, otherwise looks up the object via object_try_and_get and
//   (the function now named) biped_get_cached_look_at_position, caching the result. +0x110 is
//   prop.relationship_object_index in types/ai.h.
// register convention: EAX -> target_prop_index; no other register operands are read.
//
// UNSURE: the cache slot at prop+0xec and the out-parameter at prop+0xf0 are written here as
// a plain int32_t / real_point3d* respectively, straddling the boundary types/ai.h drew for
// unknown_ec (0xec..0xf7) and unknown_f8; the header's real_point3d framing for that run comes
// from a different, stronger reader (actor_target_data_refresh @0x41c4b0) and this function's
// 4-byte cache write and mid-field out-pointer do not fit it cleanly. Left as raw offsets from
// the prop base rather than forcing a field that would change the layout.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0

// Real signature (src/units/unit_predict_aim_target_position.c): takes a unit index in ESI and
// an out_position pointer in EBX; Ghidra recovers neither operand at this call site.
extern int32_t unit_predict_aim_target_position(void); // 0x571de0, UNSURE signature
extern void *object_try_and_get(int32_t kind);          // 0x4f6ec0
extern datum_index biped_get_cached_look_at_position(uint32_t object_index, real_point3d *out_position); // 0x55ab30

// blam-cc: EAX -> target_prop_index
// Lazily resolves and caches a prop's (a target-data record's) associated relationship /
// obstruction object handle. If the record already has an explicit relationship link, that is
// resolved through unit_predict_aim_target_position; otherwise, when the local player object is
// available, the tracked object's cached look-at reference is resolved through
// biped_get_cached_look_at_position instead. Either way the result is cached so later calls are
// a no-op.
void actor_target_get_relationship_object(datum_index target_prop_index)
{
    prop *target;
    int32_t *cache;
    datum_index resolved;

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    cache = (int32_t *)((uint8_t *)target + 0xec);

    if (*cache == -1) {
        if (target->relationship_object_index != -1) {
            *cache = unit_predict_aim_target_position();
            return;
        }
        resolved = target->object_index;
        if (object_try_and_get(1) != (void *)0) {
            resolved = biped_get_cached_look_at_position(resolved, (real_point3d *)((uint8_t *)target + 0xf0));
            *cache = (int32_t)resolved;
        }
    }
}

#if 0
Original Ghidra decompilation (0x41f3a0):

void actor_target_get_relationship_object(void)

{
  uint in_EAX;
  int iVar1;
  undefined4 uVar2;
  int iVar3;

  iVar1 = (in_EAX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  if (*(int *)(iVar1 + 0xec) == -1) {
    if (*(int *)(iVar1 + 0x110) != -1) {
      uVar2 = FUN_00571de0();
      *(undefined4 *)(iVar1 + 0xec) = uVar2;
      return;
    }
    uVar2 = *(undefined4 *)(iVar1 + 0x18);
    iVar3 = object_try_and_get(1);
    if (iVar3 != 0) {
      uVar2 = FUN_0055ab30(uVar2,iVar1 + 0xf0);
      *(undefined4 *)(iVar1 + 0xec) = uVar2;
    }
  }
  return;
}
#endif
