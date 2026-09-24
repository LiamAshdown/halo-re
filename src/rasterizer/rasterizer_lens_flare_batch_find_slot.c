// rasterizer_lens_flare_batch_find_slot  (Ghidra: FUN_00536cb0)
// address 0x536cb0, size 195 bytes
// name confidence: 0.4   rewrite confidence: 0.65
// evidence: functions.md summary ("Finds (or LRU-evicts and reassigns) a batching slot matching
//   the current material key for the screen-space sprite rendering system, returning its slot
//   index"); the compared/copied fields match lens_flare_batch_key and lens_flare_current_key
//   (0x00746fb0) exactly, and the incrementing stamp matches lens_flare_batch_clock (0x00746fa8).
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern lens_flare_batch lens_flare_batches[k_lens_flare_batch_slots]; // 0x00746fc0
extern lens_flare_batch_key lens_flare_current_key; // 0x00746fb0
extern uint32_t lens_flare_batch_clock; // 0x00746fa8

extern void rasterizer_lens_flare_batch_draw_slot(int32_t batch_index); // 0x536c10

static int key_equal(const lens_flare_batch_key *a, const lens_flare_batch_key *b)
{
    return a->bitmap_tag_index == b->bitmap_tag_index &&
           a->second_bitmap_tag_index == b->second_bitmap_tag_index &&
           a->bitmap_index == b->bitmap_index &&
           *(const uint32_t *)&a->shader_stage_config == *(const uint32_t *)&b->shader_stage_config;
}

// Finds (or LRU-evicts and reassigns) a batching slot matching the current material key for the
// screen-space sprite rendering system, returning its slot index.
int32_t rasterizer_lens_flare_batch_find_slot(void)
{
    int32_t i;
    int32_t lru_index = 0;
    uint32_t lru_last_used = lens_flare_batches[0].last_used;

    for (i = 0; i < k_lens_flare_batch_slots; i++) {
        if (key_equal(&lens_flare_batches[i].key, &lens_flare_current_key)) {
            lens_flare_batches[i].last_used = lens_flare_batch_clock + 1;
            lens_flare_batch_clock++;
            return i;
        }
        if (lens_flare_batches[i].last_used < lru_last_used) {
            lru_last_used = lens_flare_batches[i].last_used;
            lru_index = i;
        }
    }

    rasterizer_lens_flare_batch_draw_slot(lru_index);
    lens_flare_batches[lru_index].key = lens_flare_current_key;
    lens_flare_batches[lru_index].last_used = lens_flare_batch_clock + 1;
    lens_flare_batch_clock++;
    return lru_index;
}

#if 0
Original Ghidra decompilation (0x536cb0):

int FUN_00536cb0(void)

{
  int iVar1;
  int iVar2;
  uint *puVar3;
  uint uVar4;
  uint *puVar5;
  uint *puVar6;
  bool bVar7;
  int local_4;

  iVar1 = 0;
  local_4 = 0;
  puVar3 = &DAT_0075efd4;
  uVar4 = DAT_0075efd4;
  do {
    iVar2 = 4;
    bVar7 = true;
    puVar5 = puVar3 + -4;
    puVar6 = &DAT_00746fb0;
    do {
      if (iVar2 == 0) break;
      iVar2 = iVar2 + -1;
      bVar7 = *puVar5 == *puVar6;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    } while (bVar7);
    if (bVar7) {
      if (iVar1 < 5) {
        (&DAT_0075efd4)[iVar1 * 0x6006] = DAT_00746fa8 + 1;
        DAT_00746fa8 = DAT_00746fa8 + 1;
        return iVar1;
      }
      goto LAB_00536cfe;
    }
    if (*puVar3 < uVar4) {
      uVar4 = *puVar3;
      local_4 = iVar1;
    }
    puVar3 = puVar3 + 0x6006;
    iVar1 = iVar1 + 1;
    if (0x7d704b < (int)puVar3) {
LAB_00536cfe:
      FUN_00536c10();
      iVar1 = local_4 * 0x18018;
      (&DAT_0075efc4)[local_4 * 0x6006] = DAT_00746fb0;
      *(undefined4 *)(iVar1 + 0x75efc8) = DAT_00746fb4;
      *(undefined4 *)(iVar1 + 0x75efcc) = DAT_00746fb8;
      *(undefined4 *)(iVar1 + 0x75efd0) = DAT_00746fbc;
      (&DAT_0075efd4)[local_4 * 0x6006] = DAT_00746fa8 + 1;
      DAT_00746fa8 = DAT_00746fa8 + 1;
      return local_4;
    }
  } while( true );
}
#endif
