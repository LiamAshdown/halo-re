#include "halo/networking/game_mode.hpp"
#include "halo/units/seat_detach.hpp"
#include "halo/units/animation_states.hpp"
#include "halo/objects/record_access.hpp"
#include <string.h>
#include "halo/models/api.hpp"
#include "halo/units/unit.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "hs.h"
#include "networking.h"
#include "crt.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/records.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

namespace halo::units {

namespace unit_detach_child_at_named_seat_local {

typedef struct unit_seat_iterator {
    uint32_t type_mask;
    uint8_t flags_mask;
    uint8_t unknown_05;
    int16_t index;
    datum_index handle;
    uint32_t signature;
} unit_seat_iterator;

}

/**
 * Engine function unit_detach_child_at_named_seat.
 *
 * @address 0x56ab50
 */
int16_t UnitView::detach_child_at_named_seat(char *seat_marker_name)
{
    using namespace unit_detach_child_at_named_seat_local;
    uint32_t unit_index = datum_handle;
    int16_t count = 0;
    Unit *unit_tag;
    uint8_t any_seat;
    unit_seat_iterator iterator;
    uint8_t *child;

    if (unit_index == k_datum_index_none) {
        return 0;
    }
    unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)halo::objects::object_record_bytes(unit_index));
    any_seat = (uint8_t)(seat_marker_name == 0 || seat_marker_name[0] == 0);
    iterator.signature = 0x86868686;
    iterator.type_mask = 3;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;
    for (child = (uint8_t *)halo::objects::object_iterator_next((object_iterator *)&iterator); child != 0;
         child = (uint8_t *)halo::objects::object_iterator_next((object_iterator *)&iterator)) {
        datum_index child_index;
        unit_object *self;
        char label[0x100];
        char *c;

        if (*(datum_index *)(child + 0x11c) != unit_index) {
            continue;
        }
        strcpy(label, (char *)(halo::objects::block_element<UnitSeat>(unit_tag->seats, *(int16_t *)(child + 0x2f0)).label.string));
        for (c = label; *c != 0; c++) {
            *c = (char)tolower((uint8_t)*c);
        }
        if (!any_seat && strstr(label, seat_marker_name) == 0) {
            continue;
        }
        child_index = iterator.handle;
        self = reinterpret_cast<unit_object *>(halo::objects::object_try_and_get(child_index, 3));
        if (self == 0 || halo::networking::globals().game_mode == halo::networking::k_game_mode_client || self->base.parent_object == k_datum_index_none ||
            self->unit.vehicle_seat_index == -1) {
            continue;
        }
        if (self->base.type == _object_type_vehicle) {
            unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(child_index));
            datum_index vehicle_index = obj->base.parent_object;

            if (vehicle_index != k_datum_index_none && obj->unit.vehicle_seat_index != -1) {
                biped_detach_from_seat(child_index, vehicle_index);
            }
            biped_free_local_player_history(halo::objects::object_as<unit_object>(child_index));
            continue;
        }
        if (!::halo::units::unit_state_is_scripted_animation(halo::units::unit_data_of(self))) {
            Unit *self_tag = halo::objects::tag_as<Unit>(*(datum_index *)self);
            datum_index graph = halo::objects::tag_handle(self_tag->base.animation_graph);
            uint8_t *seat_block = *(uint8_t **)(halo::objects::tag_record_bytes(graph) + 0x10) + (int8_t)(uint8_t)self->unit.animation_definition_index * 0x64;

            if (*(int32_t *)(seat_block + 0x40) > 8 && (*(int16_t **)(seat_block + 0x44))[8] != -1) {
                int16_t exit_animation = (*(int16_t **)(seat_block + 0x44))[8];
                datum_index vehicle_index = self->base.parent_object;
                uint8_t *object;
                Object *object_tag;

                if (((struct unit_object *)halo::objects::object_record_bytes(vehicle_index))->unit.driver_unit_index == child_index) {
                    UnitView((int32_t)vehicle_index).notify_weapon_removed();
                }
                UnitView(child_index).set_custom_animation(halo::objects::tag_handle(self_tag->base.animation_graph), halo::models::animation_choose_random_permutation(graph, exit_animation, (animation_random_stream)1));
                object = halo::objects::object_record_bytes(child_index);
                object_tag = halo::objects::tag_as<Object>(*(datum_index *)object);
                if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1) {
                    if (test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
                        halo::objects::object_for_each_light_attachment(child_index, 0, 1);
                    }
                    if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1) {
                        clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
                        halo::objects::object_header_of(child_index).flags |= 2;
                    }
                }
                self->unit.animation_state = animation_state_value(unit_animation_state_id::seat_exit);
                halo::ai::actor_notify_weapon_pickup_once(child_index);
                if (self->base.network_role == 0) {
                    ::halo::units::unit_dispatch_scripted_event_9(0, (int32_t)child_index);
                }
                count++;
            }
        }
    }
    return count;
}

}
