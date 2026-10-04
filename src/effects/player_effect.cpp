#include "halo/game/constants.hpp"
#include "halo/game/records.hpp"
#include <cstring>
#include "halo/core/bit_cast.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/tags/flags.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/effects/effects.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"

static auto &player_effect_globals_pointer = halo::link::ref<player_effect_globals *>(halo::effects::vars().player_effect_globals_pointer);
static auto &global_white_argb = halo::link::ref<const ColorARGB *>(halo::networking::vars().global_white_argb);
static auto &screen_flash_pass = halo::link::ref<int16_t [8]>(halo::effects::vars().screen_flash_pass);
static auto &player_effect_reentry_count = halo::link::ref<int32_t>(halo::effects::vars().player_effect_reentry_count);
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &network_message_scratch = halo::link::ref<uint8_t [halo::k_network_message_scratch_size]>(halo::game::vars().network_message_scratch);

namespace halo::effects {

namespace {
/** The 0x18-byte screen flash record player_effect_build_screen_flash hands to the renderer. */
struct screen_flash_output {
    uint16_t pass;
    uint16_t pad_02;
    float intensity;
    ColorARGB color;
};
static_assert(sizeof(screen_flash_output) == 0x18);
}

/**
 * Member form of the original player_effect_apply_at_object: apply at object.
 *
 * @address 0x456900
 */
void player_effect_ref::apply_at_object(uint32_t tag_reference, int16_t local_player_index, real_point3d *origin)
{
    datum_index player_index = halo::game::globals().local_player_globals->local_players[0];

    if (player_index != k_datum_index_none) {
        player *record = &((player *)halo::game::globals().player_data->data)[player_index & halo::k_slot_mask];

        if (record->unit != k_datum_index_none) {
            real_point3d position;
            float dx, dy, dz;

            halo::objects::object_get_position(&position, record->unit);
            dx = origin->x - position.x;
            dy = origin->y - position.y;
            dz = origin->z - position.z;
            halo::effects::player_effect_apply_continuous_damage(tag_reference, local_player_index,
                (float)halo::libm::sqrt((double)(dy * dy + dx * dx + dz * dz)));
        }
    }
}

/**
 * Member form of the original player_effect_apply_continuous_damage: apply continuous damage.
 *
 * @address 0x4567c0
 */
void player_effect_ref::apply_continuous_damage(uint32_t tag_reference, int16_t local_player_index, float distance)
{
    ContinuousDamageEffect *effect =
        (ContinuousDamageEffect *)halo::cache::globals().tag_instances[tag_reference & halo::k_slot_mask].data;

    if (distance < effect->radius[1]) {
        player_effect *self = &player_effect_globals_pointer->players[local_player_index];
        float fraction = 1.0f - (distance - effect->radius[0]) /
                                 (effect->radius[0] - effect->radius[1]);
        float wobble, weighted, delta;

        fraction = (fraction < 0.0f) ? 0.0f : (1.0f < fraction ? 1.0f : fraction);

        wobble = (float)halo::math::periodic_function_evaluate(_periodic_function_cosine,
            (double)((float)halo::game::globals().game_time->game_time / effect->camera_shaking_wobble_period));
        weighted = ((1.0f - effect->camera_shaking_wobble_weight) +
                    wobble * effect->camera_shaking_wobble_weight) * fraction;

        if (0 < self->vibrate_ticks) {
            self->vibrate_ticks = 0;
            self->low_frequency_vibrate = 0.0f;
            self->high_frequency_vibrate = 0.0f;
            self->shake_translation = 0.0f;
            self->shake_rotation = 0.0f;
        }

        delta = weighted * effect->camera_shaking_random_translation;
        if (delta < 0.0f) delta = 0.0f;
        self->shake_translation += delta;

        delta = weighted * effect->camera_shaking_random_rotation;
        if (delta < 0.0f) delta = 0.0f;
        self->shake_rotation += delta;

        self->low_frequency_vibrate += fraction * effect->low_frequency_vibrate_frequency;
        self->high_frequency_vibrate += fraction * effect->high_frequency_vibrate_frequency;
    }
}

/**
 * FIXED (objdump 0x4569d0..0x456aca): the flash is {type 1, +0x02 = 2, duration 1.0, maximum (+0x20) = fraction,
 * weight (+0x24) 0, colour opaque white} and the shake {duration 1.0, +0x08 = fraction * 0.01}; both are applied
 * with (fraction, 1.0). The draft used type 2, the fraction as the duration, weight 1.0 and the shake's +0x04.
 *
 * @address 0x4569d0
 */
void player_effect_ref::apply_generic_damage_feedback(float fraction)
{
    datum_index player_index = datum;
    player_screen_flash flash_descriptor;
    player_camera_shake shake_descriptor;
    int16_t local_player_index;

    memset(&flash_descriptor, 0, sizeof(flash_descriptor));
    memset(&shake_descriptor, 0, sizeof(shake_descriptor));

    local_player_index = ((player *)halo::game::globals().player_data->data)[player_index & halo::k_slot_mask].local_player_index;
    if (local_player_index != -1) {
        player_effect *self = &player_effect_globals_pointer->players[local_player_index];

        shake_descriptor.random_translation = (float)((double)fraction * 0.01);
        shake_descriptor.duration = 1.0f;
        flash_descriptor.type = 1;
        flash_descriptor.priority = 2;
        flash_descriptor.duration = 1.0f;
        flash_descriptor.maximum_intensity = halo::bit_cast<uint32_t>(fraction);
        flash_descriptor.intensity = 0.0f;
        flash_descriptor.color = *global_white_argb;

        halo::effects::player_effect_set_screen_flash(self, &flash_descriptor, fraction, 1.0f);
        halo::effects::player_effect_set_camera_shake(self, &shake_descriptor, fraction, 1.0f);
    }
}

/**
 * FIXED (objdump 0x457000..0x457210): the scripted flash fades through transition type 5 (ECX = 5 at 0x4570d6),
 * the player flash through its own type (flash +0x14, i.e. self +0x2c), and the player flash ticks count down by
 * the tick length (game_time +0x10), not 1. The scripted ticks are reset only once a local player is given.
 * The pass index is written as a word.
 *
 * @address 0x457000
 */
void player_effect_ref::build_screen_flash(uint32_t *out_words, int16_t local_player_index)
{
    player_effect_globals *globals = player_effect_globals_pointer;
    screen_flash_output *out = reinterpret_cast<screen_flash_output *>(out_words);

    if (halo::main::globals().console_globals.active != 0) {
        return;
    }

    if (globals->scripted_flash_ticks != -1 &&
        (globals->scripted_flash_fade_in != 0 ||
         halo::game::globals().game_time->game_time - globals->scripted_flash_start_tick <= (int32_t)globals->scripted_flash_ticks)) {
        float fraction;

        out->pass = 1;
        out->color.red = globals->scripted_flash_color.red;
        out->color.green = globals->scripted_flash_color.green;
        out->color.blue = globals->scripted_flash_color.blue;
        out->color.alpha = 1.0f;
        if (globals->scripted_flash_ticks < 1) {
            fraction = 1.0f;
        } else {
            float t = (float)(halo::game::globals().game_time->game_time - globals->scripted_flash_start_tick) /
                      (float)(int32_t)globals->scripted_flash_ticks;

            if (!(t >= 0.0f)) {
                t = 0.0f;
            } else if (!(t <= 1.0f)) {
                t = 1.0f;
            }
            fraction = halo::math::transition_function_evaluate(5, t);
        }
        out->intensity = fraction;
        if (globals->scripted_flash_fade_in == 0) {
            out->intensity = 1.0f - fraction;
        }
        if (!(out->intensity >= 0.0f)) {
            out->intensity = 0.0f;
        } else if (!(out->intensity <= 1.0f)) {
            out->intensity = 1.0f;
        }
        return;
    }

    if (local_player_index != -1) {
        player_effect *self = &globals->players[local_player_index];

        globals->scripted_flash_ticks = -1;
        if (0 < self->flash_ticks || (self->flags & 1) != 0) {
            self->flags &= ~(uint32_t)1;
            out->pass = (uint16_t)screen_flash_pass[self->flash.type];
            out->color = self->flash.color;
            if (self->flash.duration > 0.0f) {
                float fraction = ((float)(int32_t)self->flash_ticks / self->flash.duration) * self->flash.intensity;

                out->intensity = halo::math::transition_function_evaluate(static_cast<int16_t>(self->flash.fade_function), fraction);
            } else {
                out->intensity = self->flash.intensity;
            }
            self->flash_ticks = (int16_t)(self->flash_ticks - halo::game::globals().game_time->ticks_this_frame);
        }
    }
}

/**
 * Member form of the original player_effect_clear_dead_players: clear dead players.
 *
 * @address 0x456730
 */
void player_effect_ref::clear_dead_players()
{
    int32_t i;

    for (i = 0; i < k_maximum_local_player_effects; i++) {
        datum_index player_index = halo::game::globals().local_player_globals->local_players[i];
        uint8_t dead = 1;

        if (player_index != k_datum_index_none) {
            player *record = &((player *)halo::game::globals().player_data->data)[player_index & halo::k_slot_mask];
            if (record->unit != k_datum_index_none) {
                dead = 0;
            }
        }

        if (dead) {
            memset(&player_effect_globals_pointer->players[i], 0, sizeof(player_effect));
        }
    }
}

/**
 * Fades one local player's four directional damage indicators upward toward 255 by the current
 * tick delta, and hands the caller the pre-fade byte values so it can detect a change.
 *
 * @address 0x457220
 */
void player_effect_ref::fade_damage_indicators(int16_t local_player_index, uint32_t *out_previous_indicators)
{
    player_effect *self = &player_effect_globals_pointer->players[local_player_index];
    uint8_t *indicators = self->damage_indicator_alpha;
    int16_t delta = halo::game::globals().game_time->ticks_this_frame;
    int i;

    std::memcpy(out_previous_indicators, indicators, sizeof(*out_previous_indicators));

    for (i = 0; i < 4; i++) {
        if (indicators[i] != 0) {
            int32_t faded = (int32_t)indicators[i] + delta;
            if (faded > 0xfe) {
                faded = 0xff;
            }
            indicators[i] = (uint8_t)faded;
        }
    }
}

/**
 * Member form of the original player_effect_mark_damage_direction: mark damage direction.
 *
 * @address 0x456cf0
 */
void player_effect_ref::mark_damage_direction(const damage_data *dd, const real_vector3d *direction, float random_blend, float damage_amount)
{
    datum_index player_index = datum;
    int16_t local_player_index = ((player *)halo::game::globals().player_data->data)[player_index & halo::k_slot_mask].local_player_index;
    player_effect *self;
    DamageEffect *tag;

    player_effect_reentry_count++;
    if (local_player_index == -1) {
        player_effect_reentry_count--;
        return;
    }
    self = &player_effect_globals_pointer->players[local_player_index];
    tag = halo::objects::tag_as<DamageEffect>(dd->damage_effect_tag);
    halo::effects::player_effect_set_screen_flash(self, reinterpret_cast<player_screen_flash *>(&tag->type), random_blend, 1.0f);
    halo::effects::player_effect_set_camera_impulse(self, local_player_index, reinterpret_cast<real *>(&tag->temporary_camera_impulse_duration), (real *)direction,
        random_blend, 1.0f);
    halo::effects::player_effect_set_camera_shake(self, reinterpret_cast<player_camera_shake *>(&tag->camera_shaking_duration), random_blend, 1.0f);
    if (halo::objects::tag_handle(tag->sound) != k_datum_index_none) {
        sound_location location;

        memset(&location, 0, sizeof(location));
        location.scale = 1.0f;
        location.gain = 1.0f;
        halo::sound::sound_play_new(halo::objects::tag_handle(tag->sound), &location, k_datum_index_none, 0, 0, 0, 0);
    }
    if (damage_amount > 0.0f && dd->responsible_object != k_datum_index_none) {
        datum_index controlling_player;
        datum_index unit_index;
        observer_camera *camera;
        real_point3d eye;
        real_point3d source;
        real_vector3d delta;
        real_vector3d side;
        real_vector3d projected;
        double angle;
        float abs_angle;

        if (test_flag(tag->damage_flags, tags::damage_effect_damage_tag_flag::damage_indicators_always_point_down)) {
            self->damage_indicator_alpha[2] = 1;
            player_effect_reentry_count--;
            return;
        }
        controlling_player = halo::game::local_player_to_player_index(local_player_index);
        unit_index = (controlling_player == k_datum_index_none) ? k_datum_index_none :
            ((player *)halo::game::globals().player_data->data)[controlling_player & halo::k_slot_mask].unit;
        if (halo::objects::object_try_and_get(unit_index, 3) == 0 ||
            halo::objects::object_try_and_get(dd->responsible_object, _object_mask_all) == 0) {
            player_effect_reentry_count--;
            return;
        }
        camera = halo::camera::observer_get_camera(local_player_index);
        if (camera == 0) {
            player_effect_reentry_count--;
            return;
        }
        halo::units::unit_get_primary_eye_marker_position(unit_index, &eye);
        halo::objects::object_get_position(&source, dd->responsible_object);
        delta.i = source.x - eye.x;
        delta.j = source.y - eye.y;
        delta.k = source.z - eye.z;
        halo::math::vector3d_cross_product(side, *((const real_vector3d *)&camera->up), *((const real_vector3d *)&camera->forward));
        projected.i = side.k * delta.k + side.j * delta.j + side.i * delta.i;
        projected.j = delta.k * camera->forward.k + delta.j * camera->forward.j + delta.i * camera->forward.i;
        projected.k = delta.k * camera->up.k + delta.j * camera->up.j + delta.i * camera->up.i;
        if (halo::math::vector3d_normalize_with_length(projected) == 0.0f) {
            player_effect_reentry_count--;
            return;
        }
        if (halo::libm::fabs(projected.k) > 0.5) {
            if (projected.k > 0.0f) {
                self->damage_indicator_alpha[0] = 1;
            } else {
                self->damage_indicator_alpha[2] = 1;
            }
        }
        angle = halo::libm::atan2(projected.j, projected.i);
        abs_angle = (float)halo::libm::fabs(angle);
        if (angle < 0.78539819f || angle > 2.3561945f) {
            if (abs_angle > 1.5707964f) {
                self->damage_indicator_alpha[1] = 1;
            } else {
                self->damage_indicator_alpha[3] = 1;
            }
        }
    }
    player_effect_reentry_count--;
}

/**
 * Member form of the original player_effect_mark_damage_direction_dispatch: mark damage direction dispatch.
 *
 * @address 0x456ad0
 */
void player_effect_ref::mark_damage_direction_dispatch(void **context)
{
    uint32_t fields[8];
    data_iterator iterator;
    player *record;
    damage_data dd;

    if (**(int32_t **)context != 0) {
        halo::networking::message_delta_decode_compound_field_staged(context);
        return;
    }
    memset(fields, 0, sizeof(fields));
    if (!halo::networking::message_delta_decode_compound_field(context, fields)) {
        return;
    }
    iterator.data = halo::game::globals().player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    for (record = (player *)halo::memory::data_iterator_next(&iterator); record != 0;
         record = (player *)halo::memory::data_iterator_next(&iterator)) {
        if (record->local_player_index != -1) {
            dd.damage_effect_tag = fields[0];
            dd.responsible_object = (fields[1] != 0) ?
                object_network_id_table->handles[fields[1]] : k_datum_index_none;
            dd.flags = fields[2];
            halo::effects::player_effect_mark_damage_direction(iterator.index, &dd, (const real_vector3d *)&fields[3],
                halo::bit_cast<float>(fields[6]), halo::bit_cast<float>(fields[7]));
            return;
        }
    }
}

/**
 * Builds a random offset matrix: when `angle` is nonzero, a rotation of `angle` radians about a
 * random unit axis (via matrix4x3_from_axis_angle); when `magnitude` is nonzero, a random
 * direction scaled by `magnitude` written into the matrix's position field.
 *
 * @address 0x457280
 */
void player_effect_ref::random_shake_offset(real_matrix4x3 *out, real magnitude, real angle)
{
    if (angle != 0.0f) {
        real cos_angle = (real)halo::libm::cos((double)angle);
        real_vector3d axis;
        int16_t axis_index;

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        axis_index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) *
            (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);
        axis = *(real_vector3d *)&halo::math::globals().sphere_point_table[axis_index];

        {
            real sin_angle = (real)halo::libm::sin((double)angle);
            halo::math::matrix4x3_from_axis_angle(*out, axis, sin_angle, cos_angle);
        }
    }

