// object_block_data_grow  (named by types/objects.h's own object_block_reference comment and
// object 0x1e8/0x1ec/0x1f0 field comments: "object_block_data_grow(field_offset, byte_count)
// appends byte_count zeroed bytes to the pool block, then writes {size, offset} at
// object + field_offset")
// address 0x4f7e50, size 156 bytes
// name confidence: 0.85 (fixed by types/objects.h's own citation of this address by this name)
// rewrite confidence: 0.55
// evidence: types/objects.h object_block_reference (size 0x00, offset 0x02), object_header
//   (block_size 0x06, data 0x08); global 0x008603b0 object_data, global 0x006b8cb4
//   object_memory_pool; callee block_list_reallocate (0x4d1de0, memory module, not otherwise
//   declared in this codebase yet).
// register convention: object index in EAX, {field_offset, extra_size} as stack parameters
//   (Ghidra's own "FUN_004f7e50(short param_1,short param_2)"). Confirmed against objdump
//   -d -M intel bin/halo.exe: 0x4f7e50 references eax as the object index with no prior stack
//   read, and param_1/param_2 both come from further up the stack once pushes are accounted
//   for.
//   // blam-cc: EAX -> object_index, stack -> field_offset, extra_size
// UNSURE: block_list_reallocate's full signature is guessed from this single call site
//   (arena pointer only); its true parameter list (matching block_list_allocate's
//   arena/size/owner shape) is not established here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern memory_pool *object_memory_pool; // 0x006b8cb4

extern uint8_t block_list_reallocate(memory_pool *arena); // 0x4d1de0, UNSURE: see file header

void object_block_data_grow(uint32_t object_index, int16_t field_offset, int16_t extra_size)
    // blam-cc: EAX -> object_index, stack -> field_offset, extra_size
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);

    if (block_list_reallocate(object_memory_pool) != 0) {
        object_block_reference *field = (object_block_reference *)((uint8_t *)header->data + field_offset);
        int16_t old_size = header->block_size;
        uint8_t *new_space = (uint8_t *)header->data + old_size;
        int16_t i;

        header->block_size = old_size + extra_size;
        field->offset = old_size;
        field->size = extra_size;

        for (i = 0; i < extra_size; i++) {
            new_space[i] = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f7e50):

uint FUN_004f7e50(short param_1,short param_2)

{
  short sVar1;
  int iVar2;
  uint in_EAX;
  uint uVar3;
  short *psVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;

  iVar6 = (in_EAX & 0xffff) * 0xc;
  iVar5 = *(int *)(DAT_008603b0 + 0x34) + iVar6;
  uVar3 = block_list_reallocate(DAT_006b8cb4);
  iVar2 = DAT_008603b0;
  if ((char)uVar3 != '\0') {
    sVar1 = *(short *)(iVar5 + 6);
    *(short *)(iVar5 + 6) = sVar1 + param_2;
    psVar4 = (short *)((int)param_1 + *(int *)(*(int *)(iVar2 + 0x34) + 8 + iVar6));
    psVar4[1] = sVar1;
    *psVar4 = param_2;
    puVar7 = (undefined4 *)((int)sVar1 + *(int *)(iVar5 + 8));
    for (uVar3 = (uint)(int)param_2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    for (uVar3 = (int)param_2 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
      *(undefined1 *)puVar7 = 0;
      puVar7 = (undefined4 *)((int)puVar7 + 1);
    }
    return 1;
  }
  return uVar3 & 0xffffff00;
}
#endif
