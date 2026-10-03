#include "halo/interface/ifr1_hud_frame.hpp"

/**
 * 0x4acbb0, blam-cc: EAX uv, EDX bitmap, CL pixel_uvs
 * C ABI entry point; forwards to halo::interface::HudFrame::draw_damage_indicators.
 * blam-cc: local_player_index -> EAX
 *
 * @address 0x4b14c0
 */
extern "C" void hud_draw_damage_indicators(int16_t local_player_index)
{
    halo::interface::HudFrame::draw_damage_indicators(local_player_index);
}

/**
 * C ABI entry point; forwards to halo::interface::HudFrame::draw_grenade_interface.
 *
 * @address 0x4b2ac0
 */
extern "C" void hud_draw_grenade_interface(int16_t local_player_index, datum_index unit_index)
{
    halo::interface::HudFrame::draw_grenade_interface(local_player_index, unit_index);
}

/**
 * C ABI entry point; forwards to halo::interface::HudFrame::draw_weapon_interface.
 *
 * @address 0x4b1e20
 */
extern "C" void hud_draw_weapon_interface(player *p)
{
    halo::interface::HudFrame::draw_weapon_interface(p);
}

/**
 * 0x4c29d0, blam-cc: EAX item_index
 * C ABI entry point; forwards to halo::interface::HudFrame::player_weapon_ammo_state.
 * blam-cc: player -> EAX
 *
 * @address 0x4acef0
 */
extern "C" uint8_t hud_player_weapon_ammo_state(const player *p, weapon_hud_ammo_state *out)
{
    return halo::interface::HudFrame::player_weapon_ammo_state(p, out);
}

/**
 * C ABI entry point; forwards to halo::interface::HudFrame::render_unit_interface.
 *
 * @address 0x4b0320
 */
extern "C" void hud_render_unit_interface(player *p)
{
    halo::interface::HudFrame::render_unit_interface(p);
}

/**
 * C ABI entry point; forwards to halo::interface::HudFrame::state_allocate.
 *
 * @address 0x4a9780
 */
extern "C" void hud_state_allocate(void)
{
    halo::interface::HudFrame::state_allocate();
}

/**
 * C ABI entry point; forwards to halo::interface::HudFrame::state_reset.
 *
 * @address 0x4a98d0
 */
extern "C" void hud_state_reset(void)
{
    halo::interface::HudFrame::state_reset();
}

/**
 * C ABI entry point; forwards to halo::interface::HudFrame::update_dispatch.
 *
 * @address 0x4a9990
 */
extern "C" void hud_update_dispatch(void)
{
    halo::interface::HudFrame::update_dispatch();
}

/**
 * C ABI entry point; forwards to halo::interface::HudFrame::update_interaction_prompt.
 * blam-cc: player_index -> EDX
 *
 * @address 0x4a9b80
 */
extern "C" void hud_update_interaction_prompt(datum_index player_index)
{
    halo::interface::HudFrame::update_interaction_prompt(player_index);
}

/**
 * C ABI entry point; forwards to halo::interface::HudFrame::update_player.
 *
 * @address 0x4a99f0
 */
extern "C" void hud_update_player(void)
{
    halo::interface::HudFrame::update_player();
}