    if (magnitude != 0.0f) {
        int16_t index;

        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * k_random_multiplier + k_random_increment;
        index = (int16_t)(((halo::math::globals().effect_random_seed >> k_random_value_shift) *
            (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);

        out->position.x = halo::math::globals().sphere_point_table[index].x * magnitude;
        out->position.y = halo::math::globals().sphere_point_table[index].y * magnitude;
        out->position.z = halo::math::globals().sphere_point_table[index].z * magnitude;
    }
}

/**
 * Member form of the original player_effect_send_network_update: send network update.
 *
 * @address 0x456bc0
 */
void player_effect_ref::send_network_update(const real_vector3d *direction, const damage_data *dd, float random_blend, float damage_amount)
{
    datum_index player_handle = datum;
    int16_t index = (int16_t)player_handle;
    int16_t salt = (int16_t)(player_handle >> 16);
    player *record;
    uint32_t fields[8];
    void *items[2];
    int32_t encoded_bits;

    if (player_handle == k_datum_index_none || index < 0 || index >= halo::game::globals().player_data->maximum_count) {
        return;
    }
    record = halo::game::player_at(index);
    if (record->identifier == 0 || (salt != 0 && record->identifier != salt) || record->marked_for_deletion != 0) {
        return;
    }
    fields[0] = dd->damage_effect_tag;
    fields[1] = 0;
    if (dd->responsible_object != k_datum_index_none) {
        fields[1] = (uint32_t)halo::objects::hash_table_get(&object_network_id_table->id_to_index,
            (int32_t)dd->responsible_object);
        if (fields[1] == halo::k_dword_none) {
            fields[1] = 0;
        }
    }
    fields[2] = dd->flags;
    fields[3] = halo::bit_cast<uint32_t>(direction->i);
    fields[4] = halo::bit_cast<uint32_t>(direction->j);
    fields[5] = halo::bit_cast<uint32_t>(direction->k);
    fields[6] = halo::bit_cast<uint32_t>(random_blend);
    fields[7] = halo::bit_cast<uint32_t>(damage_amount);
    items[0] = fields;
    items[1] = 0;
    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, 0xb, 0, items, 0, 1, 0);
    if (encoded_bits > 0 && (int8_t)record->machine_index != -1) {
        halo::networking::network_session_send_to_machine((int8_t)record->machine_index, halo::networking::globals().server, 1, network_message_scratch,
            (uint32_t)encoded_bits, 1, 0, 1, 3);
    }
}

/**
 * Member form of the original player_effect_set_camera_shake: set camera shake.
 *
 * @address 0x457d50
 */
void player_effect_view::set_camera_shake(player_camera_shake *descriptor, float intensity_falloff, float duration_scale)
{
    player_effect * self = record;
    float duration = duration_scale * halo::game::k_ticks_per_second_f;
    float blended = (1.0f - descriptor->intensity) * intensity_falloff + descriptor->intensity;

    if (duration * descriptor->duration <= (float)self->shake_ticks &&
        blended <= self->shake.intensity &&
        (blended < self->shake.intensity || duration * descriptor->duration <= (float)self->shake_ticks)) {
        return;
    }

    self->shake = *descriptor;
    self->shake.intensity = blended;
    self->shake.duration = duration * self->shake.duration;
    self->shake_ticks = (int16_t)self->shake.duration;
    self->flags |= _player_effect_camera_shake_bit;
    self->shake.wobble_period = duration * self->shake.wobble_period;
}

/**
 * VERIFIED against disassembly 0x4578a0..0x4579a7 (2026-09-30): the update condition, the 14-dword copy, the ticks __ftol
 * (scaled duration), the blend/clamp and the flag byte match; NaN inputs now take the same path as the x87 compares.
 * Emulated against the original (unicorn, 300 random descriptors, x87 and SSE2 builds): identical bytes (after moving the
 * blend to double temporaries: float-only math differed by 1 ulp). The old difftest mismatch (intensity 0 vs 0.6884) was not
 * reproduced with random descriptors; it is probably a harness input effect (uninitialised flash fields).
 *
 * @address 0x4578a0
 */
void player_effect_view::set_screen_flash(player_screen_flash *descriptor, float intensity_falloff, float duration_scale)
{
    player_effect * self = record;
    if ((self->flash.priority <= descriptor->priority ||
         (float)self->flash_ticks <= duration_scale * halo::game::k_ticks_per_second_f * descriptor->duration) &&
        screen_flash_pass[descriptor->type] != 0) {
        double blended;

        self->flash = *descriptor;
        {
            double scaled_duration = (double)(duration_scale * halo::game::k_ticks_per_second_f) * (double)descriptor->duration;

            self->flash.duration = (float)scaled_duration;
            self->flash_ticks = (int16_t)(int32_t)scaled_duration;
        }

        {
            double weight = descriptor->intensity;
            double maximum = halo::bit_cast<float>(descriptor->maximum_intensity);

            blended = (1.0 - weight) * (double)intensity_falloff + weight;
            if (blended < 0.0) {
                self->flash.intensity = 0.0f;
            } else if (blended > maximum) {
                self->flash.intensity = (float)maximum;
            } else {
                self->flash.intensity = (float)blended;
            }
            self->flags |= _player_effect_screen_flash_bit;
        }
    }
}

/**
 * FIXED (objdump 0x456980..0x4569ca): stack arg 1 is the flash descriptor (forwarded in EBX) and stack arg 2 the
 * intensity falloff (forwarded as the callee's second stack argument); the duration scale is always 1.0. Every
 * caller pushes (descriptor, 1.0f) with the player in EAX.
 *
 * @address 0x456980
 */
void player_effect_ref::set_screen_flash_for_player(player_screen_flash *descriptor, float intensity_falloff)
{
    datum_index player_index = datum;
    if (player_index != k_datum_index_none) {
        player *record = &((player *)halo::game::globals().player_data->data)[player_index & halo::k_slot_mask];

        if (record->local_player_index != -1) {
            halo::effects::player_effect_set_screen_flash(
                &player_effect_globals_pointer->players[record->local_player_index],
                descriptor, intensity_falloff, 1.0f);
        }
    }
}

/**
 * Walks every live player looking for one whose unit is a biped or vehicle currently holding
 * `weapon_object_index` as its current weapon. Returns 1 if found and that player is a local
 * player, -1 if found and it is not, or 0 if no player is holding that weapon.
 *
 * @address 0x453a10
 */
int32_t player_effect_ref::locality_for_object(datum_index weapon_object_index)
{
    data_iterator iterator;
    player *record;

    iterator.data = halo::game::globals().player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    record = (player *)halo::memory::data_iterator_next(&iterator);
    while (record != nullptr) {
        datum_index unit_index = record->unit;

        if (unit_index != k_datum_index_none) {
            int16_t index = (int16_t)unit_index;

            if (index >= 0 && index < halo::objects::globals().object_data->maximum_count) {
                object_header *header = &((object_header *)halo::objects::globals().object_data->data)[index];

                if (header->identifier != 0) {
                    int16_t salt = (int16_t)(unit_index >> 16);

                    if ((salt == 0 || header->identifier == salt) &&
                        ((1 << (header->type & 0x1f)) & _object_mask_unit) != 0 &&
                        header->data != 0) {
                        unit_data *held_unit =
                            &halo::objects::object_as<unit_object>(unit_index)->unit;
                        int16_t current_weapon = held_unit->current_weapon_index;
                        datum_index current_weapon_object = k_datum_index_none;

                        if (current_weapon != -1) {
                            current_weapon_object = held_unit->weapons[current_weapon];
                        }
                        if (weapon_object_index == current_weapon_object) {
                            return (record->local_player_index != -1) ? 1 : -1;
                        }
                    }
                }
            }
        }

        record = (player *)halo::memory::data_iterator_next(&iterator);
    }
    return 0;
}

}

