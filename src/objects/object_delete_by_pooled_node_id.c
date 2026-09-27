// object_delete_by_pooled_node_id
// address 0x4f5b50, size 117 bytes
// name confidence: 0.3 (still FUN_004f5b50 in Ghidra; functions.md's summary -- "Deletes the
//   object associated with a pooled marker/attachment id after validating engine-state
//   preconditions" -- matches the shape of the code; the 0x00687130 table is the same one read
//   by object_type_override_call_0x70_release_node elsewhere in this batch)
// rewrite confidence: 0.4 (raised from 0.3 by the phase-4 review pass: network_index_cache_remove's two register arguments were resolved from the disassembly)
// evidence: types/objects.h object_header (flags at 0x02); global 0x008603b0 object_data;
//   callees object_try_and_get 0x4f6ec0, object_delete_recursive 0x4f59d0 (this batch).
// register convention: some caller-owned record pointer in EAX (in_EAX, same shape as
//   object_type_override_call_0x70_release_node's first argument), pooled node id in ECX
//   (in_ECX).
// UNSURE: the exact struct at in_EAX and the table at 0x00687130 are unidentified, same as in
//   object_type_override_call_0x70_release_node; offsets kept raw. UNSURE: message_delta_decode_compound_field and
//   message_delta_decode_compound_field_staged are unexamined outside this batch; their argument lists are not visible here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t *object_pooled_node_globals; // 0x00687130, see object_type_override_call_0x70_release_node.c
extern void *object_pooled_node_globals_006870d8; // 0x006870d8, UNSURE: network_index_cache_remove's EAX operand

extern uint8_t message_delta_decode_compound_field_staged(void **context); // 0x4ec670, UNSURE: unexamined
extern int8_t message_delta_decode_compound_field(void *globals, void *out_value); // 0x4ec590, UNSURE: unexamined; validates engine-state preconditions
extern void network_index_cache_remove(void *globals, uint32_t object_index); // 0x4e9d40.
    // Resolved from `objdump -d -M intel bin/halo.exe`: both call sites in this module set
    // EAX to the literal 0x006870d8 and ESI to the object index immediately before the call
    // (0x4f5b16 `mov esi,edi / mov eax,0x6870d8` and 0x4f5b96 `mov eax,0x6870d8` with ESI
    // already holding the index). Ghidra shows neither, so it used to be declared with the
    // object index alone.
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0, this batch

void object_delete_by_pooled_node_id(int32_t **record, uint32_t pooled_node_id)
    // blam-cc: EAX -> record, ECX -> pooled_node_id
{
    char preconditions_ok;
    uint32_t object_index;
    object_header *header;
    int32_t node_table;

    if (**record != 0) {
        message_delta_decode_compound_field_staged(0); // UNSURE: see file header
        return;
    }

    preconditions_ok = message_delta_decode_compound_field(record, &pooled_node_id);
        // 0x4f5b58 lea ecx,[esp] -- ECX is the out slot, EAX the globals pointer
    if (preconditions_ok != 0 && pooled_node_id != 0) {
        node_table = *(int32_t *)(object_pooled_node_globals + 0x28);
        object_index = *(uint32_t *)(node_table + pooled_node_id * 4);
        if (object_index != 0xffffffff) {
            header = (object_header *)object_data->data + (object_index & 0xffff);
            if ((header->flags & _object_header_delete_pending_bit) == 0) {
                network_index_cache_remove(&object_pooled_node_globals_006870d8, object_index); // FIXED: EAX is the container ADDRESS 0x6870d8 (mov eax,imm); its dword is 0xd, not a pointer
            }
            if (object_try_and_get(object_index, _object_mask_all) != 0) {
                // 0x4f5ba0 push -1 / mov ecx,esi
                object_delete_recursive(object_index, 0);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f5b50):

void FUN_004f5b50(void)

{
  uint uVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  int in_ECX;

  if (*(int *)*in_EAX != 0) {
    FUN_004ec670();
    return;
  }
  cVar2 = FUN_004ec590();
  if (((cVar2 != '\0') && (in_ECX != 0)) &&
     (uVar1 = *(uint *)(*(int *)(PTR_DAT_00687130 + 0x28) + in_ECX * 4), uVar1 != 0xffffffff)) {
    if ((*(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + (uVar1 & 0xffff) * 0xc) & 8) == 0) {
      FUN_004e9d40();
    }
    iVar3 = object_try_and_get(0xffffffff);
    if (iVar3 != 0) {
      FUN_004f59d0(uVar1,0);
    }
  }
  return;
}
#endif
