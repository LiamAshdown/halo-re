// decal_new  (Ghidra: FUN_0044dd90; named per out/phase4/effects_types_notes.md, which refers to
// this address as decal_new directly: "decal_new 0x44dd90 and decal_link 0x44dd30 own flags,
// cluster_index, layer and the list links")
// address 0x44dd90, size 499 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: types/effects.h decal_flags, decal_grid.temporary_count/object_count,
// k_maximum_temporary_decals/k_temporary_decal_eviction_target/k_decal_eviction_attempt_limit/
// k_decal_permanent_percent/k_decal_evict_percent (all named directly from this function's
// literals: 0x200, 0x100, 99, 0x9fff6, 0x28ffd7).
// blam-cc: EAX -> requested_handle, stack -> (cluster_index, layer, insert_before, object_attached)
// REWRITTEN 2026-09-27 (static loop) from objdump 0x44dd90..0x44df82:
//  * the datum is allocated with datum_new_at_index_with_salt(EAX = requested_handle, EDX = decal_data); decal_place
//    passes the handle of the decal's geometry block (cache_allocate_block on 0x71d1c0), so a decal datum shares its
//    index and salt with its vertex block. The draft passed -1, which always fails: no decal was ever created.
//  * when the eviction scan runs off the end of the array the iterator is RESTARTED (index -1) and a restart
//    counter is bumped; 100 restarts abandon the allocation (returning -1 and leaking the datum, as the original does).
//    The draft kept calling next() on an exhausted iterator.
//  * the insert-before splice stores the NEW handle into the new decal's previous link (0x44df57 `mov [ebp+0x30],
//    edx` with EDX = the new handle), not the old previous decal -- preserved literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include <stdint.h>

extern data_array *decal_data;       // 0x0087abe4
extern decal_grid *decal_grid_block; // 0x006b0ad8
extern random_seed effect_random_seed; // 0x00719cd4

extern datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array); // 0x4d03d0
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; iterator in EDI
extern void decal_link(int16_t cluster_index, datum_index decal_index, int16_t layer); // 0x44dd30,
    // this module; blam-cc: EBX -> cluster_index, ESI -> decal_index, EDI -> layer

// Allocates the decal datum at requested_handle. A non object-attached decal is randomly classed temporary
// (k_decal_permanent_percent), and if that pushes the temporary count over k_maximum_temporary_decals, temporary
// decals are un-flagged (each with a k_decal_evict_percent chance, or always when off the grid) until the count
// drops to k_temporary_decal_eviction_target. The new decal is spliced in before `insert_before`, or linked at the
// head of the (layer, cluster_index) list.
datum_index decal_new(datum_index requested_handle, int16_t cluster_index, int16_t layer, datum_index insert_before,
    uint8_t object_attached)
{
    datum_index handle = datum_new_at_index_with_salt(requested_handle, decal_data);
    decal *self;

    if (handle == k_datum_index_none) {
        return handle;
    }
    self = &((decal *)decal_data->data)[(uint16_t)handle];

    if (object_attached != 0) {
        self->flags = _decal_object_attached_bit;
        decal_grid_block->object_count = decal_grid_block->object_count + 1;
    } else {
        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        if ((int32_t)((effect_random_seed >> k_random_value_shift) * 100) < k_decal_permanent_percent) {
            self->flags = _decal_temporary_bit;
            decal_grid_block->temporary_count = decal_grid_block->temporary_count + 1;

            if (decal_grid_block->temporary_count > k_maximum_temporary_decals) {
                data_iterator iterator;
                int16_t restarts = 0;

                iterator.data = decal_data;
                iterator.next_index = 0;
                iterator.index = k_datum_index_none;
                iterator.signature = (uint32_t)(uintptr_t)decal_data ^ k_data_iterator_signature;

                while (decal_grid_block->temporary_count > k_temporary_decal_eviction_target) {
                    decal *candidate = (decal *)data_iterator_next(&iterator);

                    if (candidate == 0) {
                        // 0x44deae: restart the scan from the beginning
                        restarts = (int16_t)(restarts + 1);
                        iterator.data = decal_data;
                        iterator.next_index = 0;
                        iterator.index = k_datum_index_none;
                        iterator.signature = (uint32_t)(uintptr_t)decal_data ^ k_data_iterator_signature;
                        if (restarts >= k_decal_eviction_attempt_limit + 1) {
                            return k_datum_index_none;
                        }
                    } else if ((candidate->flags & _decal_temporary_bit) != 0) {
                        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                        if ((int32_t)((effect_random_seed >> k_random_value_shift) * 100) < k_decal_evict_percent ||
                            candidate->cluster_index == -1) {
                            candidate->flags = candidate->flags & ~_decal_temporary_bit;
                            decal_grid_block->temporary_count = decal_grid_block->temporary_count - 1;
                        }
                    }
                }
            }
        } else {
            self->flags = 0;
        }
    }

    if (insert_before != k_datum_index_none) {
        decal *before = &((decal *)decal_data->data)[(uint16_t)insert_before];
        datum_index previous = before->previous_decal;

        if (previous == k_datum_index_none) {
            decal_grid_block->cluster_first[layer][cluster_index] = handle;
        } else {
            ((decal *)decal_data->data)[(uint16_t)previous].next_decal = handle;
        }
        before->previous_decal = handle;
        self->next_decal = insert_before;
        self->cluster_index = cluster_index;
        self->previous_decal = handle; // sic, 0x44df57
        self->layer = layer;
        return handle;
    }

    decal_link(cluster_index, handle, layer);
    return handle;
}

