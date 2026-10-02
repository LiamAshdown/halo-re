// heap_allocate_raw  (Ghidra: FUN_004d2180)
// address 0x4d2180, size 387 bytes
// name confidence: 0.75 (module summary: "Allocates a raw block from a bucketed/segregated
// free-list heap, compacting the heap first if necessary")
// rewrite confidence: 0.5 -- every field access, arithmetic expression and branch is preserved
// exactly; the four elided calls (heap_compact/_get_free_bytes/_find_free_block/
// _find_first_free_slot, all shown by Ghidra with empty argument lists) are reconstructed from
// their own already-rewritten signatures in this same batch, passing `self` (the only heap in
// scope) and, for heap_find_free_block, the rounded block size and the out-predecessor local.
// evidence: types/memory.h heap/heap_block layout; the inline free-bytes formula right before the
// first elided call is textually identical to heap_get_free_bytes.c's own formula, which is what
// pins down that this function's ECX-based callee is exactly heap_get_free_bytes.
// register convention: requested payload size in EAX (in_EAX), heap* in EDI (unaff_EDI).

#include "tags.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void heap_compact(heap *self); // this batch
extern int32_t heap_get_free_bytes(heap *self); // this batch
extern int32_t heap_find_free_block(heap *self, uint32_t size_needed, void **out_predecessor); // this batch
extern uint32_t heap_find_first_free_slot(heap *self); // this batch
extern void heap_advance_free_slot(heap *self); // this batch

uint32_t heap_allocate_raw(uint32_t size, heap *self)
{
    uint32_t block_size;
    int32_t free_bytes;
    int32_t found_block;
    heap_block *predecessor;
    int32_t slot;
    heap_block *block;
    uint32_t block_addr;

    if (size == 0 || 0x80000000u <= size || (uint32_t)self->size <= size) {
        return 0;
    }

    found_block = 0;
    predecessor = 0;
    for (block_size = size + 0x10; (block_size & 3) != 0; block_size = block_size + 1) {
        // rounds block_size up to a multiple of 4; the header itself is 0x10 bytes
        // (heap_block: size, slot, previous, next).
    }

    free_bytes = self->size;
    if (self->first_block != 0) {
        free_bytes = heap_get_free_bytes(self);
    }

    if ((uint32_t)free_bytes < block_size) {
        heap_compact(self);
        free_bytes = heap_get_free_bytes(self);
        if ((uint32_t)free_bytes < block_size) {
            found_block = heap_find_free_block(self, block_size, (void **)&predecessor);
            if (found_block == 0) {
                return 0;
            }
        }
    }

    if (self->next_free_slot == -1) {
        self->next_free_slot = (int32_t)heap_find_first_free_slot(self);
    }
    slot = self->next_free_slot;
    if (slot == -1) {
        return 0;
    }

    if (found_block == 0) {
        if (self->first_block == 0) {
            self->blocks[slot] = (heap_block *)self->base;
        } else {
            self->blocks[slot] = (heap_block *)((uint8_t *)self->last_block +
                (self->last_block->size & k_heap_block_size_mask));
        }
    } else {
        self->blocks[slot] = (heap_block *)found_block;
    }

    block = self->blocks[self->next_free_slot];
    block->size = block_size;
    block->slot = (int32_t)self->next_free_slot;
    block = self->blocks[self->next_free_slot];
    block_addr = (uint32_t)block;

    if (self->first_block == 0) {
        self->last_block = block;
        self->first_block = block;
        block->previous = 0;
        block->next = 0;
        heap_advance_free_slot(self);
        return block_addr;
    }
    if (block_addr < (uint32_t)self->first_block) {
        block->previous = 0;
        block->next = self->first_block;
        self->first_block->previous = block;
        self->first_block = block;
        heap_advance_free_slot(self);
        return block_addr;
    }
    if (block_addr <= (uint32_t)self->last_block) {
        block->previous = predecessor;
        block->next = predecessor->next;
        predecessor->next = block;
        if (block->next != 0) {
            block->next->previous = block;
        }
        heap_advance_free_slot(self);
        return block_addr;
    }
    block->next = 0;
    block->previous = self->last_block;
    self->last_block->next = block;
    self->last_block = block;
    heap_advance_free_slot(self);
    return block_addr;
}

#if 0
Original Ghidra decompilation (0x4d2180):

