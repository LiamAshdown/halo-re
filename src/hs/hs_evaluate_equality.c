// hs_evaluate_equality  (not a Ghidra function; the evaluate handler of hs functions 13 "=" and 14 "!=")
// address 0x4893e0, size 173 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[13]/[14] -> records 0x6577cc/0x6577e8 (boolean, special parse
//   0x485060); evaluate (+0xc) 0x4893e0, only reachable through those pointers. Campaign track: a10's scripts.
// objdump 0x4893e0..0x48948c: both arguments are evaluated as the type of the first one (the type word +0x04 of the
//   call node's second child: thread (0x218 each) +0x10 frame -> +0x04 call node -> +0x10 first child -> +0x08
//   next); the two-entry type array lives in the thread_index argument slot. Equal when the first
//   hs_type_sizes (0x00657568) bytes of the two values match (repz cmpsb, a zero size counts as equal); "!="
//   (function 14) inverts; the byte is returned in a dword whose upper bytes are the function_index slot's.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474
extern int16_t hs_type_sizes[];    // 0x00657568


void hs_evaluate_equality(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t *syntax = (uint8_t *)hs_syntax_data->data;
    uint8_t *frame = *(uint8_t **)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218 + 0x10);
    uint32_t call_node = *(uint32_t *)(frame + 4) & 0xffff;
    uint32_t name_node = *(uint32_t *)(syntax + call_node * 0x14 + 0x10) & 0xffff;
    uint32_t first_argument = *(uint32_t *)(syntax + name_node * 0x14 + 8) & 0xffff;
    int16_t type = *(int16_t *)(syntax + first_argument * 0x14 + 4);
    int16_t types[2];
    uint8_t *arguments;
    uint8_t equal;
    int32_t size;
    int32_t i;

    types[0] = type;
    types[1] = type;
    arguments = (uint8_t *)hs_evaluate_typed_arguments(thread_index, 2, types, first);
    if (arguments == 0) {
        return;
    }
    size = hs_type_sizes[type];
    equal = 1;
    for (i = 0; i < size; i++) {
        if (arguments[i] != arguments[4 + i]) {
            equal = 0;
            break;
        }
    }
    if (function_index == 0xe) {
        equal = (uint8_t)(equal == 0);
    }
    hs_thread_return((int32_t)equal, thread_index);
}
