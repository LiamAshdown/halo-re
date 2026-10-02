// hs_evaluate_sleep_ticks  (not a Ghidra function; the evaluate handler of hs function 19 "sleep";
//   hs_evaluate_sleep.c @0x489800 is the "sleep_until" handler)
// address 0x489650, size 417 bytes
// name confidence: 0.9  rewrite confidence: 0.85
// evidence: hs_function_definitions 0x688b58[19] -> record 0x657874, name "sleep", evaluate
//   (+0xc) 0x489650; only reachable through that pointer. First-boot track: ui.map's scripts call
//   it 11 times (scratchpad ui_hs_calls.py over the scenario's script nodes).
// objdump 0x489650..0x4897f0: scratch slots ticks (4), script (4), state (2).
//   First call: pushes the ticks expression (the call's second child) into the ticks slot;
//   state = 0.
//   state 0 on a later call: state = 1; the optional script expression (ticks' next_node) is
//   pushed into the script slot, or the script slot is set to -1 when there is none.
//   Then: a word tick count of 0 returns at once. The thread to put to sleep is this one, or
//   hs_thread_find_by_script_index(script) when the script word is not -1; none returns. Its
//   new wake tick is -2 for a negative count, else game_time + count. A thread whose wake tick
//   is -1 is left alone; another thread without the wake_saved flag first gets the flag and its
//   old wake tick saved (+0xc). Always returns 0 (hs_thread_return, ECX = this thread).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_push(datum_index node, uint32_t thread_index, void *result_address); // 0x48a560
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern datum_index hs_thread_find_by_script_index(int16_t script_index); // 0x48a960

extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474
extern game_time_globals *game_time; // 0x006f1d6c

static hs_syntax_node *syntax_get(datum_index node)
{
    return (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node & 0xffff) * 0x14);
}

static hs_thread *thread_get(datum_index thread_index)
{
    return (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
}

void hs_evaluate_sleep_ticks(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread *thread = thread_get(thread_index);
    hs_stack_frame *frame;
    int32_t *ticks;
    int32_t *script;
    int16_t *state;
    datum_index ticks_node;
    datum_index target = thread_index;

    frame = thread->stack;
    ticks = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    script = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;
    frame = thread->stack;
    state = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    ticks_node = syntax_get(syntax_get(thread->stack->syntax_node)->data.first_child)->next_node;
    if (first != 0) {
        hs_thread_push(ticks_node, thread_index, ticks);
        *state = 0;
        return;
    }
    if (*state == 0) {
        datum_index script_node = syntax_get(ticks_node)->next_node;

        *state = 1;
        if (script_node != k_datum_index_none) {
            hs_thread_push(script_node, thread_index, script);
            return;
        }
        *script = k_datum_index_none;
    }

    if (*(int16_t *)ticks != 0) {
        int16_t count = *(int16_t *)ticks;
        int16_t script_index = *(int16_t *)script;

        if (script_index != -1) {
            target = hs_thread_find_by_script_index(script_index);
        }
        if (target != k_datum_index_none) {
            hs_thread *sleeper = thread_get(target);
            int32_t wake = count < 0 ? -2 : game_time->game_time + count;
            int32_t old_wake = sleeper->wake_tick;

            if (old_wake != -1) {
                if (target != thread_index && (sleeper->flags & _hs_thread_wake_saved_bit) == 0) {
                    sleeper->flags = sleeper->flags | _hs_thread_wake_saved_bit;
                    sleeper->saved_wake_tick = old_wake;
                }
                thread_get(target)->wake_tick = wake;
            }
        }
    }
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
