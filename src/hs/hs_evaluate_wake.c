// hs_evaluate_wake  (not a Ghidra function; the evaluate handler of hs function 21 "wake")
// address 0x489a20, size 116 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[21] -> record 0x6578ac (void, special parse 0x4853c0); evaluate
//   (+0xc) 0x489a20, only reachable through that pointer. Campaign track: a10's scripts.
// objdump 0x489a20..0x489a93: the script index is the argument node's constant word (+0x10 of the call node's
//   second child, found as in hs_evaluate_equality); its thread (hs_thread_find_by_script_index, stack word
//   zero-extended), when there is one, is restarted (hs_thread_restart, stack); returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

extern datum_index hs_thread_find_by_script_index(int16_t script_index); // 0x48a960
extern void hs_thread_restart(uint32_t thread_index); // 0x48a790
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640

void hs_evaluate_wake(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t *syntax = (uint8_t *)hs_syntax_data->data;
    uint8_t *frame = *(uint8_t **)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218 + 0x10);
    uint32_t call_node = *(uint32_t *)(frame + 4) & 0xffff;
    uint32_t name_node = *(uint32_t *)(syntax + call_node * 0x14 + 0x10) & 0xffff;
    uint32_t argument = *(uint32_t *)(syntax + name_node * 0x14 + 8) & 0xffff;
    datum_index thread = hs_thread_find_by_script_index(*(int16_t *)(syntax + argument * 0x14 + 0x10));

    (void)function_index;
    (void)first;
    if (thread != k_datum_index_none) {
        hs_thread_restart(thread);
    }
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
