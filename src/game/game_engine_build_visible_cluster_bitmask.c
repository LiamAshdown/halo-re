// game_engine_build_visible_cluster_bitmask  (Ghidra: FUN_004782a0; renamed -- builds a
// 512-bit cluster bitmask, OR-ing in the potentially-visible-cluster set of every (or every
// local) player's root object)
// address 0x4782a0, size 347 bytes
// name confidence: 0.25   rewrite confidence: 0.9 (checked against objdump 0x4782a0..0x4783fa; the final
//   bit_vector_or had lost three of its four arguments)
// evidence: out/functions.json callee list (bit_vector_or, data_iterator_next, objects_get_ambient_cluster);
//   types/objects.h object::parent_object (0x11c) and object::location_cluster_index (0x09c,
//   read here at the resolved root object as the cached BSP cluster); the 16-dword (512-bit)
//   output buffer and the `(count+31)>>5` stride arithmetic are the standard cluster-bitset
//   shape used elsewhere in this engine, but DAT_00746f9c's own struct (a structure-BSP runtime
//   record with a cluster count at +0x134 and a per-cluster bit-array table at +0x14c) is not
//   named anywhere in this batch's evidence and is not reproduced as a new type here.
// register convention: none -- both are genuine stack parameters (Ghidra's own
//   param_1/param_2).
// DAT_00746f9c's layout is kept as raw offsets (+0x134 cluster count, +0x14c per-cluster bit rows). objects_get_ambient_cluster
// (0x4f7a50) takes no registers and returns an int16 cluster (-1 = none), which is OR'd in last via bit_vector_or.
// The iterator Ghidra elided is the inline types/memory.h data_iterator over player_data
//   (0x4782c1..0x4782e8: data, WORD next_index = 0, index = -1, signature = data ^ 'iter').
// reconciled: R16 the elided iterator is the inline 0x10-byte data_iterator over player_data (0x4782c1)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include <stdint.h>

extern ScenarioStructureBSP *global_structure_bsp;
extern data_array *object_data;      // 0x008603b0

extern int16_t objects_get_ambient_cluster(void); // 0x4f7a50, no register inputs
extern data_array *player_data;         // 0x0087a480
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0; blam-cc: EDI -> iterator
extern void bit_vector_or(uint32_t *a, int16_t bit_count, uint32_t *b, uint32_t *dst); // 0x4cb760, EAX a, CX count, EDX b, stack dst

// Zeroes a 16-dword (512-bit) output bitmask, then for every player (or, when
// `local_players_only` is set, every LOCAL player), walks that player's unit up its parent
// chain to the root object, caches the root's location_cluster_index back onto the player
// record's own cluster field (+0x3c), and OR's that cluster's potentially-visible-cluster
// bit-array into the output. Finally, if objects_get_ambient_cluster returns a valid result, OR's in one more
// bitmask via bit_vector_or.
void game_engine_build_visible_cluster_bitmask(uint32_t *out_bitmask, uint8_t local_players_only)
{
    uint8_t *bsp_info = (uint8_t *)global_structure_bsp;
    int32_t i;
    data_iterator iterator;
    void *p;
    int16_t player_gate_result;

    for (i = 0; i < 0x10; i++) {
        out_bitmask[i] = 0;
    }

    player_gate_result = objects_get_ambient_cluster();

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = data_iterator_next(&iterator);
    while (p != 0) {
        player *pl = (player *)p;
        if (local_players_only == 0 || pl->local_player_index != -1) {
            if (pl->unit != (datum_index)0xffffffff) {
                uint32_t current = (uint32_t)pl->unit;
                object *root;
                do {
                    root = (object *)((object_header *)object_data->data)[current & 0xffff].data;
                    current = (uint32_t)root->parent_object;
                } while (current != 0xffffffff);
                // object+0x9c is location_cluster_index (0x478347..0x478358).
                if (root->location_cluster_index != -1) {
                    pl->bsp_cluster = root->location_cluster_index;
                }
            }

            if (pl->bsp_cluster != -1) {
                int32_t cluster_count = *(int32_t *)(bsp_info + 0x134);
                uint8_t *cluster_bits_table = *(uint8_t **)(bsp_info + 0x14c);
                int32_t stride_dwords = (cluster_count + 0x1f) >> 5;
                int16_t words = (int16_t)stride_dwords;
                int32_t j;

                for (j = words - 1; j >= 0; j--) {
                    out_bitmask[j] |= *(uint32_t *)(cluster_bits_table + stride_dwords * pl->bsp_cluster * 4 + j * 4);
                }
            }
        }
        p = data_iterator_next(&iterator);
    }

    if (player_gate_result != -1) {
        // 0x4783c6: OR the ambient cluster's visibility row in (EAX row, CX cluster count, EDX = dst = out)
        int32_t cluster_count = *(int32_t *)(bsp_info + 0x134);
        uint32_t *row = (uint32_t *)(*(uint8_t **)(bsp_info + 0x14c) +
            ((cluster_count + 0x1f) >> 5) * player_gate_result * 4);

        bit_vector_or(row, (int16_t)cluster_count, out_bitmask, out_bitmask);
    }
}

#if 0
Original Ghidra decompilation (0x4782a0), from tools/pack.py 0x4782a0:

void FUN_004782a0(undefined4 *param_1,char param_2)

{
  short sVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  short sVar5;
  uint uVar6;
  int iVar7;
  uint uVar8;
  uint *puVar9;
  undefined4 *puVar10;

  iVar3 = DAT_00746f9c;
  puVar10 = param_1;
  for (iVar7 = 0x10; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar10 = 0;
    puVar10 = puVar10 + 1;
  }
  sVar4 = FUN_004f7a50();
  iVar7 = data_iterator_next();
  while (iVar7 != 0) {
    if ((param_2 == '\0') || (*(short *)(iVar7 + 2) != -1)) {
      if (*(uint *)(iVar7 + 0x34) != 0xffffffff) {
        uVar6 = *(uint *)(iVar7 + 0x34);
        do {
          uVar8 = uVar6;
          uVar6 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar8 & 0xffff) * 0xc) +
                           0x11c);
        } while (uVar6 != 0xffffffff);
        sVar1 = *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar8 & 0xffff) * 0xc) +
                          0x9c);
        if (sVar1 != -1) {
          *(short *)(iVar7 + 0x3c) = sVar1;
        }
      }
      sVar1 = *(short *)(iVar7 + 0x3c);
      if (sVar1 != -1) {
        iVar7 = *(int *)(iVar3 + 0x134);
        iVar2 = *(int *)(iVar3 + 0x14c);
        uVar6 = *(short *)(iVar3 + 0x134) + 0x1f >> 5;
        sVar5 = (short)uVar6 + -1;
        if (-1 < sVar5) {
          puVar9 = param_1 + sVar5;
          uVar6 = uVar6 & 0xffff;
          do {
            *puVar9 = *puVar9 | *(uint *)(((iVar2 + (iVar7 + 0x1f >> 5) * (int)sVar1 * 4) -
                                          (int)param_1) + (int)puVar9);
            puVar9 = puVar9 + -1;
            uVar6 = uVar6 - 1;
          } while (uVar6 != 0);
        }
      }
    }
    iVar7 = data_iterator_next();
  }
  if (sVar4 != -1) {
    bit_vector_or(param_1);
  }
  return;
}
#endif
