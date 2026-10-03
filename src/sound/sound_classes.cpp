/**
 * @file src/sound/sound_classes.cpp
 * Sound classes and the master, music and effects gain sliders.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "internal/state.hpp"

namespace halo::sound {

namespace {

/** Sound class name fragments muted when the effects slider reaches zero. */
const char *const k_effects_class_substrings_mute[25] = {
    "sound_class_projectile_impact", "sound_class_projectile_detonation", "weapon_fire",
    "sound_class_weapon_ready", "sound_class_weapon_reload", "sound_class_weapon_empty",
    "sound_class_weapon_charge", "sound_class_weapon_overheat", "sound_class_weapon_idle",
    "sound_class_object_impacts", "sound_class_particle_impacts", "sound_class_slow_impacts",
    "sound_class_footstep", "sound_class_vehicle_impact", "sound_class_vehicle_engine",
    "sound_class_device_door", "sound_class_device_force_field", "sound_class_device_machinery",
    "sound_class_device_nature", "sound_class_device_computers", "sound_class_ambient_nature",
    "sound_class_ambient_machinery", "sound_class_ambient_computer", "sound_class_player_hurt",
    "sound_class_game_event"
};

/** Sound class name fragments unmuted when the effects slider rises from zero. */
const char *const k_effects_class_substrings_unmute[25] = {
    "sound_class_projectile_impact", "sound_class_projectile_detonation", "weapon_fire",
    "sound_class_weapon_ready", "sound_class_weapon_reload", "sound_class_weapon_empty",
    "sound_class_weapon_charge", "sound_class_weapon_overheat", "sound_class_weapon_idle",
    "sound_class_object_impacts", "sound_class_particle_impacts", "sound_class_slow_impacts",
    "sound_class_footstep", "sound_class_vehicle_impact", "sound_class_vehicle_engine",
    "sound_class_device_door", "sound_class_device_force_field", "sound_class_device_machinery",
    "sound_class_device_nature", "sound_class_device_computers", "sound_class_ambient_nature",
    "sound_class_ambient_machinery", "sound_class_ambient_computers", "sound_class_player_hurt",
    "sound_class_game_event"
};

/** Sets the muted flag of every sound class whose name contains one of the given fragments. */
void sound_set_effects_class_muted(const char *const *substrings, int32_t count, uint8_t muted)
{
    int32_t s;

    for (s = 0; s < count; s++) {
        int32_t i;
        for (i = 0; i < k_maximum_sound_classes; i++) {
            if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], substrings[s]) != 0) {
                sound_class_definitions[i].muted = muted;
            }
        }
    }
}

}  // namespace

namespace classes {

void update_gain_fade(int32_t ticks)
{
    int32_t i;

    if (ticks <= 0) {
        return;
    }

    for (i = 0; i < k_maximum_sound_classes; i++) {
        sound_class_gain *gain = &sound_class_gains[i];

        if (ticks < gain->fade_ticks) {
            int16_t original_ticks = gain->fade_ticks;
            gain->fade_ticks = (int16_t)(original_ticks - ticks);
            gain->current_gain = (gain->target_gain - gain->current_gain) *
                ((float)ticks / (float)original_ticks) + gain->current_gain;
        } else {
            gain->current_gain = gain->target_gain;
            gain->fade_ticks = 0;
        }
    }
}

void set_gain_by_name(char *name, float gain, int16_t ticks)
{
    int32_t i;

    for (i = 0; i < k_maximum_sound_classes; i++) {
        if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], name) != 0) {
            float clamped_gain;
            int16_t clamped_ticks;

            if (gain < 0.0f) {
                clamped_gain = 0.0f;
            } else if (gain > 1.0f) {
                clamped_gain = 1.0f;
            } else {
                clamped_gain = gain;
            }

            clamped_ticks = (ticks < 0) ? 0 : ticks;

            sound_class_gains[i].target_gain = clamped_gain;
            sound_class_gains[i].fade_ticks = clamped_ticks;
        }
    }
}

void set_muted_by_name(uint8_t enabled, char *name)
{
    int32_t i;

    for (i = 0; i < k_maximum_sound_classes; i++) {
        if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], name) != 0) {
            sound_class_definitions[i].muted = (enabled == 0);
        }
    }
}

void set_master_gain(float gain)
{
    if (gain == sound_master_gain) {
        return;
    }

    if (sound_master_gain > 0.0f && gain <= 0.0f) {
        instances::stop_all();
        sound_enabled = 0;
        sound_master_gain = (gain < 0.0f) ? gain : 0.0f;
        instances::update_active();
        return;
    }

    if (sound_master_gain == 0.0f && gain > 0.0f) {
        sound_enabled = 1;
        sound_master_gain = (gain > 1.0f) ? gain : 1.0f;
        instances::update_active();
        return;
    }

    sound_master_gain = gain;
    instances::update_active();
}

void set_music_gain(float gain)
{
    int32_t i;

    if (gain == sound_music_gain) {
        return;
    }

    if (sound_music_gain > 0.0f && gain <= 0.0f) {
        for (i = 0; i < k_maximum_sound_classes; i++) {
            if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], "music") != 0) {
                sound_class_definitions[i].muted = 1;
            }
        }
        sound_music_gain = 0.0f;
        instances::update_active();
        return;
    }

    if (sound_music_gain != 0.0f || gain <= 0.0f) {
        sound_music_gain = gain;
        instances::update_active();
        return;
    }

    for (i = 0; i < k_maximum_sound_classes; i++) {
        if (sound_class_names[i][0] != '\0' && strstr(sound_class_names[i], "music") != 0) {
            sound_class_definitions[i].muted = 0;
        }
    }

    sound_music_gain = (gain < 1.0f) ? gain : 1.0f;
    instances::update_active();
}

void set_effects_gain(float gain)
{
    if (gain == sound_effects_gain) {
        return;
    }

    if (sound_effects_gain > 0.0f && gain <= 0.0f) {
        sound_set_effects_class_muted(k_effects_class_substrings_mute, 25, 1);
        sound_effects_gain = 0.0f;
        instances::update_active();
        return;
    }

    if (sound_effects_gain != 0.0f || gain <= 0.0f) {
        sound_effects_gain = gain;
        instances::update_active();
        return;
    }

    sound_set_effects_class_muted(k_effects_class_substrings_unmute, 25, 0);

    sound_effects_gain = (gain < 1.0f) ? gain : 1.0f;
    instances::update_active();
}

float compute_gain(SoundClass_t sound_class)
{
    float base_gain;

    base_gain = sound_class_gains[sound_class].current_gain;

    if (sound_class == soundclass_scripted_dialog_player || sound_class == soundclass_scripted_dialog_other ||
        sound_class == soundclass_scripted_dialog_force_unspatialized) {
        return base_gain * sound_master_gain;
    }
    if (sound_class == soundclass_music) {
        return sound_music_gain * sound_ducking_gain * sound_master_gain * base_gain;
    }
    if (sound_class == soundclass_scripted_effect || sound_class == soundclass_unit_dialog) {
        return sound_ducking_gain * sound_master_gain * base_gain;
    }
    return sound_effects_gain * sound_ducking_gain * sound_master_gain * base_gain;
}

}  // namespace classes


}  // namespace halo::sound