#if 0
Original Ghidra decompilation (0x44dd90):

uint FUN_0044dd90(short param_1,short param_2,uint param_3,char param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  short sVar4;
  uint uVar5;
  int iVar6;
  int iVar7;

  iVar1 = DAT_0087abe4;
  uVar5 = datum_new_at_index_with_salt();
  iVar3 = DAT_006b0ad8;
  if (uVar5 != 0xffffffff) {
    iVar7 = (uVar5 & 0xffff) * 0x38 + *(int *)(iVar1 + 0x34);
    if (param_4 == '\0') {
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      if ((DAT_00719cd4 >> 0x10) * 100 < 0x9fff6) {
        *(undefined2 *)(iVar7 + 2) = 1;
        iVar6 = *(int *)(iVar3 + 0x2804) + 1;
        *(int *)(iVar3 + 0x2804) = iVar6;
        if (0x200 < iVar6) {
          sVar4 = 0;
          while (0x100 < iVar6) {
            iVar6 = data_iterator_next();
            if (iVar6 == 0) {
              sVar4 = sVar4 + 1;
              if (99 < sVar4) {
                return 0xffffffff;
              }
            }
            else if (((*(byte *)(iVar6 + 2) & 1) != 0) &&
                    ((DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f,
                     (DAT_00719cd4 >> 0x10) * 100 < 0x28ffd7 || (*(short *)(iVar6 + 4) == -1)))) {
              *(byte *)(iVar6 + 2) = *(byte *)(iVar6 + 2) & 0xfe;
              *(int *)(iVar3 + 0x2804) = *(int *)(iVar3 + 0x2804) + -1;
            }
            iVar6 = *(int *)(iVar3 + 0x2804);
          }
        }
      }
      else {
        *(undefined2 *)(iVar7 + 2) = 0;
      }
    }
    else {
      *(undefined2 *)(iVar7 + 2) = 2;
      *(int *)(iVar3 + 0x2808) = *(int *)(iVar3 + 0x2808) + 1;
    }
    if (param_3 != 0xffffffff) {
      iVar1 = *(int *)(iVar1 + 0x34);
      iVar6 = (param_3 & 0xffff) * 0x38;
      uVar2 = *(uint *)(iVar6 + 0x30 + iVar1);
      if (uVar2 == 0xffffffff) {
        *(uint *)(iVar3 + (param_2 * 0x200 + (int)param_1) * 4) = uVar5;
      }
      else {
        *(uint *)((uVar2 & 0xffff) * 0x38 + 0x34 + iVar1) = uVar5;
      }
      *(uint *)(iVar6 + iVar1 + 0x30) = uVar5;
      *(uint *)(iVar7 + 0x34) = param_3;
      *(short *)(iVar7 + 4) = param_1;
      *(uint *)(iVar7 + 0x30) = uVar5;
      *(short *)(iVar7 + 6) = param_2;
      return uVar5;
    }
    FUN_0044dd30();
  }
  return uVar5;
}
#endif
