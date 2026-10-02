// hs_evaluate_object_cast  (not a Ghidra function; the evaluate handler of hs function 23 "unit")
// address 0x489c80, size 200 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[23] -> record 0x6578e4 (unit, special parse 0x4854f0); evaluate
//   (+0xc) 0x489c80, only reachable through that pointer. Campaign track: a10's scripts.
// objdump 0x489c80..0x489d47: takes a 4-byte scratch slot on the thread's current frame (frame +0x0c size,
//   +0x0e data). The first pass pushes the argument node (the call node's second child) with
//   hs_thread_push(EAX node, EDX thread, EBX slot) and returns; the next pass returns the object left in the slot
//   when its type bit (1 << object +0xb4) is in the mask hs_object_type_masks[function_index - 0x16]
//   (0x00657538: 0xffff, 3 for "unit", ...), else -1 (also for a -1 object).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474
extern data_array *object_data;    // 0x008603b0
extern int16_t hs_object_type_masks[]; // 0x00657538

extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address); // 0x48a560, EAX, EDX, EBX
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640

void hs_evaluate_object_cast(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t *frame = *(uint8_t **)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218 + 0x10);
    uint16_t size = *(uint16_t *)(frame + 0xc);
    datum_index *slot = (datum_index *)(frame + 0xe + (int16_t)size);
    datum_index object_index;

    *(uint16_t *)(frame + 0xc) = (uint16_t)(size + 4);
    if (first) {
        uint8_t *syntax = (uint8_t *)hs_syntax_data->data;
        uint32_t call_node = *(uint32_t *)(frame + 4) & 0xffff;
        uint32_t name_node = *(uint32_t *)(syntax + call_node * 0x14 + 0x10) & 0xffff;

        hs_thread_push(*(datum_index *)(syntax + name_node * 0x14 + 8), thread_index, slot);
        return;
    }
    object_index = *slot;
    if (object_index != k_datum_index_none) {
        uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);

        if ((int32_t)hs_object_type_masks[(int16_t)(function_index - 0x16)] & (1 << (object[0xb4] & 0x1f))) {
            hs_thread_return((int32_t)object_index, thread_index);
            return;
        }
    }
    hs_thread_return(-1, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
