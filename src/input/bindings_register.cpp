/**
 * Control binding table, last-used-binding cache, bind/unbind commands, rebind capture and device default profiles.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include "halo/input/bindings.hpp"
#include "halo/input/api.hpp"
#include "halo/input/state.hpp"

namespace halo::input {

/**
 * Implements control binding table register single.
 *
 * Original register convention: EDX -> target, EAX -> selector, EDI -> raw_id, EBX -> raw_value.
 *
 * @address 0x4f37d0
 */
void Bindings::control_binding_table_register_single(int32_t target, int32_t selector, int32_t raw_id, uint32_t raw_value)
{
    uint8_t *cursor = input_state().g_control_binding_region_ec;
    uint8_t *region_end = input_state().g_control_binding_region_ec + sizeof(input_state().g_control_binding_region_ec);
    int32_t row = 0;

    while (*(int32_t *)cursor != target) {
        cursor += 0xa0;
        row++;
        if (cursor >= region_end) {
            return;
        }
    }

    {
        uint32_t do_register;
        switch (input_state().control_binding_device_type) {
        case 1: do_register = (raw_value >> 9) & 1; break;
        case 2: do_register = (raw_value >> 8) & 1; break;
        case 3: do_register = (raw_value >> 11) & 1; break;
        case 4: do_register = (raw_value >> 10) & 1; break;
        default: do_register = 1; break;
        }
        if (!do_register) return;
    }

    if (row >= 0 && row < 6 && raw_id != -1) {
        int32_t idx;
        int32_t *count_cell;
        int32_t slot;

        if (selector < 0 || selector > 1) selector = 0;
        idx = selector + row * 2;
        count_cell = (int32_t *)(input_state().g_control_binding_region_e0 + idx * 0x50);
        slot = *count_cell + idx * 10;
        *count_cell = *count_cell + 1;
        *(int32_t *)((uint8_t *)input_state().g_control_binding_id + slot * 8) = raw_id;
        *(int16_t *)((uint8_t *)input_state().g_control_binding_value + slot * 8) = (int16_t)raw_value;
    }
}

}
