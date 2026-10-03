#pragma once

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "networking.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Visitor over the players that are driven by a local controller. HudWaypoints uses it to apply one waypoint
 * operation to every local player of a team, in the order the player data array yields them.
 */
class LocalPlayerVisitor {
public:
    virtual void visit(datum_index player_index) = 0;

    /**
     * Calls visitor.visit for every player slot with a local player index whose team equals team.
     */
    static void for_each_on_team(int16_t team, LocalPlayerVisitor &visitor);

protected:
    ~LocalPlayerVisitor() = default;
};

/**
 * Non-owning view of the four waypoint slots of one local player. A slot is free when the signed 4-bit kind in the
 * low bits of its type field is -1.
 */
class WaypointSlotSet {
public:
    static constexpr int16_t k_slot_count = 4;

    explicit WaypointSlotSet(hud_waypoint *slots) : slots(slots) {}

    /**
     * Resolves the slots of the local player that owns player_index; returns false when the index is invalid or the
     * player has no local controller.
     */
    static bool for_player(datum_index player_index, WaypointSlotSet *out);

    /**
     * Retargets the slot already showing (kind, target), or claims the last free slot for it. Does nothing when all
     * four slots are in use by other targets.
     */
    void activate(datum_index target, int16_t kind, int16_t arrow_index, float vertical_offset);

    /**
     * Frees the slot showing (kind, target) if there is one.
     */
    void deactivate(datum_index target, int16_t kind);

private:
    hud_waypoint *slots;
};

/**
 * Waypoint arrows of the player HUD: activation per player or team, arrow lookup and drawing.
 */
class HudWaypoints {
public:
    static void activate_for_player(datum_index player_index, datum_index target, int16_t kind, int16_t arrow_index, float vertical_offset);
    static void activate_for_team(datum_index target, int16_t arrow_index, int16_t team, int16_t kind, float vertical_offset);
    static int16_t arrow_find(const char *name);
    static void deactivate_for_player(datum_index player_index, datum_index target, int16_t kind);
    static void deactivate_for_team(int16_t kind, int16_t team, datum_index target);
    static void draw(const real_point3d *position, int16_t local_player_index, int16_t arrow_index, int16_t visibility, uint8_t show_distance);
    static void draw_all_for_player(void);
    static void draw_one(datum_index player_index);
};

}
