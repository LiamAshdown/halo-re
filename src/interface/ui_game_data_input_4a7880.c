// ui_game_data_input_4a7880  (not a Ghidra function; game_data_input_function_table[50])
// address 0x4a7880, size 634 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692be0 (index 50); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a7880.
// WRITTEN 2026-09-28 from objdump 0x4a7880..0x4a7af9: network game options screen, per frame: the first child's
//   spinner list 1..6 sets 0x00719208 to 0x384, 0x708, 0xa8c, 0xe10, 0x1518, 0x2328 (else 0). The second child's list
//   picks which packed option word is edited (0x00692b0c; 1 -> 0x00879f38, else 0x00879f34); when that changes, the
//   third child's list shows the word's low nibble (below 9, else 0). When the third list's selection differs from
//   0x00692b08 (or the word changed), it becomes the word's low nibble and 0x00692b08, and the bind rows are
//   repopulated from the word. With a low nibble of 8, the lists of the fourth to ninth children fill 3-bit fields at
//   bits 4, 7, 10, 13, 16 and 19. Tail-calls the extended description selection sync.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint32_t unknown_00719208; // 0x00719208, UNSURE
extern uint8_t variant_teams_enabled_0071920c; // 0x0071920c, UNSURE name (variant byte +0x34 != 0)
extern int32_t variant_team_selection_00692b08; // 0x00692b08, UNSURE name
extern int32_t unknown_00692b0c; // 0x00692b0c, UNSURE
extern uint32_t unknown_00879f34; // 0x00879f34, UNSURE
extern uint32_t unknown_00879f38; // 0x00879f38, UNSURE
extern void ui_controls_populate_bind_rows(widget_instance *widget, uint32_t packed); // 0x4a3180
extern void widget_extended_description_sync_selection(widget_instance *widget); // 0x4a66b0

static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

void ui_game_data_input_4a7880(widget_instance *widget)
{
    static const uint32_t delays[] = {0, 0x384, 0x708, 0xa8c, 0xe10, 0x1518, 0x2328};
    widget_instance *group = widget->first_child;
    int16_t selection = first_list_child(group)->selection_index;
    uint8_t changed = 0;
    uint32_t *packed;
    int32_t which;

    unknown_00719208 = selection >= 0 && selection <= 6 ? delays[selection] : 0;
    group = group->next_sibling;
    which = unknown_00692b0c;
    if (first_list_child(group)->selection_index != which) {
        which = first_list_child(group)->selection_index;
        unknown_00692b0c = which;
        changed = 1;
    }
    packed = which == 1 ? &unknown_00879f38 : &unknown_00879f34;
    if (changed) {
        uint32_t value = *packed;

        first_list_child(group->next_sibling)->selection_index = (int16_t)((value & 0xf) < 9 ? (value & 0xf) : 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection != variant_team_selection_00692b08 || changed) {
        *packed = (*packed & ~0xfu) | ((uint32_t)selection & 0xf);
        variant_team_selection_00692b08 = selection;
        ui_controls_populate_bind_rows(widget, *packed);
    }
    if ((*packed & 0xf) == 8) {
        int32_t shift;

        for (shift = 4; shift <= 19; shift += 3) {
            group = group->next_sibling;
            selection = first_list_child(group)->selection_index;
            *packed = (*packed & ~(7u << shift)) | (((uint32_t)(int32_t)selection << shift) & (7u << shift));
        }
    }
    widget_extended_description_sync_selection(widget);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
