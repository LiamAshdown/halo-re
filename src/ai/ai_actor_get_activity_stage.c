// ai_actor_get_activity_stage  (Ghidra: ai_actor_get_activity_stage; named for this rewrite)
// address 0x435680, size 121 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: classifies one actor's current combat activity into an ordinal 0..6 stage using
// actor.active/awareness_level/combat_status/target_combat_status (all already named in
// types/ai.h) plus two byte flags at +0x454/+0x45c this rewrite has no established name
// for. Matches the phase-4 summary exactly.
// register convention: Ghidra could not resolve the parameter at all.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

int32_t ai_actor_get_activity_stage(datum_index actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (a->active == 0) {
        return 0;
    }
    if (a->awareness_level < 3) {
        return 1;
    }
    if (a->combat_status == 0) {
        return 2;
    }
    if (a->target_combat_status < 6) {
        return 3;
    }
    if (a->target_combat_status < 10) {
        return 4;
    }
    if (*((uint8_t *)a + 0x454) != 0 || *((uint8_t *)a + 0x45c) != 0) { // UNSURE: no established fields
        return 6;
    }
    return 5;
}

#if 0
Original Ghidra decompilation (0x435680):

undefined4 FUN_00435680(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  undefined4 uVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724;
  iVar2 = iVar1 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar1 + 8 + *(int *)(DAT_00880360 + 0x34)) == '\0') {
    return 0;
  }
  if (*(short *)(iVar2 + 0x6a) < 3) {
    return 1;
  }
  if (*(short *)(iVar2 + 0x6e) == 0) {
    return 2;
  }
  if (*(short *)(iVar2 + 0x268) < 6) {
    return 3;
  }
  if (*(short *)(iVar2 + 0x268) < 10) {
    return 4;
  }
  if ((*(char *)(iVar2 + 0x454) != '\0') || (uVar3 = 5, *(char *)(iVar2 + 0x45c) != '\0')) {
    uVar3 = 6;
  }
  return uVar3;
}
#endif
