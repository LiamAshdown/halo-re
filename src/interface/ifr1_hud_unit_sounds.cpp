#include "halo/interface/ifr1_hud_unit_sounds.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/interface/flags.hpp"

static auto &game_looping_sound_data = halo::link::ref<data_array *>(halo::ui::vars().game_looping_sound_data);
static auto &hud_unit_meters = halo::link::ref<hud_unit_meter_globals *>(halo::ui::vars().hud_unit_meters);

namespace halo::interface {

/**
 *
 * @address 0x4afd30
 */
void HudUnitSounds::play(uint32_t active_mask, const TagReflexive *sounds, int32_t *handles, uint16_t *playing)
{
    int16_t i;

    for (i = 0; (int32_t)i < (int32_t)sounds->count; i++) {
        const UnitHUDInterfaceHUDSound *sound = (const UnitHUDInterfaceHUDSound *)sounds->pointer + i;
        uint8_t is_looping = sound->sound.tag_fourcc == halo::fourcc('l', 's', 'n', 'd');

        if ((active_mask & *(const uint32_t *)&sound->latched_to) != 0) {
            if (is_looping) {
                if (handles[i] == -1) {
                    datum_index tag = *(const datum_index *)&sound->sound.tag_id;
                    datum_index handle = k_datum_index_none;

                    if (tag != k_datum_index_none) {
                        handle = halo::memory::datum_new(halo::sound::globals().game_looping_sound_data);
                        if (handle != k_datum_index_none) {
                            uint8_t *element = (uint8_t *)halo::sound::globals().game_looping_sound_data->data + (handle & halo::k_slot_mask) * 0x34;
                            *(int32_t *)&((game_looping_sound *)element)->object_index = -1;
                            ((game_looping_sound *)element)->definition_index = tag;
                            ((game_looping_sound *)element)->state = 2;
                            *(int32_t *)&((game_looping_sound *)element)->flags = 0;
                            ((game_looping_sound *)element)->function_index = -1;
                            ((game_looping_sound *)element)->last_update = -1;
                            ((game_looping_sound *)element)->flags |= 1;
                            ((game_looping_sound *)element)->scale = sound->scale;
                        }
                    }
                    handles[i] = (int32_t)handle;
                }
            } else if (handles[i] == -1 || (*playing & (1u << i)) == 0) {
                hud_sound_start_parameters parameters;

                if (handles[i] != -1) {
                    halo::sound::sound_impulse_fade_out(handles[i]);
                }
                parameters.unknown_00 = 0;
                parameters.scale = sound->scale;
                parameters.gain = 1.0f;
                handles[i] = halo::sound::sound_play_new(*(const datum_index *)&sound->sound.tag_id, (sound_location *)&parameters, -1, 0, 0, 0, 0);
            }
            *playing |= (uint16_t)(1u << i);
        } else if (handles[i] != -1) {
            if (is_looping) {
                uint8_t *element = (uint8_t *)halo::sound::globals().game_looping_sound_data->data + (handles[i] & halo::k_slot_mask) * 0x34;
                ((game_looping_sound *)element)->flags |= 2;
            }
            handles[i] = -1;
            *playing &= (uint16_t)~(1u << i);
        }
    }
}

/**
 *
 * blam-cc: player -> EAX
 *
 * @address 0x4afee0
 */
void HudUnitSounds::update(player *p, uint8_t hud_enabled)
{
    hud_unit_meter_state *state = &hud_unit_meters->players[p->local_player_index];
    datum_index unit_index = p->unit;
    uint8_t *unit;
    Unit *unit_tag;
    UnitHUDInterface *hud;
    int32_t choice;
    int32_t last;
    datum_index hud_tag;
    uint32_t mask;

    if (unit_index == k_datum_index_none) {
        unit_index = state->last_unit;
    }
    unit = (uint8_t *)halo::objects::object_try_and_get(unit_index, 3);
    if (unit == 0) {
        return;
    }
    unit_tag = halo::interface::tag_data<Unit>(*(datum_index *)unit);
    choice = (int16_t)(halo::game::globals().local_player_globals->local_player_count > 1);
    last = (int32_t)((struct Unit *)unit_tag)->new_hud_interfaces.count - 1;
    if (choice > last) {
        choice = last;
    }
    if ((int16_t)choice < 0) {
        return;
    }
    hud_tag = halo::interface::tag_handle(halo::interface::reflexive_elements<UnitUnitHudInterface>(((struct Unit *)unit_tag)->new_hud_interfaces)[(int16_t)choice].hud.tag_id);
    if (hud_tag == k_datum_index_none) {
        return;
    }
    hud = halo::interface::tag_data<UnitHUDInterface>(hud_tag);

    mask = 0;
    if ((unit[0x10] & 4) != 0 || !(((unit_object *)unit)->base.body_vitality > 0.0f)) {
        state->last_unit = k_datum_index_none;
    } else if (hud_enabled != 0 && halo::cutscene::globals().cinematic_globals->in_progress == 0) {
        float shield = ((unit_object *)unit)->base.shield_vitality;
        float health = ((unit_object *)unit)->base.body_vitality;

        if (state->displayed_shield != -1.0f && halo::game::game_engine_object_flag_bit3_clear(halo::game::local_player_to_player_index(p->local_player_index)) != 0 &&
            (hud_unit_meters->flags & 4) == 0) {
            mask = (((unit_object *)unit)->base.vitality_flags >> 12) & 1;
            if (state->displayed_shield > shield) {
                mask |= 2;
            }
            if (shield < 0.25f && shield > 0.0f) {
                mask |= 4;
            }
            if (shield == 0.0f) {
                mask |= 8;
            }
        }
        if ((hud_unit_meters->flags & 1) == 0) {
            if (health < 0.25f) {
                mask |= 0x10;
            }
            if (halo::interface::has_bit(((object *)unit)->vitality_flags, halo::objects::vitality_flag::health_frozen)) {
                mask |= 0x20;
            }
            if (state->displayed_health > health && state->displayed_health - health < 0.1875f) {
                mask |= 0x40;
            }
            if (!(state->displayed_health - health < 0.1875f)) {
                mask |= 0x80;
            }
        }
    }
    halo::interface::hud_unit_sounds_play(mask, &hud->sounds, state->sound_handles, &state->sounds_playing);
}

}
