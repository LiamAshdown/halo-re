// hud_nameplate_candidate_filter  (not named by Ghidra: a code pointer pushed at 0x45e3e4)
// address 0x45e2e0, size 94 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-27 (static loop) from objdump 0x45e2e0..0x45e33d: the filter
// hud_find_nearby_teammate_for_nameplate hands to the PVS object collect (0x4fa1a0 -> object_tree_collect_matching).
// Accepts an object whose flags (+0x10) bit 0 is clear, whose type (+0xb4) is 0 (biped: (1 << type) & 1), whose
// +0x106 bit 2 is clear, and whose player is not the context player (*context = player handle).
// blam-cc: cdecl, stack -> object_index, context

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_game.h"

extern data_array *object_data; // 0x008603b0
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0

uint8_t hud_nameplate_candidate_filter(uint32_t object_index, void *context)
{
    datum_index player_handle = *(datum_index *)context;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;

    if ((obj[0x10] & 1) != 0) {
        return 0;
    }
    if (((1u << obj[0xb4]) & 1) == 0) {
        return 0;
    }
    if ((obj[0x106] & 4) != 0) {
        return 0;
    }
    return player_index_from_unit_index(object_index) != player_handle ? 1 : 0;
}
