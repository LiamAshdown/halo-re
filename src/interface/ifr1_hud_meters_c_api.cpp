#include "halo/interface/ifr1_hud_meters.hpp"

/**
 * C ABI entry point; forwards to halo::interface::HudMeters::draw_fill.
 * blam-cc: placement -> ESI
 *
 * @address 0x4abbc0
 */
extern "C" void hud_meter_draw_fill(void *dest, uint8_t value_a, uint8_t value_b, uint32_t flags, float fraction, float fraction_2, const hud_meter_placement *meter)
{
    halo::interface::HudMeters::draw_fill(dest, value_a, value_b, flags, fraction, fraction_2, meter);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMeters::find_matching_elements.
 * blam-cc: EAX -> source_tag_ref, ECX -> target_tag_ref, stack -> out
 *
 * @address 0x493f00
 */
extern "C" uint8_t hud_meter_find_matching_elements(uint32_t source_tag_ref, uint32_t target_tag_ref, int16_t *out)
{
    return halo::interface::HudMeters::find_matching_elements(source_tag_ref, target_tag_ref, out);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMeters::flash_color_blend.
 * blam-cc: flash -> ESI, start_time -> EDI
 *
 * @address 0x4ab980
 */
extern "C" uint32_t hud_meter_flash_color_blend(const hud_flash_parameters *flash, int32_t start_time)
{
    return halo::interface::HudMeters::flash_color_blend(flash, start_time);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMeters::permute_node_records.
 * blam-cc: EAX -> target_tag_ref, EBX -> lookup, stack -> (dest, source)
 *
 * @address 0x493ea0
 */
extern "C" void hud_meter_permute_node_records(uint8_t *dest, uint8_t *source, uint32_t target_tag_ref, int16_t *lookup)
{
    halo::interface::HudMeters::permute_node_records(dest, source, target_tag_ref, lookup);
}

/**
 * 0x4ab630, blam-cc: ECX bitmap_tag, AX sequence_index, DI frame_index
 * C ABI entry point; forwards to halo::interface::HudMeters::resolve_bitmap_frame.
 * blam-cc: EAX -> frame_index, stack -> bitmap_tag, sequence_index, out_data, out_offset
 *
 * @address 0x4ab8d0
 */
extern "C" void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index, void **out_data, int32_t *out_offset)
{
    halo::interface::HudMeters::resolve_bitmap_frame(bitmap_tag, sequence_index, frame_index, out_data, out_offset);
}

/**
 * C ABI entry point; forwards to halo::interface::HudMeters::unit_meter_apply_predictive_damage.
 * blam-cc: player_index -> ECX, damage -> stack param_1
 *
 * @address 0x4b16e0
 */
extern "C" void hud_unit_meter_apply_predictive_damage(datum_index player_index, float damage)
{
    halo::interface::HudMeters::unit_meter_apply_predictive_damage(player_index, damage);
}

/**
 * 0x4b0160, blam-cc: DI local_player_index
 * C ABI entry point; forwards to halo::interface::HudMeters::unit_meters_update.
 *
 * @address 0x4b0110
 */
extern "C" void hud_unit_meters_update(void)
{
    halo::interface::HudMeters::unit_meters_update();
}

/**
 * 0x4afee0, blam-cc: EAX player
 * C ABI entry point; forwards to halo::interface::HudMeters::unit_meters_update_for_player.
 * blam-cc: local_player_index -> DI
 *
 * @address 0x4b0160
 */
extern "C" void hud_unit_meters_update_for_player(int16_t local_player_index)
{
    halo::interface::HudMeters::unit_meters_update_for_player(local_player_index);
}
