// actor_target_is_visible_or_object_count_ok  (Ghidra: actor_target_is_visible_or_object_count_ok, renamed)
// address 0x40f700, size 145 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (VERIFIED against objdump 0x40f700..0x40f790)
// evidence: phase-4 summary "for grenade-target kind 3, revalidates that the previously
// chosen target is still a good throw candidate"; the only kind handled specially (3)
// checks the recognized prop's relationship_object_index, then, failing that, counts
// hostiles within 6 world units of the prop's last_known_position via actor_score_blast_area_clear.
// register convention: actor_index in EAX, kind on the stack (param_1).
// blam-cc: EAX -> actor_index, stack -> kind

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0


// blam-cc: EAX -> actor_index, stack -> kind
uint8_t actor_target_is_visible_or_object_count_ok(datum_index actor_index, int16_t kind)
{
    actor *self;
    prop *p;
    int16_t hostile_count;

    if (kind != 3) {
        return 1;
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    p = (prop *)((uint8_t *)prop_data->data + (self->unknown_610 & 0xffff) * sizeof(prop));

    if (p->relationship_object_index != -1) {
        return 1;
    }
    if (p->is_parented != 0) {
        return 0;
    }

    hostile_count = 0;
    actor_score_blast_area_clear(actor_index, 6.0f, 0.0f, &p->last_known_position, &hostile_count);
    return 2 < hostile_count;
}

#if 0
Original Ghidra decompilation (0x40f700):

bool FUN_0040f700(short param_1)

{
  uint in_EAX;
  int iVar1;
  int iVar2;

  if (param_1 != 3) {
    return true;
  }
  iVar1 = (*(uint *)((in_EAX & 0xffff) * 0x724 + 0x610 + *(int *)(DAT_00880360 + 0x34)) & 0xffff) *
          0x138;
  iVar2 = iVar1 + *(int *)(DAT_008802c0 + 0x34);
  if (*(int *)(iVar1 + 0x110 + *(int *)(DAT_008802c0 + 0x34)) != -1) {
    return true;
  }
  if (*(char *)(iVar2 + 0x12e) != '\0') {
    return false;
  }
  _param_1 = 0;
  actor_score_blast_area_clear(0x40c00000,0,iVar2 + 0xbc,&param_1);
  return 2 < param_1;
}
#endif
