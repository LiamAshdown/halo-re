#include "halo/interface/ifr1_hud_waypoints.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::HudWaypoints::activate_for_player.
 * blam-cc: EAX -> player_index, EBX -> target, DX -> kind, stack -> arrow_index, vertical_offset
 *
 * @address 0x4af0d0
 */
void hud_waypoint_activate_for_player(datum_index player_index, datum_index target, int16_t kind, int16_t arrow_index, float vertical_offset)
{
    halo::interface::HudWaypoints::activate_for_player(player_index, target, kind, arrow_index, vertical_offset);
}

/**
 * C ABI entry point; forwards to halo::interface::HudWaypoints::activate_for_team.
 * blam-cc: target -> EAX
 *
 * @address 0x4af1b0
 */
void hud_waypoint_activate_for_team(datum_index target, int16_t arrow_index, int16_t team, int16_t kind, float vertical_offset)
{
    halo::interface::HudWaypoints::activate_for_team(target, arrow_index, team, kind, vertical_offset);
}

/**
 * C ABI entry point; forwards to halo::interface::HudWaypoints::arrow_find.
 * blam-cc: name -> EDI
 *
 * @address 0x4af070
 */
int16_t hud_waypoint_arrow_find(const char *name)
{
    return halo::interface::HudWaypoints::arrow_find(name);
}

/**
 * C ABI entry point; forwards to halo::interface::HudWaypoints::deactivate_for_player.
 * blam-cc: EAX -> player_index, EDI -> target, SI -> kind
 *
 * @address 0x4af230
 */
void hud_waypoint_deactivate_for_player(datum_index player_index, datum_index target, int16_t kind)
{
    halo::interface::HudWaypoints::deactivate_for_player(player_index, target, kind);
}

/**
 * C ABI entry point; forwards to halo::interface::HudWaypoints::deactivate_for_team.
 * blam-cc: kind -> EAX
 *
 * @address 0x4af2b0
 */
void hud_waypoint_deactivate_for_team(int16_t kind, int16_t team, datum_index target)
{
    halo::interface::HudWaypoints::deactivate_for_team(kind, team, target);
}

/**
 * C ABI entry point; forwards to halo::interface::HudWaypoints::draw.
 * blam-cc: position -> EAX
 *
 * @address 0x4af5e0
 */
void hud_waypoint_draw(const real_point3d *position, int16_t local_player_index, int16_t arrow_index, int16_t visibility, uint8_t show_distance)
{
    halo::interface::HudWaypoints::draw(position, local_player_index, arrow_index, visibility, show_distance);
}

/**
 * C ABI entry point; forwards to halo::interface::HudWaypoints::draw_all_for_player.
 *
 * @address 0x4aa5f0
 */
void hud_waypoint_draw_all_for_player(void)
{
    halo::interface::HudWaypoints::draw_all_for_player();
}

/**
 * C ABI entry point; forwards to halo::interface::HudWaypoints::draw_one.
 * blam-cc: player_index -> EAX
 *
 * @address 0x4aa440
 */
void hud_waypoint_draw_one(datum_index player_index)
{
    halo::interface::HudWaypoints::draw_one(player_index);
}

}
