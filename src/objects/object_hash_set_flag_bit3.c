// object_hash_set_flag_bit3
// address 0x4ef200, size 149 bytes
// name confidence: 0.5 (Ghidra-recovered name; paired with object_hash_clear_flag_bit3)
// rewrite confidence: 0.4
// evidence: types/objects.h object.vitality_flags (0x106/0x107, _object_hash_flag_bit == byte
// 0x107 bit 3); the globals note in objects.h explicitly documents 0x0087a464/0x0087a468 as "the
// hash chain object_hash_set_flag_bit3 walks", not owned by this module.
// UNSURE: the two data_arrays at 0x0087a464/0x0087a468 belong to another module (likely the BSP
// cluster/collision system); their element layout is a generic {key, next, value}-style 0x0c
// stride, kept as raw offsets rather than a named struct.
// register convention: uint32_t key in EAX (in_EAX).
// blam-cc: EAX=key

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;    // 0x008603b0
extern data_array *object_list_header_data;     // 0x0087a464, UNSURE: not owned by this module
extern data_array *object_list_reference_data;     // 0x0087a468, UNSURE: not owned by this module

void object_hash_set_flag_bit3(uint32_t key)
{
    object_header *headers = (object_header *)object_data->data;
    uint32_t node = 0xffffffff;
    uint32_t next_node;

    if (key != 0xffffffff) {
        // objdump: mov eax,[edx+eax*4+0x8] -- the key's entry holds its first node at +8, not +0
        node = *(uint32_t *)((uint8_t *)object_list_header_data->data + (key & 0xffff) * 0xc + 8);
        if (node == 0xffffffff) {
            next_node = 0xffffffff;
        } else {
            uint8_t *entry = (uint8_t *)object_list_reference_data->data + (node & 0xffff) * 0xc;
            next_node = *(uint32_t *)(entry + 8);
            node = *(uint32_t *)(entry + 4);
        }
    } else {
        next_node = 0xffffffff;
    }

    while (node != 0xffffffff) {
        object *obj = headers[node & 0xffff].data;
        *((uint8_t *)obj + 0x107) |= 8;

        if (next_node == 0xffffffff) {
            node = 0xffffffff;
        } else {
            uint8_t *entry = (uint8_t *)object_list_reference_data->data + (next_node & 0xffff) * 0xc;
            next_node = *(uint32_t *)(entry + 8);
            node = *(uint32_t *)(entry + 4);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ef200):

void object_hash_set_flag_bit3(void)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint in_EAX;
  uint in_ECX;
  uint uVar5;

  iVar4 = DAT_0087a468;
  iVar3 = DAT_008603b0;
  uVar5 = 0xffffffff;
  if (in_EAX != 0xffffffff) {
    uVar5 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar5 == 0xffffffff) {
      uVar5 = 0xffffffff;
      in_ECX = 0xffffffff;
    }
    else {
      iVar2 = *(int *)(DAT_0087a468 + 0x34) + (uVar5 & 0xffff) * 0xc;
      in_ECX = *(uint *)(iVar2 + 8);
      uVar5 = *(uint *)(iVar2 + 4);
    }
  }
  while (uVar5 != 0xffffffff) {
    pbVar1 = (byte *)(*(int *)(*(int *)(iVar3 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc) + 0x107);
    *pbVar1 = *pbVar1 | 8;
    if (in_ECX == 0xffffffff) {
      uVar5 = 0xffffffff;
      in_ECX = 0xffffffff;
    }
    else {
      iVar2 = *(int *)(iVar4 + 0x34) + (in_ECX & 0xffff) * 0xc;
      in_ECX = *(uint *)(iVar2 + 8);
      uVar5 = *(uint *)(iVar2 + 4);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
