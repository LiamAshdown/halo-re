/**
 * @file src/interface/coop_option.cpp
 * The CO-OP row of Choose Difficulty (ui\shell\main_menu\difficulty_select\difficulty_select_list_screen). It is a
 * second instance of the LEGENDARY row's widget, linked in after it, so it looks and moves like the difficulty rows;
 * pressing it toggles whether the campaign game about to start may be joined (src/game/lockstep.cpp) instead of
 * choosing a difficulty, and while it has focus the description box explains it.
 */

#include "halo/interface/coop_option.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/records.hpp"
#include "halo/interface/ui_event.hpp"
#include "halo/interface/widget_pool.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/game/lockstep.hpp"
#include "tags.h"
#include "interface.h"

#include <string.h>

namespace halo::interface::coop_option {

namespace {

constexpr char k_screen[] = "ui\\shell\\main_menu\\difficulty_select\\difficulty_select_list_screen";
constexpr char k_list[] = "ui\\shell\\main_menu\\difficulty_select\\difficulty_select_list";
constexpr char k_last_row[] = "ui\\shell\\main_menu\\difficulty_select\\impossible_difficulty_item";
constexpr int16_t k_row_height = 33;  // the difficulty rows' spacing (bounds 78, 111, 144, 177)
// the description box breaks lines only at \r\n, like difficulty_descriptions
constexpr char k_description[] =
    "Let other players join this\r\ngame from Multiplayer, Join\r\nGame. The level restarts\r\nwhen someone joins.";

widget_instance *g_row;
widget_instance *g_list;
uint16_t g_text[0x100];

const uint16_t *utf16(const char *text)
{
    size_t i;

    for (i = 0; text[i] != 0 && i < 0xff; i++) {
        g_text[i] = static_cast<uint8_t>(text[i]);
    }
    g_text[i] = 0;
    return g_text;
}

widget_instance *description()
{
    widget_instance *panel = g_list != nullptr ? g_list->extended_description : nullptr;

    return panel != nullptr && panel->first_child != nullptr ? panel->first_child->next_sibling : nullptr;
}

}  // namespace

void screen_loaded(widget_instance *root, datum_index tag_index)
{
    if (root == nullptr || tag_index != halo::interface::lookup_tag(halo::groups::ui_widget_definition, k_screen)) {
        return;
    }
    widget_instance *list = halo::interface::widget_find_by_tag_id(root, halo::interface::lookup_tag(halo::groups::ui_widget_definition, k_list));
    widget_instance *last = halo::interface::widget_find_by_tag_id(root, halo::interface::lookup_tag(halo::groups::ui_widget_definition, k_last_row));

    if (list == nullptr || last == nullptr) {
        return;
    }
    widget_instance *row = halo::interface::chimera__load_ui_widget(nullptr, last->definition, list, list->controller_index,
        k_datum_index_none, k_datum_index_none, -1);

    if (row == nullptr) {
        return;
    }
    row->local_x = last->local_x;
    row->local_y = static_cast<int16_t>(last->local_y + k_row_height);
    row->previous_sibling = last;
    row->next_sibling = last->next_sibling;
    if (last->next_sibling != nullptr) {
        last->next_sibling->previous_sibling = row;
    }
    last->next_sibling = row;
    g_row = row;
    g_list = list;
}

void closed(widget_instance *widget)
{
    if (widget == g_row) {
        g_row = nullptr;
    }
    if (widget == g_list) {
        g_list = nullptr;
    }
}

bool handle_event(widget_instance *widget, const int16_t *event)
{
    // A or Start on the row (a click arrives as A: its left-mouse handler pushes one)
    if (widget == nullptr || widget != g_row || event[0] != 3 || halo::interface::event_state(event) != 1 ||
        (halo::interface::event_code(event) != 0 && halo::interface::event_code(event) != 12)) {
        return false;
    }
    halo::game::lockstep::set_coop_allowed(!halo::game::lockstep::coop_allowed());
    halo::interface::widget_play_sound_effect(2);
    return true;
}

const uint16_t *text(widget_instance *widget)
{
    if (widget == nullptr) {
        return nullptr;
    }
    if (widget == g_row) {
        return utf16(halo::game::lockstep::coop_allowed() ? "CO-OP: ON" : "CO-OP: OFF");
    }
    if (g_row != nullptr && widget == description() && g_list->focused_child == g_row) {
        return utf16(k_description);
    }
    return nullptr;
}

}  // namespace halo::interface::coop_option
