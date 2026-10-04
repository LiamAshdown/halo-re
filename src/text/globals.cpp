/**
 * @file src/text/globals.cpp
 * Binds halo::text::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/text/text.hpp"
#include "halo/text/api.hpp"
#include <stdarg.h>
#include "halo/text/limits.hpp"
#include "halo/core/datum.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/crt.hpp"
#include "halo/text/api.hpp"
#include "link/text.hpp"

static_assert(k_text_maximum_tab_stops == 16);

namespace halo::text {

Globals &Service::instance()
{
    static Globals state{
        ::hud_text_draw_color_a,
        ::hud_text_draw_font_tag_id,
        ::hud_text_draw_background_mode,
        ::hud_text_draw_color_or_flags,
        ::hud_text_draw_column,
        ::hud_text_draw_flags,
        ::text_localization_strings,
        ::text_color_scale,
        ::ui_prompt_clip_x,
        ::ui_prompt_clip_y,
        ::text_encoding_state,
        ::text_markup_codes,
        ::global_globals,
        ::missing_string,
        ::missing_string_text,
        ::text_measure_bounds,
        ::text_measure_font,
        ::text_highlight_start,
        ::text_highlight_end,
        ::text_tab_stops,
    };
    return state;
}

}  // namespace halo::text
