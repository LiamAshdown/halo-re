#include "halo/hs/hs3_machine.hpp"
#include "game.h"
#include "crt.h"
#include "halo/memory/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern game_time_globals *game_time;
extern int32_t (*hs_type_conversion_procedures[k_hs_type_count][k_hs_type_count])(int32_t value);
}

namespace halo::hs::part3 {

/**
 * Steps `thread_index` through as many syntax nodes as its per-tick time budget allows (see the game_time check
 * below), dispatching each node's function evaluate handler (or, for a script-call node, pushing/finalizing the
 * called script's own thread) until either its evaluation stack empties (finished) or it must pause and resume
 * later. On finishing: a startup/dormant script thread is parked forever (wake_tick -1); a command thread (type
 * 2) is deleted outright; any other thread (including a continuous/static script) is simply left as is for the
 * next tick.
 *
 * @address 0x48a370
 */
void ThreadMachine::evaluate_step(uint32_t thread_index) const
{
    hs_thread *thread;
    ScenarioScript *script;
    ScenarioScript *called_script;
    uint8_t *scratch;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    uint8_t saved_flags;
    char first;
    hs_function_definition *definition;

    thread = (hs_thread *)((uint8_t *)halo::hs::globals().thread_data->data + (thread_index & halo::k_slot_mask) * sizeof(hs_thread));
    script = 0;
    halo::hs::globals().current_thread_index = (int16_t)thread_index;

    if (thread->type == _hs_thread_script) {
        script = &((ScenarioScript *)halo::scenario::globals().scenario->scripts.pointer)[thread->script_index];
    }

    thread->wake_tick = 0;
    if ((void *)thread->stack == (void *)&thread->stack_data) {

        frame = thread->stack;
        frame->size = 0;
        scratch = (uint8_t *)frame + 0x0e + frame->size;
        frame->size = frame->size + 4;
        halo::hs::hs_thread_push(script->root_expression_index, thread_index, scratch);
    }

    while ((void *)thread->stack != (void *)&thread->stack_data) {
        if (thread->wake_tick < 0 ||
            (halo::game::globals().game_time->initialized != 0 &&
             (halo::game::globals().game_time->active != 0 || halo::game::globals().game_time->paused != 0) &&
             halo::game::globals().game_time->game_time < thread->wake_tick) ||
            halo::hs::globals().runtime_active == 0) {
            break;
        }

        frame = thread->stack;
        node = (hs_syntax_node *)((uint8_t *)halo::hs::globals().syntax_data->data + (frame->syntax_node & halo::k_slot_mask) * 0x14);
        saved_flags = thread->flags;
        frame->size = 0;
        thread->flags = thread->flags & 0xfe;
        first = saved_flags & 1;

        if ((node->flags & _hs_syntax_node_script_call_bit) == 0) {
            definition = halo::hs::globals().function_definitions[node->index_union];
            ((void (*)(int16_t, uint32_t, char))definition->evaluate)(node->index_union,
                thread_index, first);
        } else {

            called_script = &((ScenarioScript *)halo::scenario::globals().scenario->scripts.pointer)[node->index_union];
            frame = thread->stack;
            scratch = (uint8_t *)frame + 0x0e + frame->size;
            frame->size = frame->size + 4;
            if (first != 0) {
                halo::hs::hs_thread_push(called_script->root_expression_index, thread_index, scratch);
            } else {
                halo::hs::hs_thread_return(*(int32_t *)scratch, thread_index);
            }
        }
    }

    if ((void *)thread->stack == (void *)&thread->stack_data) {
        if (thread->type == _hs_thread_script) {
            if (script->script_type == _hs_script_startup ||
                script->script_type == _hs_script_dormant) {

                thread->wake_tick = -1;
                halo::hs::globals().current_thread_index = -1;
                return;
            }
        } else if (thread->type == _hs_thread_command) {
            halo::memory::datum_delete(halo::hs::globals().thread_data, thread_index);
        }
    }
}

/**
 * Finds the thread whose script_index equals `script_index`, or k_datum_index_none if none.
 *
 * @address 0x48a960
 */
datum_index ThreadMachine::find_by_script_index(int16_t script_index) const
{
    datum_index thread_handle;
    hs_thread *thread;

    thread_handle = halo::memory::datum_next(-1, halo::hs::globals().thread_data);
    while (thread_handle != k_datum_index_none) {
        thread = (hs_thread *)((uint8_t *)halo::hs::globals().thread_data->data + (thread_handle & halo::k_slot_mask) * sizeof(hs_thread));
        if (thread->script_index == script_index) {
            return thread_handle;
        }
        thread_handle = halo::memory::datum_next((int16_t)thread_handle, halo::hs::globals().thread_data);
    }
    return k_datum_index_none;
}

/**
 * Finds the (active or dormant) thread whose associated script's name matches `name` case-insensitively, or
 * k_datum_index_none if none does.
 *
 * @address 0x48a9f0
 */
datum_index ThreadMachine::find_by_script_name(char *name) const
{
    datum_index thread_handle;
    hs_thread *thread;
    ScenarioScript *scripts;

    scripts = (ScenarioScript *)halo::scenario::globals().scenario->scripts.pointer;
    thread_handle = halo::memory::datum_next(-1, halo::hs::globals().thread_data);
    while (thread_handle != k_datum_index_none) {
        thread = (hs_thread *)((uint8_t *)halo::hs::globals().thread_data->data + (thread_handle & halo::k_slot_mask) * sizeof(hs_thread));
        if (thread->script_index != -1 &&
            _stricmp(scripts[thread->script_index].name.string, name) == 0) {
            return thread_handle;
        }
        thread_handle = halo::memory::datum_next((int16_t)thread_handle, halo::hs::globals().thread_data);
    }
    return k_datum_index_none;
}

/**
 * Allocates a new hs_thread of the given `type`, associated with `script_index` (or -1 for none). A thread for a
 * dormant (hs_script_type 1) script starts parked at wake_tick -2; every other thread starts runnable at
 * wake_tick 0. Returns the new thread's handle, or k_datum_index_none if the thread table is full.
 *
 * @address 0x48a2f0
 */
datum_index ThreadMachine::create(int32_t script_index, uint8_t type) const
{
    datum_index handle;
    hs_thread *thread;
    ScenarioScript *scripts;

    handle = halo::memory::datum_new(halo::hs::globals().thread_data);
    if (handle != k_datum_index_none) {
        thread = (hs_thread *)((uint8_t *)halo::hs::globals().thread_data->data + (handle & halo::k_slot_mask) * sizeof(hs_thread));
        thread->stack = (hs_stack_frame *)&thread->stack_data;
        thread->stack->previous = 0;
        thread->stack->size = 0;
        thread->stack->syntax_node = k_datum_index_none;
        thread->type = type;
        thread->script_index = script_index;
        thread->flags = 0;

        scripts = (ScenarioScript *)halo::scenario::globals().scenario->scripts.pointer;
        if (script_index != -1 && scripts[script_index].script_type == _hs_script_dormant) {
            thread->wake_tick = -2;
            return handle;
        }
        thread->wake_tick = 0;
    }
    return handle;
}

/**
 * Pops the current frame off `thread_index`'s evaluation stack, returning to its parent.
 *
 * @address 0x48a770
 */
void ThreadMachine::pop_frame(uint32_t thread_index) const
{
    hs_thread *thread;

    thread = (hs_thread *)((uint8_t *)halo::hs::globals().thread_data->data + (thread_index & halo::k_slot_mask) * sizeof(hs_thread));
    thread->stack = thread->stack->previous;
}

/**
 * Evaluates `node` as the next step for `thread_index`: if it is a non-primitive expression, pushes a new stack
 * frame to evaluate its children (recording `result_address` in the outgoing frame first, per the docs above).
 * If it is a primitive, resolves its value immediately (via the global-variable path when the node has the
 * global bit set, otherwise directly) and stores it straight into `*result_address` without pushing a frame.
 *
 * @address 0x48a560
 */
void ThreadMachine::push(datum_index node, uint32_t thread_index, void *result_address) const
{
    hs_syntax_node *syntax_node;
    hs_thread *thread;
    hs_stack_frame *frame;
    hs_stack_frame *new_frame;
    int32_t value;
    hs_global_reference reference;
    uint16_t index;
    hs_type_t source_type;

    syntax_node = (hs_syntax_node *)((uint8_t *)halo::hs::globals().syntax_data->data + (node & halo::k_slot_mask) * 0x14);
    thread = (hs_thread *)((uint8_t *)halo::hs::globals().thread_data->data + (thread_index & halo::k_slot_mask) * sizeof(hs_thread));

    if ((syntax_node->flags & _hs_syntax_node_primitive_bit) == 0) {
        thread->stack->result_address = result_address;
        frame = thread->stack;
        new_frame = (hs_stack_frame *)((uint8_t *)frame + 0x10 + frame->size);
        new_frame->previous = frame;
        thread->stack = new_frame;
        new_frame->size = 0;
        thread->flags = thread->flags | 1;
        new_frame->syntax_node = node;
        return;
    }

    if ((syntax_node->flags & _hs_syntax_node_global_bit) != 0) {
        reference = (hs_global_reference)syntax_node->data.global_reference;
        index = reference & k_hs_global_index_mask;
        if ((reference & k_hs_global_builtin_bit) != 0) {
            source_type = halo::hs::globals().global_definitions[index]->type;
        } else {
            source_type = ((ScenarioGlobal *)halo::scenario::globals().scenario->globals.pointer)[index].type;
        }
        value = halo::hs::hs_global_get_value(reference);
        value = halo::hs::hs_coerce_value(value, syntax_node->type, source_type);
        *(int32_t *)result_address = value;
        return;
    }
    value = halo::hs::hs_coerce_value(syntax_node->data.long_value, syntax_node->type,
                            (hs_type_t)syntax_node->index_union);
    *(int32_t *)result_address = value;
}

/**
 * Restarts or resumes an existing (possibly dormant) HS thread. If its wake/saved-wake flag is set, restores
 * wake_tick from saved_wake_tick and clears the flag. Otherwise, if the thread's current (or parent) frame is
 * evaluating a call to sleep_until, pops one or two frames off its stack so the next evaluate step re-enters the
 * sleep_until call correctly.
 *
 * @address 0x48a790
 */
void ThreadMachine::restart(uint32_t thread_index) const
{
    hs_thread *thread;
    hs_syntax_node *node;
    hs_stack_frame *parent_frame;
    datum_index syntax_node;

    thread = (hs_thread *)((uint8_t *)halo::hs::globals().thread_data->data + (thread_index & halo::k_slot_mask) * sizeof(hs_thread));
    if (thread->wake_tick == -1) {
        return;
    }

    thread->wake_tick = 0;
    if ((thread->flags & 2) != 0) {
        thread->wake_tick = thread->saved_wake_tick;
        thread->flags = thread->flags & 0xfd;
        return;
    }

    syntax_node = thread->stack->syntax_node;
    if (syntax_node != k_datum_index_none) {
        node = (hs_syntax_node *)((uint8_t *)halo::hs::globals().syntax_data->data + (syntax_node & halo::k_slot_mask) * 0x14);
        if (node->index_union == _hs_function_sleep_until) {
            thread->stack = thread->stack->previous;
            return;
        }
    }

    parent_frame = thread->stack->previous;
    if (parent_frame != 0) {
        syntax_node = parent_frame->syntax_node;
        if (syntax_node != k_datum_index_none) {
            node = (hs_syntax_node *)((uint8_t *)halo::hs::globals().syntax_data->data +
                (syntax_node & halo::k_slot_mask) * 0x14);
            if (node->index_union == _hs_function_sleep_until) {
                halo::hs::hs_thread_pop_frame(thread_index);
                halo::hs::hs_thread_pop_frame(thread_index);
                thread->flags = thread->flags & 0xfe;
            }
        }
    }
}

/**
 * Finishes evaluating the current frame's syntax node with the given `value`: converts it from the node's actual
 * return type (a script's declared return type, or a function definition's) to the type its parent context
 * expects (unless they already match, the source is "passthrough", or the expected type is in the object-name
 * family 0x2b..0x30, which this function does not convert into), writes the (possibly converted) value through
 * the current frame's result_address, and pops the frame.
 *
 * @address 0x48a640
 */
void ThreadMachine::return_value(int32_t value, uint32_t thread_index) const
{
    hs_thread *thread;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    hs_type_t actual_type;
    hs_type_t expected_type;
    ScenarioScript *scripts;

    thread = (hs_thread *)((uint8_t *)halo::hs::globals().thread_data->data + (thread_index & halo::k_slot_mask) * sizeof(hs_thread));
    frame = thread->stack;
    node = (hs_syntax_node *)((uint8_t *)halo::hs::globals().syntax_data->data + (frame->syntax_node & halo::k_slot_mask) * 0x14);

    if ((node->flags & _hs_syntax_node_script_call_bit) == 0) {
        actual_type = halo::hs::globals().function_definitions[node->index_union]->return_type;
    } else {
        scripts = (ScenarioScript *)halo::scenario::globals().scenario->scripts.pointer;
        actual_type = scripts[node->index_union].return_type;
    }

    expected_type = node->type;
    if (actual_type != expected_type && actual_type != _hs_type_passthrough &&
        (expected_type < 0x2b || 0x30 < expected_type)) {
        if (expected_type < 0x25 || 0x2a < expected_type) {

            value = hs_type_conversion_procedures[expected_type][actual_type](value);
        } else if (0x2a < actual_type && actual_type < 0x31) {
            value = halo::objects::object_lookup_table_get(value);
        }
    }

    *(int32_t *)frame->previous->result_address = value;
    thread->stack = frame->previous;
}

}
