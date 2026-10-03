#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"
#include "units.h"
#include "cutscene.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * HUD unit meters: fill drawing, flash colour blend, bitmap frame lookup and predictive damage.
 */
class HudMeters {
public:
    static void draw_fill(void *dest, uint8_t value_a, uint8_t value_b, uint32_t flags, float fraction, float fraction_2, const hud_meter_placement *meter);
    static uint8_t find_matching_elements(uint32_t source_tag_ref, uint32_t target_tag_ref, int16_t *out);
    static uint32_t flash_color_blend(const hud_flash_parameters *flash, int32_t start_time);
    static void permute_node_records(uint8_t *dest, uint8_t *source, uint32_t target_tag_ref, int16_t *lookup);
    static void resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index, void **out_data, int32_t *out_offset);
    static void unit_meter_apply_predictive_damage(datum_index player_index, float damage);
    static void unit_meters_update(void);
    static void unit_meters_update_for_player(int16_t local_player_index);
};

}
