// hs_evaluate_cls  (not a Ghidra function; the evaluate handler of hs "cls" (-> void))
// address 0x4826e0, size 53 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x4826e0, only reachable through that pointer.
//   Campaign track: 12 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x4826e0: when the terminal is initialized: both message indices to -1, data_delete_all (ESI) of the message array, console_clear_screen; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern uint8_t terminal_initialized; // 0x006b2efc
extern data_array *terminal_messages; // 0x006b2f00
extern int32_t console_message_head; // 0x006b2f04
extern int32_t console_message_tail; // 0x006b2f08
extern void data_delete_all(data_array *array); // 0x4d0580, ESI
extern void console_clear_screen(void); // 0x496f90

void hs_evaluate_cls(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    if (terminal_initialized != 0) {
        console_message_head = -1;
        console_message_tail = -1;
        data_delete_all(terminal_messages);
        console_clear_screen();
    }
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
