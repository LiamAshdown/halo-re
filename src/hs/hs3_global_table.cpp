#include "halo/hs/hs3_machine.hpp"
#include "crt.h"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/main/api.hpp"
#include "halo/hs/api.hpp"

extern "C" {
extern char *hs_empty_string;
}

namespace halo::hs::part3 {

/**
 * Resolves a global-variable name to a packed hs_global_reference: a builtin index with k_hs_global_builtin_bit
 * set, a scenario-global index with it clear, or k_hs_global_reference_none if nothing matches.
 *
 * @address 0x483480
 */
hs_global_reference GlobalTable::find_global_by_name(char *name) const
{
    uint16_t index;
    ScenarioGlobal *globals;
    int32_t count;
    int32_t i;

    index = 0;
    do {
        if (_stricmp(name, halo::hs::globals().global_definitions[index]->name) == 0) {
            return (hs_global_reference)(index | k_hs_global_builtin_bit);
        }
        index = index + 1;
    } while ((int16_t)index < k_hs_builtin_global_count);

    if (halo::scenario::globals().scenario_index != k_datum_index_none) {
        count = (int32_t)halo::scenario::globals().scenario->globals.count;
        if (0 < count) {
            globals = (ScenarioGlobal *)halo::scenario::globals().scenario->globals.pointer;
            for (i = 0; i < count; i++) {
                if (_stricmp(name, globals[i].name.string) == 0) {
                    return (hs_global_reference)(i & k_hs_global_index_mask);
                }
            }
        }
    }
    return k_hs_global_reference_none;
}

/**
 * Returns the name of the global variable named by `global` (a builtin definition's name string, or a scenario
 * global's TagString name).
 *
 * @address 0x483450
 */
char *GlobalTable::get_name(hs_global_reference global) const
{
    hs_global_definition *definition;
    ScenarioGlobal *scenario_global;

    if ((global & k_hs_global_builtin_bit) != 0) {
        definition = halo::hs::globals().global_definitions[global & k_hs_global_index_mask];
        return definition->name;
    }
    scenario_global = (ScenarioGlobal *)halo::scenario::globals().scenario->globals.pointer + (global & k_hs_global_index_mask);
    return scenario_global->name.string;
}

/**
 * Returns the declared value type of the global variable named by `global` (a builtin definition's type, or a
 * scenario global's type).
 *
 * @address 0x483420
 */
hs_type_t GlobalTable::get_type(hs_global_reference global) const
{
    hs_global_definition *definition;
    ScenarioGlobal *scenario_global;

    if ((global & k_hs_global_builtin_bit) != 0) {
        definition = halo::hs::globals().global_definitions[global & k_hs_global_index_mask];
        return definition->type;
    }
    scenario_global = (ScenarioGlobal *)halo::scenario::globals().scenario->globals.pointer + (global & k_hs_global_index_mask);
    return scenario_global->type;
}

/**
 * Returns the current raw value of `reference` (after first syncing it from its bound engine variable, if it is
 * a builtin -- see hs_global_read_value).
 *
 * @address 0x48a720
 */
int32_t GlobalTable::get_value(hs_global_reference reference) const
{
    hs_global *slot;
    uint16_t index;

    halo::hs::hs_global_read_value(reference);
    index = reference & k_hs_global_index_mask;
    if ((reference & k_hs_global_builtin_bit) != 0) {
        slot = (hs_global *)((uint8_t *)halo::hs::globals().globals_data->data + index * 8);
    } else {
        slot = (hs_global *)((uint8_t *)halo::hs::globals().globals_data->data + (index + k_hs_builtin_global_count) * 8);
    }
    return slot->value.long_value;
}

/**
 * Copies the current value of the bound engine variable for builtin global `reference` into its hs_global
 * storage slot, dispatched by the builtin's declared type. Does nothing for a scenario-defined reference (bit 15
 * clear) or for a builtin whose definition has no bound address, beyond writing that type's documented default.
 *
 * @address 0x48aec0
 */
void GlobalTable::read_value(hs_global_reference reference) const
{
    hs_global_definition *definition;
    hs_global *slot;

    if ((reference & k_hs_global_builtin_bit) == 0) {
        return;
    }

    definition = halo::hs::globals().global_definitions[reference & k_hs_global_index_mask];
    slot = (hs_global *)((uint8_t *)halo::hs::globals().globals_data->data +
        (reference & k_hs_global_index_mask) * 8);

    switch (definition->type) {
    case _hs_type_boolean:
        slot->value.boolean_value = definition->address ? *(uint8_t *)definition->address : 0;
        break;
    case _hs_type_real:
        slot->value.long_value = definition->address ? *(int32_t *)definition->address : 0;
        break;
    case _hs_type_short:
        slot->value.short_value = definition->address ? *(int16_t *)definition->address : 0;
        break;
    case _hs_type_long:
        slot->value.long_value = definition->address ? *(int32_t *)definition->address : 0;
        break;
    case _hs_type_string:
        slot->value.string_value = definition->address ?
            *(char **)definition->address : hs_empty_string;
        break;
    case 10: case 11: case 12: case 13: case 14: case 15: case 16:
    case 18: case 19: case 20: case 21: case 22:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x2b:
        slot->value.short_value = definition->address ? *(int16_t *)definition->address : (int16_t)halo::k_word_none;
        break;
    case 17:
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c:
    case 0x1d: case 0x1e: case 0x1f:
    case 0x25: case 0x26: case 0x27: case 0x28: case 0x29: case 0x2a:
        slot->value.long_value = definition->address ? *(int32_t *)definition->address : -1;
        break;
    }
}

/**
 * Writes builtin global `reference`'s current hs_global storage value back into its bound engine variable,
 * dispatched by the builtin's declared type. Does nothing for a scenario-defined reference, or for a builtin
 * whose definition has no bound address.
 *
 * @address 0x48b030
 */
void GlobalTable::write_value(hs_global_reference reference) const
{
    hs_global_definition *definition;
    hs_global *slot;

    if ((reference & k_hs_global_builtin_bit) == 0) {
        return;
    }

    definition = halo::hs::globals().global_definitions[reference & k_hs_global_index_mask];
    slot = (hs_global *)((uint8_t *)halo::hs::globals().globals_data->data +
        (reference & k_hs_global_index_mask) * 8);

    switch (definition->type) {
    case _hs_type_boolean:
        if (definition->address) {
            *(uint8_t *)definition->address = slot->value.boolean_value;
        }
        break;
    case _hs_type_real: case _hs_type_long:
    case 0x18: case 0x1a: case 0x1c: case 0x1e: case 0x26: case 0x28: case 0x2a:
        if (definition->address) {
            *(int32_t *)definition->address = slot->value.long_value;
        }
        break;
    case _hs_type_short:
    case 11: case 13: case 15: case 19: case 21:
    case 0x21: case 0x23: case 0x2b:
        if (definition->address) {
            *(int16_t *)definition->address = slot->value.short_value;
        }
        break;
    case _hs_type_string:
    case 17:
    case 0x17: case 0x19: case 0x1b: case 0x1d: case 0x1f:
    case 0x25: case 0x27: case 0x29:
        if (definition->address) {
            *(int32_t *)definition->address = slot->value.long_value;
        }
        break;
    case 10: case 12: case 14: case 16: case 18: case 20: case 22:
    case 0x20: case 0x22: case 0x24:
        if (definition->address) {
            *(int16_t *)definition->address = slot->value.short_value;
        }
        break;
    }
}

/**
 * Looks up `name` as a "snd!" (sound) tag first, returning a pointer to its gain modifier (tag data + 0x28) if
 * found. Otherwise looks it up as an "lsnd" (sound_looping) tag; if found and its permutation-like count (+0x3c)
 * is positive, returns a pointer into its data (+0x40 + 4, i.e. the second element of an array there). If
 * neither tag exists, reports a compiler error and returns NULL.
 *
 * @address 0x488b10
 */
float *GlobalTable::sound_get_gain_reference(char *name) const
{
    datum_index tag_id;
    uint8_t *sound_data;
    uint8_t *looping_data;

    tag_id = halo::cache::tag_lookup(0x736e6421, name);
    if (tag_id != k_datum_index_none) {
        sound_data = (uint8_t *)halo::cache::globals().tag_instances[(tag_id & halo::k_slot_mask) & halo::k_slot_mask].data;
        return (float *)(sound_data + 0x28);
    }

    tag_id = halo::cache::tag_lookup(0x6c736e64, name);
    if (tag_id != k_datum_index_none) {
        looping_data = (uint8_t *)halo::cache::globals().tag_instances[(tag_id & halo::k_slot_mask) & halo::k_slot_mask].data;
        if (0 < *(int32_t *)(looping_data + 0x3c)) {
            return (float *)(*(uint32_t *)(looping_data + 0x40) + 4);
        }
    }

    halo::main::console_print_error_va(0, "the sound '%s' does not exist");
    return 0;
}

}
