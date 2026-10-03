#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Interface subsystem lifecycle, loading screen, main menu and small utilities.
 */
class InterfaceMain {
public:
    InterfaceMain() = delete;

    static void draw_cursor();
    static void globals_allocate();
    static void handle_quit_request();
    static void loading_screen_reset();
    static void loading_screen_set_text(const char *text);
    static void tick();
    static void update_for_resolution_change(int32_t new_cursor_x, int32_t new_cursor_y);
    static void on_shown(int32_t fade_milliseconds);
    static void play_title_music();
    static void * registry_get_product_id();
    static void set_profile_name(widget_instance *widget, const uint16_t *name_source);
    static void string_replace_all_in_place(char *buffer, char *search, char *replacement);
    static void initialize_terminal();
};

/**
 * Known map list management.
 */
class MapList {
public:
    MapList() = delete;

    static void add_entry(char *path, int32_t map_id);
    static int32_t find_known_map_index(char *map_path);
    static void free_all();
    static void get_friendly_level_name(wchar_t *destination, char *map_path, int32_t destination_capacity);
};

} // namespace halo::interface
