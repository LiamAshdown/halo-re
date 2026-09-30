// cache_allocate_block  (Ghidra: FUN_004d1840)
// address 0x4d1840, size 984 bytes
// name confidence: 0.8 (out/phase4/memory_types_notes.md names it directly)
// rewrite confidence: 0.85
// REWRITTEN 2026-09-27 (static loop) from objdump 0x4d1840..0x4d1c17 (the draft was a goto-for-goto Ghidra
// transliteration whose ring-buffer bookkeeping could not be checked). The scan walks the entries in offset order.
// A 256-slot ring of open windows (cache_allocation_gap) starts a new window at every step (while the ring has
// room); free space and evictable entries grow every open window, each window remembering the newest age it
// would evict. A window that reaches blocks_needed closes (window_start advances) and becomes the best
// candidate when it is the first, has an older newest_age, or the same age and a smaller size. A protected entry
// (in_use_procedure says so, or it was touched this age) closes every open window. The oldest unprotected entry is
// remembered as the LRU. Then: every entry overlapping the chosen blocks is evicted, the LRU is evicted too when
// the entry table is full, and a new entry is linked after the window's previous entry.
// blam-cc: stack -> (self, requested_bytes)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "fn_memory.h"
#include <stdint.h>

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, EDI iterator
extern void cache_evict_entry(datum_index handle, cache *self); // 0x4d1c20, EBX handle, EDI self


static cache_entry *cache_entry_at(cache *self, datum_index handle)
{
    return (cache_entry *)((uint8_t *)self->entries->data + (uint32_t)(uint16_t)handle * sizeof(cache_entry));
}

datum_index cache_allocate_block(cache *self, uint32_t requested_bytes)
{
    cache_allocation_gap gaps[256];                 // esp+0x58
    cache_allocation_gap best;                      // esp+0x30
    int32_t blocks_needed;                          // esp+0x20
    datum_index cursor = self->first;               // esp+0x1c
    datum_index gap_previous = k_datum_index_none;  // esp+0x2c
    datum_index lru = k_datum_index_none;           // esp+0x24
    uint32_t lru_age = 0;                           // esp+0x44
    int16_t window_start = 0;                       // esp+0x18
    int16_t write = 0;                              // esp+0x14
    int32_t offset = 0;                             // edi
    uint8_t found = 0;                              // esp+0x13
    datum_index handle;
    cache_entry *entry;

    blocks_needed = (int32_t)requested_bytes >> self->block_shift;
    if ((requested_bytes & ((1u << self->block_shift) - 1u)) != 0) {
        blocks_needed++;
    }
    best.previous_entry = k_datum_index_none;
    best.newest_age = 0;
    best.offset = 0;
    best.size = 0;

    if (self->block_count <= 0) {
        return k_datum_index_none;
    }

    do {
        int16_t next = (write == 0xff) ? 0 : (int16_t)(write + 1);
        int32_t gap;
        uint32_t entry_age;                         // esp+0x28

        if (next != window_start) {
            gaps[write].previous_entry = gap_previous;
            gaps[write].newest_age = 0;
            gaps[write].offset = offset;
            gaps[write].size = 0;
            write = next;
        }

        if (cursor == k_datum_index_none) {
            entry_age = 0;
            gap = self->block_count - offset;
            offset = self->block_count;
        } else {
            entry = cache_entry_at(self, cursor);
            if (offset != entry->offset) {
                gap = entry->offset - offset;
                entry_age = 0;
                offset = entry->offset;
            } else {
                uint8_t protected_entry = 0;

                entry_age = entry->age;
                gap = entry->size;
                if (self->in_use_procedure != 0 &&
                    (uint8_t)((int32_t (*)(datum_index))self->in_use_procedure)(cursor) != 0) {
                    protected_entry = 1;
                }
                if (entry->age == self->age) {
                    protected_entry = 1;
                } else if (!protected_entry && (lru == k_datum_index_none || entry->age < lru_age)) {
                    lru = cursor;
                    lru_age = entry->age;
                }
                offset = entry->size + entry->offset;
                gap_previous = cursor;
                cursor = entry->next;
                if (protected_entry) {
                    window_start = write; // 0x4d1a2a: every open window ends here
                    continue;
                }
            }
        }

        {
            int16_t scan = window_start;

            while (scan != write) {
                cache_allocation_gap *window = &gaps[scan];

                if (entry_age > window->newest_age) {
                    window->newest_age = entry_age;
                }
                window->size += gap;
                if (window->size >= blocks_needed) {
                    if (!found || window->newest_age < best.newest_age ||
                        (window->newest_age == best.newest_age && window->size < best.size)) {
                        best = *window;
                        found = 1;
                    }
                    window_start = (window_start == 0xff) ? 0 : (int16_t)(window_start + 1);
                }
                scan = (scan == 0xff) ? 0 : (int16_t)(scan + 1);
            }
        }
    } while (offset < self->block_count);

    if (!found) {
        return k_datum_index_none;
    }

    {
        data_iterator iterator;
        cache_entry *candidate;

        iterator.data = self->entries;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)self->entries ^ k_data_iterator_signature;
        while ((candidate = (cache_entry *)data_iterator_next(&iterator)) != 0) {
            if (candidate->offset < blocks_needed + best.offset && candidate->size + candidate->offset > best.offset) {
                cache_evict_entry(iterator.index, self);
            }
        }
    }

    if (self->entries->actual_count == self->entries->maximum_count && lru != k_datum_index_none) {
        if (best.previous_entry == lru) {
            best.previous_entry = cache_entry_at(self, lru)->previous;
        }
        cache_evict_entry(lru, self);
    }

    handle = datum_new(self->entries);
    if (handle == k_datum_index_none) {
        return handle;
    }
    entry = cache_entry_at(self, handle);
    if (best.previous_entry == k_datum_index_none) {
        entry->previous = k_datum_index_none;
        if (self->first == k_datum_index_none) {
            self->last = handle;
        } else {
            cache_entry_at(self, self->first)->previous = handle;
        }
        entry->next = self->first;
        self->first = handle;
    } else {
        cache_entry *previous = cache_entry_at(self, best.previous_entry);

        if (previous->next == k_datum_index_none) {
            entry->previous = self->last;
            self->last = handle;
        } else {
            cache_entry *following = cache_entry_at(self, previous->next);

            entry->previous = following->previous;
            following->previous = handle;
        }
        entry->next = previous->next;
        previous->next = handle;
    }
    entry->offset = best.offset;
    entry->size = blocks_needed;
    entry->age = self->age;
    return handle;
}

