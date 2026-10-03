#include "halo/interface/ifr1_chimera_bridge.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::ChimeraBridge::do_show_loading_screen.
 *
 * @address 0x497410
 */
void chimera__do_show_loading_screen(void)
{
    halo::interface::ChimeraBridge::do_show_loading_screen();
}

/**
 * C ABI entry point; forwards to halo::interface::ChimeraBridge::load_main_menu.
 *
 * @address 0x4989f0
 */
void chimera__load_main_menu(void)
{
    halo::interface::ChimeraBridge::load_main_menu();
}

/**
 * C ABI entry point; forwards to halo::interface::ChimeraBridge::load_ui_widget.
 * blam-cc: cdecl, 7 stack arguments (every caller pushes 0x1c bytes); objdump 0x497a70..0x497bdc
 *
 * @address 0x497a70
 */
widget_instance * chimera__load_ui_widget(const char *tag_path, datum_index tag_index, widget_instance *parent, uint16_t controller_index, datum_index history_definition, datum_index history_list_definition, int16_t history_selection)
{
    return halo::interface::ChimeraBridge::load_ui_widget(tag_path, tag_index, parent, controller_index, history_definition, history_list_definition, history_selection);
}

/**
 * C ABI entry point; forwards to halo::interface::ChimeraBridge::main_menu_music.
 *
 * @address 0x4921a0
 */
void chimera__main_menu_music(uint8_t finalize_render_frame)
{
    halo::interface::ChimeraBridge::main_menu_music(finalize_render_frame);
}

}
