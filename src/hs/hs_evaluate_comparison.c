// hs_evaluate_comparison  (not a Ghidra function; the evaluate handler of hs functions 15 ">", 16 "<", 17 ">=",
//   18 "<=")
// address 0x489490, size 440 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[15..18] -> records 0x657804..0x657858 (boolean, special parse
//   0x485150); evaluate (+0xc) 0x489490, only reachable through those pointers. Campaign track: a10's scripts.
// objdump 0x489490..0x489615 (+ three jump tables to 0x489648): both arguments are evaluated as the first one's
//   type (found as in hs_evaluate_equality), the type pair kept in the globals 0x006b15e4/0x006b15e6. The first
//   value stays on the x87 stack (a real as loaded, a long or short exactly by fild) and the second is stored
//   as a float; fcomp then decides by function index - 15 (the tables at 0x489618/28/38 all map 0 > 1 < 2 >= 3
//   <=; anything else is false). Every comparison is false for NaN. The byte is returned in a dword whose upper
//   bytes are the thread_index slot's.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *hs_thread_data;        // 0x0087a470
extern data_array *hs_syntax_data;        // 0x0087a474
extern int16_t hs_comparison_types[2];    // 0x006b15e4

extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640

void hs_evaluate_comparison(int16_t function_index, uint32_t thread_index, char first)
{
    uint8_t *syntax = (uint8_t *)hs_syntax_data->data;
    uint8_t *frame = *(uint8_t **)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218 + 0x10);
    uint32_t call_node = *(uint32_t *)(frame + 4) & 0xffff;
    uint32_t name_node = *(uint32_t *)(syntax + call_node * 0x14 + 0x10) & 0xffff;
    uint32_t first_argument = *(uint32_t *)(syntax + name_node * 0x14 + 8) & 0xffff;
    int16_t type = *(int16_t *)(syntax + first_argument * 0x14 + 4);
    int32_t *arguments;
    double a;
    float b;
    uint8_t result = 0;

    hs_comparison_types[1] = type;
    hs_comparison_types[0] = type;
    arguments = hs_evaluate_typed_arguments(thread_index, 2, hs_comparison_types, first);
    if (arguments == 0) {
        return;
    }
    if (hs_comparison_types[0] == 6) {
        a = *(float *)&arguments[0];
        b = *(float *)&arguments[1];
    } else if (hs_comparison_types[0] == 8) {
        a = (double)arguments[0];
        b = (float)arguments[1];
    } else {
        a = (double)*(int16_t *)&arguments[0];
        b = (float)*(int16_t *)&arguments[1];
    }
    switch (function_index - 15) {
    case 0: result = (uint8_t)(a > (double)b); break;
    case 1: result = (uint8_t)(a < (double)b); break;
    case 2: result = (uint8_t)(a >= (double)b); break;
    case 3: result = (uint8_t)(a <= (double)b); break;
    default: break;
    }
    hs_thread_return((int32_t)((thread_index & 0xffffff00) | result), thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
