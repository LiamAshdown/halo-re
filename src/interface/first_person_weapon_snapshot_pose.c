// first_person_weapon_snapshot_pose  (Ghidra: FUN_004930b0, renamed per types/interface.h)
// address 0x4930b0, size 160 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4930b0..0x49314f)
// evidence: types/interface.h first_person_weapon_interface comment: "+0x88 / +0x8a and the two
// 0x800 byte blocks -- first_person_weapon_snapshot_pose @0x4930b0 copies from +0x8c to +0x88c
// and updates the two shorts at +0x88/+0x8a. 0x800 is the gap between the two block bases, not a
// stated size; the real copy length is derived at run time from the weapon node/track count" --
// exactly what this rewrite finds: the copy count comes from the animation graph's node count (weapon tag +0x478 dependency), not a fixed 0x800.
// register convention: local_player_index in AX (in_AX), blend_gap in DX (in_DX), both
// unrecognized by Ghidra. // blam-cc: local_player_index=AX, blend_gap=DX
// The +0x68 dword of the first-person animation graph tag (Weapon +0x478 dependency) is ModelAnimations.nodes.count; each node
// pose is 8 dwords (32 bytes), so the copy is `(count << 5) >> 2` dwords (0x49310a..0x49311c). The byte-remainder `rep movsb`
// count is `(count << 5) & 3`, always zero, so it is omitted.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "cache.h"

extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances; // 0x0087bc14, types/cache.h
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98

// Copies the live animation_control block into previous_pose (for blending into the next
// state), with the copy length derived from the first-person animation graph's node count, then extends
// blend_end to blend_gap (resetting blend_start to 0) if the current blend window is shorter
// than blend_gap.
void first_person_weapon_snapshot_pose(int16_t local_player_index, int16_t blend_gap)
{
    first_person_weapon_interface *fp;
    uint8_t *weapon_obj;
    uint8_t *item_tag_data;
    ModelAnimations *graph;
    int32_t dword_count;
    uint32_t *src;
    uint32_t *dst;

    fp = &first_person_weapon_interfaces[local_player_index];

    weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
    item_tag_data = *(uint8_t **)((uint8_t *)tag_instances +
                                   (*(uint32_t *)weapon_obj & 0xffff) * 0x20 + 0x14);
    graph = (ModelAnimations *)*(uint8_t **)((uint8_t *)tag_instances +
                                  (*(uint32_t *)(item_tag_data + 0x478) & 0xffff) * 0x20 + 0x14);
    dword_count = (int32_t)(((uint32_t)graph->nodes.count << 5) >> 2);

    src = (uint32_t *)fp->animation_control;
    dst = (uint32_t *)fp->previous_pose;
    while (dword_count != 0) {
        *dst = *src;
        src++;
        dst++;
        dword_count--;
    }

    if ((int32_t)fp->blend_end - (int32_t)fp->blend_start <= blend_gap) {
        fp->blend_start = 0;
        fp->blend_end = blend_gap;
    }
}

#if 0
Original Ghidra decompilation (0x4930b0):

void FUN_004930b0(void)

{
  short in_AX;
  int iVar1;
  int iVar2;
  short in_DX;
  undefined4 *puVar3;
  undefined4 *puVar4;

  iVar1 = in_AX * 0x1ea0 + DAT_006b2d98;
  puVar3 = (undefined4 *)(iVar1 + 0x8c);
  puVar4 = (undefined4 *)(iVar1 + 0x88c);
  for (iVar2 = (*(uint *)(*(int *)((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) +
                                                                    8 + (*(uint *)(iVar1 + 8) &
                                                                        0xffff) * 0xc) & 0xffff) *
                                                       0x20 + 0x14 + DAT_0087bc14) + 0x478) & 0xffff
                                   ) * 0x20 + 0x14 + DAT_0087bc14) + 0x68) & 0x7ffffff) << 3;
      iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar4 = puVar4 + 1;
  }
  for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
    *(undefined1 *)puVar4 = *(undefined1 *)puVar3;
    puVar3 = (undefined4 *)((int)puVar3 + 1);
    puVar4 = (undefined4 *)((int)puVar4 + 1);
  }
  if ((int)*(short *)(iVar1 + 0x8a) - (int)*(short *)(iVar1 + 0x88) <= (int)in_DX) {
    *(undefined2 *)(iVar1 + 0x88) = 0;
    *(short *)(iVar1 + 0x8a) = in_DX;
  }
  return;
}
#endif