#if 0
Original Ghidra decompilation (0x4d1840):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_004d1840(int param_1,uint param_2)

{
  bool bVar1;
  bool bVar2;
  char cVar3;
  uint uVar4;
  int iVar5;
  byte bVar6;
  short sVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  int local_1044;
  int local_1040;
  uint local_103c;
  int local_1038;
  uint local_1034;
  uint local_1030;
  uint local_102c;
  uint local_1028;
  uint local_1024;
  uint local_1020;
  uint local_101c;
  uint uStack_1014;
  uint local_1000 [1023];
  undefined4 uStack_4;

  uStack_4 = 0x4d184a;
  bVar6 = (byte)*(undefined4 *)(param_1 + 0x2c);
  local_1038 = (int)param_2 >> (bVar6 & 0x1f);
  if ((param_2 & (1 << (bVar6 & 0x1f)) - 1U) != 0) {
    local_1038 = local_1038 + 1;
  }
  local_103c = *(uint *)(param_1 + 0x34);
  uVar4 = 0xffffffff;
  iVar8 = 0;
  bVar2 = false;
  local_1034 = 0xffffffff;
  local_1040 = 0;
  local_1044 = 0;
  local_102c = 0xffffffff;
  uVar9 = 0;
  if (0 < *(int *)(param_1 + 0x28)) {
    do {
      sVar7 = (short)iVar8;
      if (sVar7 == 0xff) {
        iVar5 = 0;
      }
      else {
        iVar5 = sVar7 + 1;
      }
      if (iVar5 != (short)local_1040) {
        iVar8 = (int)sVar7;
        local_1000[iVar8 * 4] = local_102c;
        local_1000[iVar8 * 4 + 2] = uVar9;
        local_1000[iVar8 * 4 + 1] = 0;
        local_1000[iVar8 * 4 + 3] = 0;
        if (sVar7 == 0xff) {
          local_1044 = 0;
          iVar8 = local_1044;
        }
        else {
          iVar8 = iVar8 + 1;
          local_1044 = iVar8;
        }
      }
      if (local_103c == 0xffffffff) {
        uVar4 = *(uint *)(param_1 + 0x28);
        local_1030 = 0;
        iVar5 = uVar4 - uVar9;
LAB_004d1913:
        iVar10 = local_1040;
        iVar11 = local_1040;
        if ((short)local_1040 != (short)iVar8) {
          do {
            iVar8 = (int)(short)iVar10;
            if (local_1000[iVar8 * 4 + 1] < local_1030) {
              local_1000[iVar8 * 4 + 1] = local_1030;
            }
            uVar9 = local_1000[iVar8 * 4 + 3] + iVar5;
            local_1000[iVar8 * 4 + 3] = uVar9;
            if (local_1038 <= (int)uVar9) {
              if (((!bVar2) || (local_1000[iVar8 * 4 + 1] < local_1024)) ||
                 ((local_1000[iVar8 * 4 + 1] == local_1024 && ((int)uVar9 < (int)local_101c)))) {
                local_1028 = local_1000[iVar8 * 4];
                local_1024 = local_1000[iVar8 * 4 + 1];
                local_1020 = local_1000[iVar8 * 4 + 2];
                local_101c = local_1000[iVar8 * 4 + 3];
                bVar2 = true;
              }
              if ((short)local_1040 == 0xff) {
                local_1040 = 0;
              }
              else {
                local_1040 = local_1040 + 1;
              }
            }
            if ((short)iVar10 == 0xff) {
              iVar10 = 0;
            }
            else {
              iVar10 = iVar8 + 1;
            }
            iVar8 = local_1044;
            iVar11 = local_1040;
          } while ((short)iVar10 != (short)local_1044);
        }
      }
      else {
        iVar5 = *(int *)(*(int *)(param_1 + 0x3c) + 0x34);
        iVar11 = (local_103c & 0xffff) * 0x1c;
        iVar10 = iVar11 + iVar5;
        if (uVar9 != *(uint *)(iVar11 + 8 + iVar5)) {
          uVar4 = *(uint *)(iVar10 + 8);
          iVar5 = uVar4 - uVar9;
          local_1030 = 0;
          goto LAB_004d1913;
        }
        local_1030 = *(uint *)(iVar10 + 0x14);
        iVar5 = *(int *)(iVar10 + 4);
        if ((*(code **)(param_1 + 0x24) == (code *)0x0) ||
           (cVar3 = (**(code **)(param_1 + 0x24))(local_103c), iVar8 = local_1044, cVar3 == '\0')) {
          bVar1 = false;
        }
        else {
          bVar1 = true;
        }
        uVar9 = *(uint *)(iVar10 + 0x14);
        if (uVar9 == *(uint *)(param_1 + 0x30)) {
          bVar1 = true;
        }
        else if ((!bVar1) && ((local_1034 == 0xffffffff || (uVar9 < uStack_1014)))) {
          local_1034 = local_103c;
          uStack_1014 = uVar9;
        }
        uVar4 = *(int *)(iVar10 + 8) + *(int *)(iVar10 + 4);
        local_102c = local_103c;
        local_103c = *(uint *)(iVar10 + 0xc);
        iVar11 = iVar8;
        if (!bVar1) goto LAB_004d1913;
      }
      local_1040 = iVar11;
      uVar9 = uVar4;
    } while ((int)uVar4 < *(int *)(param_1 + 0x28));
    if (bVar2) {
      iVar8 = data_iterator_next();
      if (iVar8 != 0) {
        do {
          if ((*(int *)(iVar8 + 8) < (int)(local_1038 + local_1020)) &&
             ((int)local_1020 < *(int *)(iVar8 + 4) + *(int *)(iVar8 + 8))) {
            cache_evict_entry();
          }
          iVar8 = data_iterator_next();
        } while (iVar8 != 0);
      }
      iVar8 = *(int *)(param_1 + 0x3c);
      if ((*(short *)(iVar8 + 0x30) == *(short *)(iVar8 + 0x20)) && (local_1034 != 0xffffffff)) {
        if (local_1028 == local_1034) {
          local_1028 = *(uint *)((local_1034 & 0xffff) * 0x1c + 0x10 + *(int *)(iVar8 + 0x34));
        }
        cache_evict_entry();
      }
      uVar4 = datum_new();
      if (uVar4 != 0xffffffff) {
        iVar8 = *(int *)(*(int *)(param_1 + 0x3c) + 0x34);
        iVar5 = (uVar4 & 0xffff) * 0x1c + iVar8;
        if (local_1028 == 0xffffffff) {
          uVar9 = *(uint *)(param_1 + 0x34);
          *(undefined4 *)(iVar5 + 0x10) = 0xffffffff;
          if (uVar9 == 0xffffffff) {
            *(uint *)(param_1 + 0x38) = uVar4;
            *(undefined4 *)(iVar5 + 0xc) = *(undefined4 *)(param_1 + 0x34);
            *(uint *)(param_1 + 0x34) = uVar4;
          }
          else {
            *(uint *)((uVar9 & 0xffff) * 0x1c + iVar8 + 0x10) = uVar4;
            *(undefined4 *)(iVar5 + 0xc) = *(undefined4 *)(param_1 + 0x34);
            *(uint *)(param_1 + 0x34) = uVar4;
          }
        }
        else {
          iVar11 = (local_1028 & 0xffff) * 0x1c;
          uVar9 = *(uint *)(iVar11 + 0xc + iVar8);
          if (uVar9 == 0xffffffff) {
            *(undefined4 *)(iVar5 + 0x10) = *(undefined4 *)(param_1 + 0x38);
            *(uint *)(param_1 + 0x38) = uVar4;
          }
          else {
            iVar8 = (uVar9 & 0xffff) * 0x1c + iVar8;
            *(undefined4 *)(iVar5 + 0x10) = *(undefined4 *)(iVar8 + 0x10);
            *(uint *)(iVar8 + 0x10) = uVar4;
          }
          iVar8 = *(int *)(*(int *)(param_1 + 0x3c) + 0x34);
          *(undefined4 *)(iVar5 + 0xc) = *(undefined4 *)(iVar8 + 0xc + iVar11);
          *(uint *)(iVar8 + iVar11 + 0xc) = uVar4;
        }
        *(uint *)(iVar5 + 8) = local_1020;
        *(int *)(iVar5 + 4) = local_1038;
        *(undefined4 *)(iVar5 + 0x14) = *(undefined4 *)(param_1 + 0x30);
        return uVar4;
      }
    }
    else {
      uVar4 = 0xffffffff;
    }
  }
  return uVar4;
}
#endif