namespace halo::effects {

void player_effect_set_camera_shake(player_effect *self, player_camera_shake *descriptor, float intensity_falloff, float duration_scale)
{
    halo::effects::player_effect_view(self).set_camera_shake(descriptor, intensity_falloff, duration_scale);
}

void player_effect_set_screen_flash(player_effect *self, player_screen_flash *descriptor, float intensity_falloff, float duration_scale)
{
    halo::effects::player_effect_view(self).set_screen_flash(descriptor, intensity_falloff, duration_scale);
}

void player_effect_apply_at_object(uint32_t tag_reference, int16_t local_player_index, real_point3d *origin)
{
    halo::effects::player_effect_ref::apply_at_object(tag_reference, local_player_index, origin);
}

void player_effect_apply_continuous_damage(uint32_t tag_reference, int16_t local_player_index, float distance)
{
    halo::effects::player_effect_ref::apply_continuous_damage(tag_reference, local_player_index, distance);
}

void player_effect_apply_generic_damage_feedback(datum_index player_index, float fraction)
{
    halo::effects::player_effect_ref(player_index).apply_generic_damage_feedback(fraction);
}

void player_effect_build_screen_flash(uint32_t *out, int16_t local_player_index)
{
    halo::effects::player_effect_ref::build_screen_flash(out, local_player_index);
}

void player_effect_clear_dead_players()
{
    halo::effects::player_effect_ref::clear_dead_players();
}

void player_effect_fade_damage_indicators(int16_t local_player_index, uint32_t *out_previous_indicators)
{
    halo::effects::player_effect_ref::fade_damage_indicators(local_player_index, out_previous_indicators);
}

void player_effect_mark_damage_direction(datum_index player_index, const damage_data *dd, const real_vector3d *direction, float random_blend, float damage_amount)
{
    halo::effects::player_effect_ref(player_index).mark_damage_direction(dd, direction, random_blend, damage_amount);
}

void player_effect_mark_damage_direction_dispatch(void **context)
{
    halo::effects::player_effect_ref::mark_damage_direction_dispatch(context);
}

void player_effect_random_shake_offset(real_matrix4x3 *out, real magnitude, real angle)
{
    halo::effects::player_effect_ref::random_shake_offset(out, magnitude, angle);
}

void player_effect_send_network_update(datum_index player_handle, const real_vector3d *direction, const damage_data *dd, float random_blend, float damage_amount)
{
    halo::effects::player_effect_ref(player_handle).send_network_update(direction, dd, random_blend, damage_amount);
}

void player_effect_set_screen_flash_for_player(datum_index player_index, player_screen_flash *descriptor, float intensity_falloff)
{
    halo::effects::player_effect_ref(player_index).set_screen_flash_for_player(descriptor, intensity_falloff);
}

int32_t player_weapon_locality_for_object(datum_index weapon_object_index)
{
    return halo::effects::player_effect_ref::locality_for_object(weapon_object_index);
}

}
