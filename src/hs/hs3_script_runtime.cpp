#include "halo/hs/records.hpp"
#include "halo/ai/records.hpp"
#include "halo/hs/hs3_machine.hpp"
#include "halo/scenario/api.hpp"
#include "game.h"
#include "win32.h"
#include <string.h>
#include "halo/memory/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern data_array *player_data;
extern data_array *object_data;
extern game_time_globals *game_time;
extern byte_swap_definition hs_syntax_data_header_byte_swap_definition;
extern byte_swap_definition hs_syntax_node_byte_swap_definition;
}

static const uint8_t k_swapped_script_node_name[12] = { 0x63, 0x73, 0x69, 0x72, 0x74, 0x70, 0x6e, 0x20, 0x00, 0x65, 0x64, 0x6f };

namespace halo::hs::part3 {

/**
 * Behaviour of the original `hs_reposition_players_outside_trigger_volume` function, moved unchanged into the
 * class.
 *
 * @address 0x487750
 */
void ScriptRuntime::reposition_players_outside_trigger_volume(int32_t trigger_volume_index, int32_t location_index) const
{
    datum_index player_index = halo::memory::datum_next(-1, halo::game::globals().player_data);

    while (player_index != k_datum_index_none) {
        datum_index unit = *(datum_index *)((uint8_t *)halo::game::globals().player_data->data + (player_index & halo::k_slot_mask) * 0x200 + 0x34);

        if (unit != k_datum_index_none) {
            uint8_t *object = reinterpret_cast<uint8_t *>(halo::ai::object_at(unit));

            if (!halo::scenario::scenario_query::trigger_volume_contains_point((int16_t)trigger_volume_index, (real_point3d *)(object + 0xa0))) {
                halo::hs::hs_object_detach_and_place_at_location((int16_t)location_index, unit, 1, 1);
            }
        }
        player_index = halo::memory::datum_next((int16_t)player_index, halo::game::globals().player_data);
    }
}

/**
 * Allocates the hs_thread and hs_globals datum arrays. On success, marks hs_globals_data valid
 * (data_array::valid at +0x24), resets it, and reserves its first k_hs_builtin_global_count (0x1eb) slots for
 * the engine builtins, each with a distinct salt (see UNSURE above).
 *
 * @address 0x489e70
 */
void ScriptRuntime::runtime_initialize() const
{
    int32_t i;

    halo::hs::globals().thread_data = halo::saved_games::game_state_new((char *)"hs thread", k_hs_thread_maximum_count, 0x218 );
    halo::hs::globals().globals_data = halo::saved_games::game_state_new((char *)"hs globals", k_hs_global_maximum_count, 0x8 );
    if (halo::hs::globals().thread_data != 0 && halo::hs::globals().globals_data != 0) {
        halo::hs::globals().globals_data->valid = 1;
        halo::memory::data_delete_all(halo::hs::globals().globals_data);
        for (i = 0; i < k_hs_builtin_global_count; i++) {
            halo::memory::datum_new_at_index_with_salt((datum_index)i | 0x10000, halo::hs::globals().globals_data);
        }
    }
}

/**
 * Runs one scheduler tick: steps every thread whose wake_tick has arrived (0 <= wake_tick <= current tick),
 * noting whether any command thread (type 2) is live. Always disposes empty object lists afterward; if no
 * command thread was seen, runs a syntax-node garbage collection pass every 16 ticks.
 *
 * @address 0x48a1a0
 */
void ScriptRuntime::runtime_update() const
{
    int32_t current_tick;
    datum_index thread_handle;
    hs_thread *thread;
    char command_thread_pending;

    if (halo::hs::globals().runtime_active == 0) {
        return;
    }

    current_tick = halo::game::globals().game_time->game_time;
    command_thread_pending = 0;
    thread_handle = halo::memory::datum_next(-1, halo::hs::globals().thread_data);
    while (thread_handle != k_datum_index_none) {
        thread = halo::hs::thread_at(thread_handle);
        if (thread->type == 2) {
            command_thread_pending = 1;
        }
        if (-1 < thread->wake_tick && thread->wake_tick <= current_tick) {
            halo::hs::hs_thread_evaluate_step(thread_handle);
        }
        thread_handle = halo::memory::datum_next((int16_t)thread_handle, halo::hs::globals().thread_data);
        if (halo::hs::globals().runtime_active == 0) {
            break;
        }
    }

    halo::hs::object_lists_dispose_empty();
    if (command_thread_pending == 0 && halo::game::globals().game_time->game_time % 16 == 0) {
        halo::hs::hs_syntax_node_garbage_collect();
    }
}

/**
 * Resets the HS thread table and marks the runtime active, then (if a scenario is loaded) evaluates every
 * scenario global's default-value expression via a transient type-1 thread (bumping the referenced object_list's
 * holder count when a global's type is object_list), and finally auto-starts (hs_thread_new) every script whose
 * type is not static (3) or stub (4).
 *
 * @address 0x489ef0
 */
void ScriptRuntime::scenario_scripts_initialize() const
{
    hs_thread *init_thread;
    hs_stack_frame *frame;
    datum_index thread_handle;
    ScenarioGlobal *globals;
    ScenarioScript *scripts;
    int32_t i;
    int16_t slot;
    hs_global_reference reference;
    hs_global *global_slot;
    int32_t list_handle;
    object_list_header *list_header;

    halo::hs::globals().thread_data->valid = 1;
    halo::memory::data_delete_all(halo::hs::globals().thread_data);
    halo::hs::globals().runtime_active = 1;
    halo::hs::globals().current_thread_index = -1;

    thread_handle = halo::memory::datum_new(halo::hs::globals().thread_data);
    init_thread = 0;
    if (thread_handle != k_datum_index_none) {
        init_thread = halo::hs::thread_at(thread_handle);
        init_thread->stack = (hs_stack_frame *)&init_thread->stack_data;
        init_thread->stack->previous = 0;
        init_thread->stack->size = 0;
        init_thread->stack->syntax_node = k_datum_index_none;
        init_thread->type = 1;
        init_thread->script_index = -1;
        init_thread->flags = 0;
        init_thread->wake_tick = 0;
    }

    if (halo::scenario::globals().scenario_index == k_datum_index_none) {
        return;
    }

    if (init_thread != 0) {
        globals = (ScenarioGlobal *)halo::scenario::globals().scenario->globals.pointer;
        for (i = 0; i < (int32_t)halo::scenario::globals().scenario->globals.count; i++) {

            reference = (hs_global_reference)i;
            slot = (int16_t)(reference & k_hs_global_index_mask) +
                   ((reference & k_hs_global_builtin_bit) ? 0 : k_hs_builtin_global_count);

            if (-1 < slot && slot < halo::hs::globals().globals_data->maximum_count) {
                global_slot = (hs_global *)((uint8_t *)halo::hs::globals().globals_data->data +
                    halo::hs::globals().globals_data->size * slot);
                if (global_slot->identifier == 0) {
                    halo::hs::globals().globals_data->actual_count = halo::hs::globals().globals_data->actual_count + 1;
                    if (halo::hs::globals().globals_data->last_index <= slot) {
                        halo::hs::globals().globals_data->last_index = slot + 1;
                    }
                    halo::memory::datum_element_initialize(halo::hs::globals().globals_data, global_slot);
                    global_slot->identifier = (int16_t)0xaced;
                }
            }

            init_thread->script_index = -1;
            init_thread->stack->size = 0;

            halo::hs::hs_thread_push(globals[i].initialization_expression_index, thread_handle,
                &((hs_global *)halo::hs::globals().globals_data->data)[slot & halo::k_slot_mask].value);
            if ((init_thread->flags & 1) != 0) {
                halo::hs::hs_thread_evaluate_step(thread_handle);
                if (globals[i].type == 0x17) {
                    list_handle = halo::hs::hs_global_get_value(reference);
                    if (list_handle != -1) {
                        list_header = halo::hs::object_list_header_at(list_handle);
                        list_header->reference_count = list_header->reference_count + 1;
                    }
                }
            }
            halo::hs::hs_global_write_value(reference);
        }
        halo::memory::datum_delete(halo::hs::globals().thread_data, thread_handle);
    }

    scripts = (ScenarioScript *)halo::scenario::globals().scenario->scripts.pointer;
    for (i = 0; i < (int32_t)halo::scenario::globals().scenario->scripts.count; i++) {
        if (scripts[i].script_type != 3 && scripts[i].script_type != 4) {
            halo::hs::hs_thread_new(i, 0);
        }
    }
}

/**
 * (Re)compiles and links every script in the loaded scenario into a fresh syntax-node table backed by the
 * scenario's own script_syntax_data storage. If restore_previous is set, the caller's previous hs_syntax_data is
 * restored before returning either way.
 *
 * @address 0x483190
 */
char ScriptRuntime::scripts_compile_and_link(char restore_previous) const
{
    data_array *saved_syntax_data;
    Scenario *scenario;
    char no_scripts_but_source_files;
    char success;
    char *error_message;
    int32_t error_offset;

    saved_syntax_data = halo::hs::globals().syntax_data;
    scenario = halo::scenario::globals().scenario;
    success = 1;
    halo::hs::hs_allocate_script_node_table();
    no_scripts_but_source_files = (scenario->scripts.count == 0) && (0 < scenario->source_files.count);
    halo::hs::globals().syntax_data = (data_array *)scenario->script_syntax_data.pointer;
    halo::hs::globals().syntax_data->data = (uint8_t *)halo::hs::globals().syntax_data + 0x38;
    if (no_scripts_but_source_files || (halo::hs::hs_compile_postprocess(&error_message, &error_offset) == 0)) {
        success = halo::hs::hs_compile_source();
        if ((success != 0) && (halo::hs::hs_compile_postprocess(&error_message, &error_offset) != 0)) {
            success = 1;
            goto restore;
        }
        halo::memory::data_delete_all(halo::hs::globals().syntax_data);
    } else if (0x3ff < (int32_t)scenario->script_string_data.size) {
        goto restore;
    }
    success = 0;
restore:
    if (restore_previous != 0) {
        halo::hs::globals().syntax_data = saved_syntax_data;
    }
    return success;
}

/**
 * Tears down the active HS syntax-node table (freeing it only if hs owns the allocation rather than the scenario
 * tag) and marks both object-list containers unused.
 *
 * @address 0x4832b0
 */
void ScriptRuntime::scripts_free() const
{
    data_array *nodes;

    nodes = halo::hs::globals().syntax_data;
    if (halo::hs::globals().syntax_data != 0) {
        halo::hs::hs_syntax_node_garbage_collect();
        if (halo::hs::globals().syntax_data_is_local != 0) {
            nodes->valid = 0;
            memset(nodes, 0, sizeof(*nodes));
            GlobalFree(nodes);
            halo::hs::globals().syntax_data_is_local = 0;
        }
        halo::hs::globals().syntax_data = 0;
    }
    halo::hs::hs_dispose_dynamic_globals();
    halo::objects::globals().object_list_header_data->valid = 0;
    halo::objects::globals().object_list_reference_data->valid = 0;
}

/**
 * Rebuilds the syntax-node table, relinks the scenario's scripts if any are already compiled, resets both
 * object-list containers, and re-initializes the HS runtime for the scenario.
 *
 * @address 0x483250
 */
void ScriptRuntime::scripts_reload() const
{
    Scenario *scenario;

    scenario = (halo::scenario::globals().scenario_index != k_datum_index_none) ? halo::scenario::globals().scenario : 0;
    halo::hs::hs_allocate_script_node_table();
    if ((scenario != 0) && (scenario->script_syntax_data.size != 0)) {
        halo::hs::hs_scripts_compile_and_link(0);
    }
    halo::objects::globals().object_list_header_data->valid = 1;
    halo::memory::data_delete_all(halo::objects::globals().object_list_header_data);
    halo::objects::globals().object_list_reference_data->valid = 1;
    halo::memory::data_delete_all(halo::objects::globals().object_list_reference_data);
    halo::hs::hs_scenario_scripts_initialize();
}

/**
 * Behaviour of the original `hs_syntax_data_byte_swap` function, moved unchanged into the class.
 *
 * @address 0x48b150
 */
void ScriptRuntime::syntax_data_byte_swap(void *element, uint8_t *data, uint32_t size) const
{
    byte_swap_definition *header = &hs_syntax_data_header_byte_swap_definition;
    byte_swap_definition *node = &hs_syntax_node_byte_swap_definition;
    uint32_t count;
    uint32_t i;

    (void)element;
    if (size == 0) {
        return;
    }
    if (memcmp(data, k_swapped_script_node_name, sizeof(k_swapped_script_node_name)) == 0) {
        if (data == 0) {
            return;
        }
        halo::memory::struct_definition_byte_swap(header, (int32_t)data, header->codes, 0, 0);
        for (i = 0; i < 0x4000; i++) {
            halo::memory::struct_definition_byte_swap(node, (int32_t)(data + node->size * i), node->codes, 0, 0);
        }
        return;
    }
    if ((int32_t)(size - 0x38) < 0 || (size - 0x38) % 0x14 != 0) {
        return;
    }
    count = (size - 0x38) / 0x14;
    if (data != 0) {
        halo::memory::struct_definition_byte_swap(header, (int32_t)data, header->codes, 0, 0);
    }
    if (data + 0x38 != 0) {
        for (i = 0; i < count; i++) {
            halo::memory::struct_definition_byte_swap(node, (int32_t)(data + 0x38 + node->size * i), node->codes, 0, 0);
        }
    }
}

/**
 * Deletes every hs_syntax_node not marked collectable (flags bit 3 clear) out of hs_syntax_data.
 *
 * @address 0x483310
 */
void ScriptRuntime::syntax_node_garbage_collect() const
{
    data_array *nodes;
    datum_index current;
    hs_syntax_node *node;
    int32_t next_start;
    int16_t next_index;
    hs_syntax_node *scan;

    nodes = halo::hs::globals().syntax_data;
    current = halo::memory::datum_next(-1, nodes);
    for (;;) {
        for (;;) {
            if (current == k_datum_index_none) {
                return;
            }
            node = (hs_syntax_node *)((uint8_t *)nodes->data + (current & halo::k_slot_mask) * nodes->size);
            if ((node->flags & _hs_syntax_node_garbage_collectable_bit) == 0) {
                halo::memory::datum_delete(nodes, current);
            }
            next_start = (int32_t)(current & halo::k_slot_mask) + 1;
            current = k_datum_index_none;
            next_index = (int16_t)next_start;
            if (0 <= next_index && next_index < nodes->last_index) {
                break;
            }
        }
        scan = (hs_syntax_node *)((uint8_t *)nodes->data + (int32_t)next_index * nodes->size);
        for (;;) {
            if (scan->identifier != 0) {
                current = ((uint32_t)(uint16_t)scan->identifier << 16) | (uint16_t)next_index;
                break;
            }
            next_start = next_start + 1;
            scan = (hs_syntax_node *)((uint8_t *)scan + nodes->size);
            next_index = (int16_t)next_start;
            if (nodes->last_index <= next_index) {
                break;
            }
        }
    }
}

}