uint FUN_004d2180(void)

{
  int iVar1;
  uint *puVar2;
  uint in_EAX;
  uint uVar3;
  undefined4 uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int unaff_EDI;
  int local_4;

  if (((in_EAX != 0) && (in_EAX < 0x80000000)) && (uVar3 = *(uint *)(unaff_EDI + 8), in_EAX < uVar3)
     ) {
    iVar7 = 0;
    local_4 = 0;
    for (uVar5 = in_EAX + 0x10; (uVar5 & 3) != 0; uVar5 = uVar5 + 1) {
    }
    if (*(int *)(unaff_EDI + 0x2c) != 0) {
      uVar3 = ((*(int *)(unaff_EDI + 4) - (**(uint **)(unaff_EDI + 0x30) & 0x7fffffff)) -
              (int)*(uint **)(unaff_EDI + 0x30)) + uVar3;
    }
    iVar6 = 0;
    if (uVar3 < uVar5) {
      FUN_004d2310();
      uVar3 = FUN_004d20f0();
      if ((uVar3 < uVar5) && (iVar7 = FUN_004d2370(&local_4), iVar6 = local_4, iVar7 == 0)) {
        return 0;
      }
    }
    if (*(int *)(unaff_EDI + 0x10) == -1) {
      uVar4 = FUN_004d2110();
      *(undefined4 *)(unaff_EDI + 0x10) = uVar4;
    }
    iVar1 = *(int *)(unaff_EDI + 0x10);
    if (iVar1 != -1) {
      if (iVar7 == 0) {
        if (*(int *)(unaff_EDI + 0x2c) == 0) {
          *(undefined4 *)(unaff_EDI + 0x34 + iVar1 * 4) = *(undefined4 *)(unaff_EDI + 4);
        }
        else {
          *(uint *)(unaff_EDI + 0x34 + iVar1 * 4) =
               (**(uint **)(unaff_EDI + 0x30) & 0x7fffffff) + (int)*(uint **)(unaff_EDI + 0x30);
        }
      }
      else {
        *(int *)(unaff_EDI + 0x34 + iVar1 * 4) = iVar7;
      }
      uVar3 = *(uint *)(unaff_EDI + 0x10);
      puVar2 = *(uint **)(unaff_EDI + 0x34 + uVar3 * 4);
      *puVar2 = uVar5;
      puVar2[1] = uVar3;
      uVar3 = *(uint *)(unaff_EDI + 0x34 + *(int *)(unaff_EDI + 0x10) * 4);
      if (*(uint *)(unaff_EDI + 0x2c) == 0) {
        *(uint *)(unaff_EDI + 0x30) = uVar3;
        *(uint *)(unaff_EDI + 0x2c) = uVar3;
        *(undefined4 *)(uVar3 + 8) = 0;
        *(undefined4 *)(uVar3 + 0xc) = 0;
        FUN_004d2140();
        return uVar3;
      }
      if (uVar3 < *(uint *)(unaff_EDI + 0x2c)) {
        *(undefined4 *)(uVar3 + 8) = 0;
        *(undefined4 *)(uVar3 + 0xc) = *(undefined4 *)(unaff_EDI + 0x2c);
        *(uint *)(*(int *)(unaff_EDI + 0x2c) + 8) = uVar3;
        *(uint *)(unaff_EDI + 0x2c) = uVar3;
        FUN_004d2140();
        return uVar3;
      }
      uVar5 = *(uint *)(unaff_EDI + 0x30);
      if (uVar3 <= uVar5) {
        *(int *)(uVar3 + 8) = iVar6;
        *(undefined4 *)(uVar3 + 0xc) = *(undefined4 *)(iVar6 + 0xc);
        *(uint *)(iVar6 + 0xc) = uVar3;
        if (*(int *)(uVar3 + 0xc) != 0) {
          *(uint *)(*(int *)(uVar3 + 0xc) + 8) = uVar3;
        }
        FUN_004d2140();
        return uVar3;
      }
      *(undefined4 *)(uVar3 + 0xc) = 0;
      *(uint *)(uVar3 + 8) = uVar5;
      *(uint *)(*(int *)(unaff_EDI + 0x30) + 0xc) = uVar3;
      *(uint *)(unaff_EDI + 0x30) = uVar3;
      FUN_004d2140();
      return uVar3;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
