/**
 * @file src/sound/effects_objects.cpp
 * Lifetime of the global EAX sound effects object.
 */

#include "internal/state.hpp"

namespace halo::sound {

namespace effects {

static int32_t select_mode(int16_t channel_index, directsound_channel *channel)
{
    int32_t mode = -1;

    if (directsound_eax_enabled && directsound_eax_available) {
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        }

        global_sound_effect_object = (sound_effect_object *)malloc(0xe8);
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable = &sound_eax3_vtable;
            global_sound_effect_object->supported_properties = 0;
            if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, channel, channel_index) != 0) {
                mode = 2;
                global_sound_effect_object->mode = 2;
                return mode;
            }
            if (global_sound_effect_object != 0) {
                global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
            }
        }

        global_sound_effect_object = (sound_effect_object *)malloc(0xe8);
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable = &sound_eax2_vtable;
            global_sound_effect_object->supported_properties = 0;
            if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, channel, channel_index) != 0) {
                mode = 1;
                global_sound_effect_object->mode = 1;
                return mode;
            }
            if (global_sound_effect_object != 0) {
                global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
            }
        }

        global_sound_effect_object = (sound_effect_object *)malloc(0x1c);
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable = &sound_eax1_vtable;
            global_sound_effect_object->supported_properties = 0;
            if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, channel, channel_index) != 0) {
                mode = 0;
                global_sound_effect_object->supported_properties = 0;
                return mode;
            }
        }
    }

    mode = -1;
    sound_effect_object_state = 2;
    return mode;
}

int32_t detect_mode(int16_t channel_index, directsound_channel *channel)
{
    char mode_name[32];
    char message[64];
    const char *format;
    int32_t mode = select_mode(channel_index, channel);

    switch (mode) {
        case 0:  format = "SOUND_EFFECT_OBJECT_EAX1"; break;
        case 1:  format = "SOUND_EFFECT_OBJECT_EAX2"; break;
        case 2:  format = "SOUND_EFFECT_OBJECT_EAX3"; break;
        case 3:  format = "SOUND_EFFECT_OBJECT_DIRECTX"; break;
        case -1: format = "SOUND_EFFECT_OBJECT_NONE"; break;
        default:
            mode = -1;
            format = "SOUND_EFFECT_OBJECT_NONE";
            break;
    }
    sprintf(mode_name, format);
    sprintf(message, "Sound effect mode set to %s", mode_name);
    return mode;
}

void shutdown(void)
{
    if (global_sound_effect_object != 0) {
        global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        free(global_sound_effect_object);
        global_sound_effect_object = 0;
        sound_effect_object_state = 0;
    }
}

int32_t initialize_channel(int16_t channel_index)
{
    if (global_sound_effect_object != 0) {
        return global_sound_effect_object->vtable->initialize_channel(global_sound_effect_object, channel_index);
    }
    return 0;
}

int apply_all_channels(void)
{
    int16_t i;

    for (i = 0; i < directsound_channel_count; i++) {
        if (directsound_channels[i].type_flags & _sound_channel_3d_bit) {
            global_sound_effect_object->vtable->initialize_channel(global_sound_effect_object, i);
        }
    }
    return 1;
}

void reinitialize(int enable)
{
    if (!directsound_initialized) {
        return;
    }

    if (enable == 0) {
        if (global_sound_effect_object == 0) {
            return;
        }
        effects::shutdown();
        sound_effect_object_state = 2;
        return;
    }

    if (global_sound_effect_object != 0) {
        int16_t mode = global_sound_effect_object->mode;
        if (mode == 0 || mode == 1 || mode == 2) {
            return;
        }
    }

    sound_effect_object_state = 0;
    if (global_sound_effect_object != 0) {
        global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        }
    }

    global_sound_effect_object = (sound_effect_object *)malloc(0xe8);
    if (global_sound_effect_object != 0) {
        global_sound_effect_object->vtable = &sound_eax3_vtable;
        global_sound_effect_object->supported_properties = 0;
        if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, directsound_channels, 0) != 0) {
            effects::apply_all_channels();
            return;
        }
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        }
    }

    global_sound_effect_object = (sound_effect_object *)malloc(0xe8);
    if (global_sound_effect_object != 0) {
        global_sound_effect_object->vtable = &sound_eax2_vtable;
        global_sound_effect_object->supported_properties = 0;
        if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, directsound_channels, 0) != 0) {
            effects::apply_all_channels();
            return;
        }
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        }
    }

    global_sound_effect_object = (sound_effect_object *)malloc(0x1c);
    if (global_sound_effect_object == 0) {
        return;
    }
    global_sound_effect_object->vtable = &sound_eax1_vtable;
    global_sound_effect_object->supported_properties = 0;
    if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, directsound_channels, 0) != 0) {
        effects::apply_all_channels();
    }
}

}  // namespace effects


}  // namespace halo::sound
