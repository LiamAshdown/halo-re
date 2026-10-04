#include "halo/hs/records.hpp"
#include "halo/hs/hs1_runtime.hpp"
#include <stdio.h>
#include <string.h>
#include "halo/memory/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/platform/memory.hpp"


namespace halo::hs {

/**
 * Allocates a fresh "script node" data_array for the compiler/runtime. If a scenario is loaded and its
 * script_syntax_data tag block does not already carry exactly this table's byte size, the scenario's old
 * (GlobalAlloc'd) block is freed and the new table is hot-swapped into Scenario::script_syntax_data;
 * otherwise the allocation is left standalone and hs_syntax_data_is_local is set so hs_scripts_free knows to
 * GlobalFree it itself.
 *
 * @address 0x483100
 */
void ScriptRuntime::allocate_script_node_table(void)
{
    Scenario *scenario;

    scenario = (halo::scenario::globals().scenario_index != k_datum_index_none) ? halo::scenario::globals().scenario : 0;
    if ((scenario == 0) || (scenario->script_syntax_data.size != k_hs_syntax_node_table_size)) {
        halo::hs::globals().syntax_data = halo::memory::data_new(sizeof(hs_syntax_node), const_cast<char *>("script node"), k_hs_syntax_node_maximum_count);
        if (halo::hs::globals().syntax_data != 0) {
            halo::hs::globals().syntax_data->valid = 1;
            halo::memory::data_delete_all(halo::hs::globals().syntax_data);
            if (scenario != 0) {
                halo::platform::heap_free((void *)scenario->script_syntax_data.pointer);
                scenario->script_syntax_data.pointer = (uint32_t)halo::hs::globals().syntax_data;
                scenario->script_syntax_data.size = k_hs_syntax_node_table_size;
                return;
            }
            halo::hs::globals().syntax_data_is_local = 1;
        }
    }
}

/**
 * Finds the thread already running the script named `name` and restarts it, implementing a script "call" as
 * a plain restart of its existing thread. Returns 1 if a thread was found, 0 otherwise.
 *
 * @address 0x48a2d0
 */
char ScriptRuntime::call_script_by_name(char *name)
{
    datum_index thread_handle;

    thread_handle = halo::hs::hs_thread_find_by_script_name(name);
    if (thread_handle != k_datum_index_none) {
        halo::hs::hs_thread_restart(thread_handle);
        return 1;
    }
    return 0;
}

/**
 * Deletes every hs_globals_data slot from k_hs_builtin_global_count (0x1eb) up to hs_globals_data's current
 * last_index (the scenario-defined globals), then marks the runtime inactive. hs_thread_data is marked
 * invalid first (data_array::valid at +0x24).
 *
 * @address 0x48a130
 */
void ScriptRuntime::dispose_dynamic_globals(void)
{
    halo::hs::globals().thread_data->valid = 0;

    if (k_hs_builtin_global_count < halo::hs::globals().globals_data->last_index) {
        int16_t slot;
        for (slot = k_hs_builtin_global_count; slot < halo::hs::globals().globals_data->last_index; slot++) {
            if (slot != k_datum_index_none && -1 < slot && slot < halo::hs::globals().globals_data->maximum_count) {
                hs_global *element = halo::hs::global_slot(slot);
                if (element->identifier != 0 && (-1 < slot || element->identifier == (slot >> 0xf))) {
                    halo::memory::datum_delete(halo::hs::globals().globals_data, (datum_index)slot);
                }
            }
        }
    }

    halo::hs::globals().runtime_active = 0;
}

/**
 * Writes the signature and documentation string of every registered HS script function to hs_doc.txt.
 *
 * @address 0x484270
 */
void ScriptRuntime::doc(void)
{
    FILE *file;
    int16_t i;
    char buffer[2048];

    file = fopen("hs_doc.txt", "w");
    for (i = 0; i < k_hs_function_count; i = i + 1) {
        halo::hs::hs_format_function_signature(i, buffer);
        fprintf(file, "%s\r\n", buffer);
        strcpy(buffer, halo::hs::globals().function_definitions[i]->info);
        fprintf(file, "%s\r\n\r\n", buffer);
    }
    fclose(file);
}

}

namespace halo::hs {

void hs_allocate_script_node_table(void)
{
    halo::hs::ScriptRuntime::allocate_script_node_table();
}

char hs_call_script_by_name(char *name)
{
    return halo::hs::ScriptRuntime::call_script_by_name(name);
}

void hs_dispose_dynamic_globals(void)
{
    halo::hs::ScriptRuntime::dispose_dynamic_globals();
}

void hs_doc(void)
{
    halo::hs::ScriptRuntime::doc();
}

}
