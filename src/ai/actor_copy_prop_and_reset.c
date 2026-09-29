// actor_copy_prop_and_reset  (Ghidra: actor_copy_prop_and_reset, renamed)
// address 0x43e840, size 205 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x43e840..0x43e90c (EAX dest, ECX src).)
// evidence: types/ai.h prop (identifier/actor_index/next_in_actor/pair_index preserved
// across the copy; kind(+0x24)=4 after; unknown_3a/unknown_3c reset; noticed_a/b/c(+0xb9..bb)
// cleared; unknown_40/44/48 set to last_known_position(+0xbc) minus alerted_ticks(+0x80);
// unknown_d4(+0xd4, real_point3d) set from the module's {1,0,0}-ish constant; unknown_123
// cleared). phase-4 summary "copies a firing-position node record into another slot and
// resets its per-instance runtime state."
// register convention: EAX -> dest_prop, ECX -> src_prop.
//   // blam-cc: EAX -> dest_prop, ECX -> src_prop

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0
extern real_point3d *global_origin3d_pointer; // 0x00696714, UNSURE: same constant referenced elsewhere in this module

// blam-cc: EAX -> dest_prop, ECX -> src_prop
void actor_copy_prop_and_reset(datum_index dest_prop, datum_index src_prop)
{
    prop *dest = (prop *)((uint8_t *)prop_data->data + (dest_prop & 0xffff) * sizeof(prop));
    prop *src = (prop *)((uint8_t *)prop_data->data + (src_prop & 0xffff) * sizeof(prop));

    int16_t identifier = dest->identifier;
    datum_index actor_index = dest->actor_index;
    datum_index next_in_actor = dest->next_in_actor;
    datum_index pair_index = dest->pair_index;

    *dest = *src;

    dest->identifier = identifier;
    dest->pair_index = pair_index;
    dest->actor_index = actor_index;
    dest->next_in_actor = next_in_actor;

    dest->kind = 4;
    dest->unknown_3a = 900;
    dest->unknown_3c = 0;
    dest->noticed_a = 0;
    dest->noticed_b = 0;
    dest->noticed_c = 0;
    {
        float dx = dest->last_known_position.x - dest->unknown_80.x;
        float dy = dest->last_known_position.y - dest->unknown_80.y;
        float dz = dest->last_known_position.z - dest->unknown_80.z;
        dest->unknown_40 = *(uint32_t *)&dx;
        dest->unknown_44 = *(uint32_t *)&dy;
        dest->unknown_48 = *(uint32_t *)&dz;
    }
    dest->unknown_d4 = *global_origin3d_pointer;
    dest->unknown_123 = 0;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043e840 @ 0x43e840) ----
void FUN_0043e840(void)

{
  undefined2 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined *puVar5;
  uint in_EAX;
  undefined4 *puVar6;
  uint in_ECX;
  int iVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;

  puVar6 = (undefined4 *)((in_EAX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34));
  uVar2 = puVar6[1];
  uVar1 = *(undefined2 *)puVar6;
  uVar3 = puVar6[2];
  uVar4 = puVar6[3];
  puVar8 = (undefined4 *)((in_ECX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34));
  puVar9 = puVar6;
  for (iVar7 = 0x4e; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar9 = *puVar8;
    puVar8 = puVar8 + 1;
    puVar9 = puVar9 + 1;
  }
  *(undefined2 *)puVar6 = uVar1;
  puVar6[3] = uVar4;
  puVar6[1] = uVar2;
  puVar6[2] = uVar3;
  puVar5 = PTR_DAT_00696714;
  *(undefined2 *)(puVar6 + 9) = 4;
  *(undefined2 *)((int)puVar6 + 0x3a) = 900;
  *(undefined2 *)(puVar6 + 0xf) = 0;
  *(undefined1 *)((int)puVar6 + 0xb9) = 0;
  *(undefined1 *)((int)puVar6 + 0xba) = 0;
  *(undefined1 *)((int)puVar6 + 0xbb) = 0;
  puVar6[0x10] = (float)puVar6[0x2f] - (float)puVar6[0x20];
  puVar6[0x11] = (float)puVar6[0x30] - (float)puVar6[0x21];
  puVar6[0x12] = (float)puVar6[0x31] - (float)puVar6[0x22];
  puVar6[0x35] = *(undefined4 *)puVar5;
  puVar6[0x36] = *(undefined4 *)(puVar5 + 4);
  puVar6[0x37] = *(undefined4 *)(puVar5 + 8);
  *(undefined1 *)((int)puVar6 + 0x123) = 0;
  return;
}
#endif
