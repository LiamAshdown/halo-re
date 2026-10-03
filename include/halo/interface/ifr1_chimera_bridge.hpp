#pragma once

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Entry points of the chimera layer that load the main menu, UI widgets and the loading screen.
 */
class ChimeraBridge {
public:
    static void do_show_loading_screen(void);
    static void load_main_menu(void);
    static widget_instance * load_ui_widget(char *tag_path, datum_index tag_index, widget_instance *parent, uint16_t controller_index, datum_index history_definition, datum_index history_list_definition, int16_t history_selection);
    static void main_menu_music(uint8_t finalize_render_frame);
};

}
