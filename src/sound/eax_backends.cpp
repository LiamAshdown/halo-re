/**
 * @file src/sound/eax_backends.cpp
 * EAX 1, 2 and 3 effect backends.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "halo/sound/directsound.hpp"
#include "internal/state.hpp"
#include "halo/sound/api.hpp"
#include "halo/core/bit_cast.hpp"

constexpr uint32_t SOUND_EAX20_LISTENER_REQUIRED = 0x000027fcu;
constexpr uint32_t SOUND_EAX20_CHANNEL_REQUIRED = 0x000007dcu;
constexpr uint32_t SOUND_EAX30_LISTENER_REQUIRED = 0x0140db70u;
constexpr uint32_t SOUND_EAX30_CHANNEL_REQUIRED = 0x00119fe0u;

/** Deferred-apply flag OR-ed into a property id, -10000 millibels as raw bits, and float bit patterns used as property defaults. */
constexpr uint32_t k_eax_deferred = 0x80000000u;
constexpr uint32_t k_eax_silent_millibels = 0xffffd8f0u;
constexpr uint32_t k_float_bits_0_1 = 0x3dcccccdu;
constexpr uint32_t k_float_bits_0_2 = 0x3e4ccccdu;
constexpr uint32_t k_float_bits_1000 = 0x447a0000u;

namespace halo::sound {

namespace {

typedef struct sound_eax20_default_property {
    uint32_t bit;
    uint32_t id;
    uint32_t default_bits;
} sound_eax20_default_property;

typedef struct sound_eax20_query { uint32_t id; uint32_t bit; } sound_eax20_query;

typedef struct sound_eax20_listener_field { uint32_t bit; uint32_t id; int32_t offset; int32_t mode; } sound_eax20_listener_field;

typedef struct sound_eax30_default_property { uint32_t bit; uint32_t id; uint32_t default_bits; } sound_eax30_default_property;

typedef struct sound_eax30_query { uint32_t id; uint32_t bit; } sound_eax30_query;

typedef struct sound_eax30_listener_field { uint32_t bit; uint32_t id; int32_t offset; int32_t mode; } sound_eax30_listener_field;

/** Default listener properties applied to the EAX 2.0 environment. */
const sound_eax20_default_property k_listener_defaults[11] = {
    {1u << 2, 2,  k_eax_silent_millibels},
    {1u << 3, 3,  k_eax_silent_millibels},
    {1u << 4, 4,  0u},
    {1u << 5, 5,  k_float_bits_0_1},
    {1u << 6, 6,  k_float_bits_0_1},
    {1u << 7, 7,  k_eax_silent_millibels},
    {1u << 8, 8,  0u},
    {1u << 9, 9,  k_eax_silent_millibels},
    {1u << 10, 10, 0u},
    {1u << 13, 13, 0u},
    {1u << 15, 15, 0u},
};

/** Default per-channel properties applied to EAX 2.0 buffers. */
const sound_eax20_default_property k_channel_defaults[9] = {
    {1u << 2, 2,  k_eax_silent_millibels},
    {1u << 3, 3,  k_eax_silent_millibels},
    {1u << 4, 4,  k_eax_silent_millibels},
    {1u << 5, 5,  k_eax_silent_millibels},
    {1u << 6, 6,  0u},
    {1u << 7, 7,  0u},
    {1u << 8, 8,  0u},
    {1u << 9, 9,  0u},
    {1u << 10, 10, 0u},
};

/** Listener properties queried for support when an EAX backend initializes. */
const sound_eax20_query k_listener_queries[14] = {
    {2, 1u << 2}, {3, 1u << 3}, {4, 1u << 4}, {5, 1u << 5}, {6, 1u << 6},
    {7, 1u << 7}, {8, 1u << 8}, {9, 1u << 9}, {10, 1u << 10}, {13, 1u << 13},
    {8, 1u << 8}, {15, 1u << 15}, {15, 1u << 15}, {k_eax_deferred, 1u},
};

/** Buffer properties queried for support when an EAX backend initializes. */
const sound_eax20_query k_buffer_queries[8] = {
    {2, 1u << 2}, {3, 1u << 3}, {4, 1u << 4}, {6, 1u << 6},
    {7, 1u << 7}, {8, 1u << 8}, {9, 1u << 9}, {k_eax_deferred, 1u},
};

/** Mapping from SoundEnvironment fields to EAX listener properties. */
const sound_eax20_listener_field k_fields[11] = {
    {1u << 2, 2,  0x08, 1}, {1u << 3, 3,  0x0c, 1}, {1u << 4, 4,  0x10, 0},
    {1u << 5, 5,  0x14, 0}, {1u << 6, 6,  0x18, 0}, {1u << 7, 7,  0x1c, 2},
    {1u << 8, 8,  0x20, 0}, {1u << 9, 9,  0x24, 3}, {1u << 10, 10, 0x28, 0},
    {1u << 13, 13, 0x2c, 0}, {1u << 15, 15, -1,   0},
};

/** Converts one SoundEnvironment field to the EAX 2.0 property value. */
int32_t sound_eax20_convert_field(const uint8_t *environment, const sound_eax20_listener_field *field)
{
    float value;
    int32_t millibels;
    int32_t clamp_max;

    if (field->offset < 0) {
        return 0;
    }
    value = *(const float *)(environment + field->offset);
    if (field->mode == 0) {
        return *(const int32_t *)(environment + field->offset);
    }
    if (value == 0.0f) {
        return k_sound_minimum_volume;
    }
    clamp_max = field->mode == 1 ? 0 : (field->mode == 2 ? 1000 : 2000);
    millibels = (int32_t)(log10((double)value) * 2000.0 + (double)clamp_max);
    if (millibels < k_sound_minimum_volume) {
        return k_sound_minimum_volume;
    }
    return millibels > clamp_max ? clamp_max : millibels;
}

/** Default listener properties applied to the EAX 2.0 environment. */
const sound_eax30_default_property k_listener_defaults_sound_eax30_effect_shutdown[12] = {
    {1u << 5, 5,    k_eax_silent_millibels},
    {1u << 6, 6,    k_eax_silent_millibels},
    {1u << 24, 0x18, 0u},
    {1u << 8, 8,    k_float_bits_0_1},
    {1u << 9, 9,    k_float_bits_0_1},
    {1u << 11, 0x0b, k_eax_silent_millibels},
    {1u << 12, 0x0c, 0u},
    {1u << 14, 0x0e, k_eax_silent_millibels},
    {1u << 15, 0x0f, 0u},
    {1u << 4, 4,    0u},
    {1u << 25, 0x19, 0u},
    {1u << 4, 0x16, k_float_bits_1000},
};

/** Default per-channel properties applied to EAX 2.0 buffers. */
const sound_eax30_default_property k_channel_defaults_sound_eax30_effect_shutdown[9] = {
    {1u << 5, 5,    0u},
    {1u << 6, 6,    0u},
    {1u << 7, 7,    k_eax_silent_millibels},
    {1u << 8, 8,    k_eax_silent_millibels},
    {1u << 8, 0x14, 0u},
    {1u << 9, 9,    0u},
    {1u << 10, 0x0a, 0u},
    {1u << 11, 0x0b, 0u},
    {1u << 12, 0x0c, 0u},
};

/** Listener properties queried for support when an EAX backend initializes. */
const sound_eax30_query k_listener_queries_sound_eax30_effect_initialize[13] = {
    {5, 1u << 5}, {6, 1u << 6}, {0x18, 1u << 24}, {8, 1u << 8}, {9, 1u << 9},
    {0x0b, 1u << 11}, {0x0c, 1u << 12}, {0x0e, 1u << 14}, {0x0f, 1u << 15},
    {4, 1u << 4}, {0x19, 1u << 25}, {0x16, 1u << 22}, {k_eax_deferred, 1u},
};

/** Buffer properties queried for support when an EAX backend initializes. */
const sound_eax30_query k_buffer_queries_sound_eax30_effect_initialize[12] = {
    {5, 1u << 5}, {6, 1u << 6}, {7, 1u << 7}, {8, 1u << 8}, {0x14, 1u << 20},
    {9, 1u << 9}, {0x0a, 1u << 10}, {0x0b, 1u << 11}, {0x0c, 1u << 12}, {0x0f, 1u << 15},
    {0x10, 1u << 16}, {k_eax_deferred, 1u},
};

/** Mapping from SoundEnvironment fields to EAX listener properties. */
const sound_eax30_listener_field k_fields_sound_eax30_effect_apply_listener[12] = {
    {1u << 5, 5,    0x08, 1},
    {1u << 6, 6,    0x0c, 1},
    {1u << 24, 0x18, 0x10, 0},
    {1u << 8, 8,    0x14, 0},
    {1u << 9, 9,    0x18, 0},
    {1u << 11, 0x0b, 0x1c, 2},
    {1u << 12, 0x0c, 0x20, 0},
    {1u << 14, 0x0e, 0x24, 3},
    {1u << 15, 0x0f, 0x28, 0},
    {1u << 4, 4,    0x2c, 0},
    {1u << 25, 0x19, -1,   4},
    {1u << 4, 0x16, 0x34, 5},
};

/** Converts one SoundEnvironment field to the EAX 3.0 property value. */
int32_t sound_eax30_convert_field(const uint8_t *environment, const sound_eax30_listener_field *field)
{
    float value;
    int32_t millibels;
    int32_t clamp_max;

    switch (field->mode) {
        case 0:
            return *(const int32_t *)(environment + field->offset);
        case 4:
            return 0;
        case 5: {
            float scale = gain::reverb_size_scale(*(const float *)(environment + field->offset));
            return halo::bit_cast<int32_t>(scale);
        }
        default:
            break;
    }

    value = *(const float *)(environment + field->offset);
    if (value == 0.0f) {
        return k_sound_minimum_volume;
    }
    clamp_max = field->mode == 1 ? 0 : (field->mode == 2 ? 1000 : 2000);
    millibels = (int32_t)(log10((double)value) * 2000.0 + (double)clamp_max);
    if (millibels < k_sound_minimum_volume) {
        return k_sound_minimum_volume;
    }
    return millibels > clamp_max ? clamp_max : millibels;
}

}  // namespace

void Eax1Backend::shutdown(sound_effect_object * this_object)
{
    void *property_set = this_object->property_set;

    if (property_set != 0) {
        ((uint32_t (__stdcall *)(void *))(*(void ***)property_set)[halo::sound::dsound_slot::release])(property_set);
        this_object->property_set = 0;
    }
}

int32_t Eax1Backend::initialize(sound_effect_object * this_object, directsound_channel * channel, int32_t unused)
{
    uint32_t type_support;
    void **vtable;

    (void)unused;
    this_object->property_set = 0;
    this_object->supported_properties = 0;
    this_object->listener_supported = 0;
    this_object->channel_supported = 0;

    if (channel == 0 || channel->buffer_3d == 0) {
        return 0;
    }

    vtable = *(void ***)channel->buffer_3d;
    if (((int32_t (__stdcall *)(void *, const uint8_t *, void **))vtable[0])(channel->buffer_3d, sound_eax_property_set_guid,
            &this_object->property_set) < 0) {
        this_object->property_set = 0;
    } else {
        void *property_set = this_object->property_set;
        sound_query_support_fn query_support =
            (sound_query_support_fn)(*(void ***)property_set)[halo::sound::dsound_slot::ks_query_support];

        type_support = 0;
        if (query_support(property_set, sound_eax_listener_property_guid, 2, &type_support) >= 0 &&
            (type_support & 3) == 3) {
            this_object->supported_properties |= 0x04;
        }
        if (query_support(property_set, sound_eax_listener_property_guid, 3, &type_support) >= 0 &&
            (type_support & 3) == 3) {
            this_object->supported_properties |= 0x08;
        }
        if (query_support(property_set, sound_eax_listener_property_guid, 4, &type_support) >= 0 &&
            (type_support & 3) == 3) {
            this_object->supported_properties |= 0x10;
        }
    }

    this_object->listener_supported = this_object->supported_properties != 0;
    return this_object->listener_supported;
}

void Eax1Backend::apply_listener(sound_effect_object * this_object, const SoundEnvironment * environment)
{
    float value;

    if ((this_object->supported_properties & 0x08) != 0) {
        value = environment->decay_time;
        ((sound_property_set_fn)(*(void ***)this_object->property_set)[halo::sound::dsound_slot::ks_set])(this_object->property_set,
            sound_eax_listener_property_guid, 3, 0, 0, &value, 4);
    }
    if ((this_object->supported_properties & 0x10) != 0) {
        value = environment->decay_hf_ratio * 0.4761905f;
        value = value + value;
        ((sound_property_set_fn)(*(void ***)this_object->property_set)[halo::sound::dsound_slot::ks_set])(this_object->property_set,
            sound_eax_listener_property_guid, 4, 0, 0, &value, 4);
    }
    if ((this_object->supported_properties & 0x04) != 0) {
        value = environment->reverb_intensity;
        ((sound_property_set_fn)(*(void ***)this_object->property_set)[halo::sound::dsound_slot::ks_set])(this_object->property_set,
            sound_eax_listener_property_guid, 2, 0, 0, &value, 4);
    }
    directsound_deferred_dirty = 1;
}

void Eax1Backend::set_environment_index(sound_effect_object * this_object, int32_t environment)
{
    sound_property_set_fn set = (sound_property_set_fn)(*(void ***)this_object->property_set)[halo::sound::dsound_slot::ks_set];
    int32_t result = set(this_object->property_set, sound_eax_listener_property_guid, 1, 0, 0, &environment, 4);

    if (result >= 0) {
        ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[halo::sound::dsound_slot::lst_commit_deferred_settings])(directsound_listener);
    }
}

void Eax1Backend::set_room_gain(sound_effect_object * this_object, float gain)
{
    sound_property_set_fn set = (sound_property_set_fn)(*(void ***)this_object->property_set)[halo::sound::dsound_slot::ks_set];
    int32_t result = set(this_object->property_set, sound_eax_listener_property_guid, 2, 0, 0, &gain, 4);

    if (result >= 0) {
        ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[halo::sound::dsound_slot::lst_commit_deferred_settings])(directsound_listener);
    }
}

int32_t EffectsBackend::listener_supported(sound_effect_object * this_object)
{
    return this_object->listener_supported;
}

int32_t EffectsBackend::channel_supported(sound_effect_object * this_object)
{
    return this_object->channel_supported;
}

void Eax2Backend::shutdown(sound_effect_object * this_object_base)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    int32_t i;
    void *property_set;
    sound_property_set_fn set;
    uint32_t default_bits;

    property_set = this_object->base.property_set;
    if (property_set != 0) {
        void *listener_set = this_object->channel_property_sets[0];

        set = (sound_property_set_fn)(*(void ***)listener_set)[4];
        for (i = 0; i < 11; i++) {
            if (this_object->base.supported_properties & k_listener_defaults[i].bit) {
                default_bits = k_listener_defaults[i].default_bits;
                set(listener_set, sound_eax20_listener_property_guid, k_listener_defaults[i].id, 0, 0,
                    &default_bits, 4);
            }
        }
        ((uint32_t (__stdcall *)(void *))(*(void ***)property_set)[halo::sound::dsound_slot::release])(property_set);
        this_object->base.property_set = 0;
    }

    for (i = 0; i < k_maximum_eax_channels; i++) {
        property_set = this_object->channel_property_sets[i];
        if (property_set != 0) {
            int32_t j;
            set = (sound_property_set_fn)(*(void ***)property_set)[4];
            for (j = 0; j < 9; j++) {
                if (this_object->base.supported_properties & k_channel_defaults[j].bit) {
                    default_bits = k_channel_defaults[j].default_bits;
                    set(property_set, sound_eax20_buffer_property_guid, k_channel_defaults[j].id, 0, 0,
                        &default_bits, 4);
                }
            }
            ((uint32_t (__stdcall *)(void *))(*(void ***)property_set)[halo::sound::dsound_slot::release])(property_set);
        }
        this_object->channel_property_sets[i] = 0;
    }
}

int32_t Eax2Backend::initialize(sound_effect_object * this_object_base, directsound_channel * listener, int32_t unused)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    (void)unused;
    int32_t i;
    void *property_set;
    sound_query_support_fn query;
    uint32_t out_value;
    uint32_t supported;

    this_object->base.property_set = 0;
    this_object->base.supported_properties = 0;
    this_object->base.listener_supported = 0;
    this_object->base.channel_supported = 0;

    if (listener == 0 || listener->buffer_3d == 0) {
        return 0;
    }

    for (i = 0; i < k_maximum_eax_channels; i++) {
        this_object->channel_property_sets[i] = 0;
    }

    if (((int32_t (__stdcall *)(void *, const uint8_t *, void **))(*(void ***)listener->buffer_3d)[halo::sound::dsound_slot::query_interface])(listener->buffer_3d,
            sound_eax_property_set_guid, &this_object->base.property_set) >= 0) {
        property_set = this_object->base.property_set;
        query = (sound_query_support_fn)(*(void ***)property_set)[5];

        for (i = 0; i < 14; i++) {
            if (query(property_set, sound_eax20_listener_property_guid, k_listener_queries[i].id, &out_value) >= 0 &&
                (out_value & 3) == 3) {
                this_object->base.supported_properties |= k_listener_queries[i].bit;
            }
        }
        for (i = 0; i < 8; i++) {
            if (query(property_set, sound_eax20_buffer_property_guid, k_buffer_queries[i].id, &out_value) >= 0 &&
                (out_value & 3) == 3) {
                this_object->base.supported_properties |= k_buffer_queries[i].bit;
            }
        }
    } else {
        this_object->base.property_set = 0;
    }

    supported = this_object->base.supported_properties;
    this_object->base.listener_supported = (supported & SOUND_EAX20_LISTENER_REQUIRED) == SOUND_EAX20_LISTENER_REQUIRED;
    this_object->base.channel_supported = (supported & SOUND_EAX20_CHANNEL_REQUIRED) == SOUND_EAX20_CHANNEL_REQUIRED;

    return this_object->base.listener_supported && this_object->base.channel_supported;
}

int32_t EaxBackend::initialize_channel(sound_effect_object * this_object_base, int32_t channel_index)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    directsound_channel *channel = &directsound_channels[channel_index];
    void **property_set_out = &this_object->channel_property_sets[channel_index];
    void **vtable = *(void ***)channel->buffer_3d;
    int32_t (__stdcall *query_interface)(void *, const uint8_t *, void **) =
        (int32_t (__stdcall *)(void *, const uint8_t *, void **))vtable[0];
    int32_t result = query_interface(channel->buffer_3d, sound_eax_property_set_guid, property_set_out);

    return (result >= 0 && *property_set_out != 0) ? 1 : 0;
}

void Eax2Backend::apply_channel(sound_effect_object * this_object_base, int32_t channel_index)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    directsound_channel *channel_state;
    void *property_set;
    sound_property_set_fn set;
    uint32_t supported;
    uint8_t deferred;
    int32_t underwater_direct, underwater_room;
    int32_t obstruction_at_1000, obstruction_at_0;
    int32_t occlusion_millibels, self_obstruction_millibels;
    int32_t unused_id6 = 0, unused_id8 = 0;
    int32_t id10_value = k_float_bits_0_2;

    underwater_direct = 0;
    underwater_room = 0;
    obstruction_at_1000 = k_sound_minimum_volume;
    obstruction_at_0 = k_sound_minimum_volume;
    occlusion_millibels = 0;
    self_obstruction_millibels = 0;

    channel_state = &directsound_channels[channel_index];

    if (channel_state->spatialized) {
        float eax_value = channel_state->eax_value;

        if (channel_state->underwater) {
            underwater_direct = gain::to_directsound_volume(sound_eax20_underwater_direct_gain, 1000);
            underwater_room = gain::to_directsound_volume(sound_eax20_underwater_direct_gain, 0);
        }
        obstruction_at_1000 = gain::to_directsound_volume(1.0f - eax_value, 1000);
        obstruction_at_0 = gain::to_directsound_volume(1.0f - eax_value, 0);
        occlusion_millibels = gain::to_millibels(channel_state->occlusion);
        self_obstruction_millibels = gain::to_millibels(channel_state->obstruction);
    }

    supported = this_object->base.supported_properties;
    deferred = (supported & 1) != 0;
    property_set = this_object->channel_property_sets[channel_index];
    set = (sound_property_set_fn)(*(void ***)property_set)[4];

    {
        struct { uint32_t bit; uint32_t id; int32_t *value; } fields[9] = {
            {1u << 2, 2,  &underwater_direct},
            {1u << 3, 3,  &underwater_room},
            {1u << 4, 4,  &obstruction_at_1000},
            {1u << 5, 5,  &obstruction_at_0},
            {1u << 6, 6,  &unused_id6},
            {1u << 7, 7,  &occlusion_millibels},
            {1u << 8, 8,  &unused_id8},
            {1u << 9, 9,  &self_obstruction_millibels},
            {1u << 10, 10, &id10_value},
        };
        int32_t i;

        for (i = 0; i < 9; i++) {
            if (supported & fields[i].bit) {
                uint32_t id = deferred ? (fields[i].id | k_eax_deferred) : fields[i].id;
                set(property_set, sound_eax20_buffer_property_guid, id, 0, 0, fields[i].value, 4);
            }
        }
    }

    directsound_deferred_dirty = 1;
}

void Eax2Backend::apply_listener(sound_effect_object * this_object_base, const SoundEnvironment * environment)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    int32_t i;
    void *property_set;
    sound_property_set_fn set;
    int32_t value;
    uint32_t id;
    uint8_t deferred;

    property_set = this_object->channel_property_sets[0];
    set = (sound_property_set_fn)(*(void ***)property_set)[4];
    deferred = (this_object->base.supported_properties & 1) != 0;

    for (i = 0; i < 11; i++) {
        if (this_object->base.supported_properties & k_fields[i].bit) {
            value = sound_eax20_convert_field((const uint8_t *)environment, &k_fields[i]);
            id = deferred ? (k_fields[i].id | k_eax_deferred) : k_fields[i].id;
            set(property_set, sound_eax20_listener_property_guid, id, 0, 0, &value, 4);
        }
    }

    directsound_deferred_dirty = 1;
}

void Eax2Backend::set_environment_index(sound_effect_object * this_object_base, int32_t environment)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    void *property_set = this_object->channel_property_sets[0];
    sound_property_set_fn set = (sound_property_set_fn)(*(void ***)property_set)[halo::sound::dsound_slot::ks_set];

    set(property_set, sound_eax20_listener_property_guid, 0xb, 0, 0, &environment, 4);
    ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[halo::sound::dsound_slot::lst_commit_deferred_settings])(directsound_listener);
}

void Eax2Backend::set_room_gain(sound_effect_object * this_object_base, float gain)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    void *property_set = this_object->channel_property_sets[0];
    sound_property_set_fn set;
    int32_t value;

    if (gain != 0.0f) {
        value = (int32_t)((double)gain * 12000.0 - 10000.0);
    } else {
        value = k_sound_minimum_volume;
    }

    set = (sound_property_set_fn)(*(void ***)property_set)[halo::sound::dsound_slot::ks_set];
    set(property_set, sound_eax20_listener_property_guid, 9, 0, 0, &value, 4);
    ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[halo::sound::dsound_slot::lst_commit_deferred_settings])(directsound_listener);
}

void Eax3Backend::shutdown(sound_effect_object * this_object_base)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    int32_t i;
    void *property_set;
    sound_property_set_fn set;
    uint32_t default_bits;

    property_set = this_object->base.property_set;
    if (property_set != 0) {
        void *listener_set = this_object->channel_property_sets[0];

        set = (sound_property_set_fn)(*(void ***)listener_set)[4];
        for (i = 0; i < 12; i++) {
            if (this_object->base.supported_properties & k_listener_defaults_sound_eax30_effect_shutdown[i].bit) {
                default_bits = k_listener_defaults_sound_eax30_effect_shutdown[i].default_bits;
                set(listener_set, sound_eax30_listener_property_guid, k_listener_defaults_sound_eax30_effect_shutdown[i].id, 0, 0,
                    &default_bits, 4);
            }
        }
        ((uint32_t (__stdcall *)(void *))(*(void ***)property_set)[halo::sound::dsound_slot::release])(property_set);
        this_object->base.property_set = 0;
    }

    for (i = 0; i < k_maximum_eax_channels; i++) {
        property_set = this_object->channel_property_sets[i];
        if (property_set != 0) {
            int32_t j;
            set = (sound_property_set_fn)(*(void ***)property_set)[4];
            for (j = 0; j < 9; j++) {
                if (this_object->base.supported_properties & k_channel_defaults_sound_eax30_effect_shutdown[j].bit) {
                    default_bits = k_channel_defaults_sound_eax30_effect_shutdown[j].default_bits;
                    set(property_set, sound_eax30_buffer_property_guid, k_channel_defaults_sound_eax30_effect_shutdown[j].id, 0, 0,
                        &default_bits, 4);
                }
            }
            ((uint32_t (__stdcall *)(void *))(*(void ***)property_set)[halo::sound::dsound_slot::release])(property_set);
        }
        this_object->channel_property_sets[i] = 0;
    }
}

int32_t Eax3Backend::initialize(sound_effect_object * this_object_base, directsound_channel * listener, int32_t unused)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    (void)unused;
    int32_t i;
    void *property_set;
    sound_query_support_fn query;
    uint32_t out_value;
    uint32_t supported;

    this_object->base.property_set = 0;
    this_object->base.supported_properties = 0;
    this_object->base.listener_supported = 0;
    this_object->base.channel_supported = 0;

    if (listener == 0 || listener->buffer_3d == 0) {
        return 0;
    }

    for (i = 0; i < k_maximum_eax_channels; i++) {
        this_object->channel_property_sets[i] = 0;
    }

    if (((int32_t (__stdcall *)(void *, const uint8_t *, void **))(*(void ***)listener->buffer_3d)[halo::sound::dsound_slot::query_interface])(listener->buffer_3d,
            sound_eax_property_set_guid, &this_object->base.property_set) >= 0) {
        property_set = this_object->base.property_set;
        query = (sound_query_support_fn)(*(void ***)property_set)[5];

        for (i = 0; i < 13; i++) {
            if (query(property_set, sound_eax30_listener_property_guid, k_listener_queries_sound_eax30_effect_initialize[i].id, &out_value) >= 0 &&
                (out_value & 3) == 3) {
                this_object->base.supported_properties |= k_listener_queries_sound_eax30_effect_initialize[i].bit;
            }
        }
        for (i = 0; i < 12; i++) {
            if (query(property_set, sound_eax30_buffer_property_guid, k_buffer_queries_sound_eax30_effect_initialize[i].id, &out_value) >= 0 &&
                (out_value & 3) == 3) {
                this_object->base.supported_properties |= k_buffer_queries_sound_eax30_effect_initialize[i].bit;
            }
        }
    } else {
        this_object->base.property_set = 0;
    }

    supported = this_object->base.supported_properties;
    this_object->base.listener_supported = (supported & SOUND_EAX30_LISTENER_REQUIRED) == SOUND_EAX30_LISTENER_REQUIRED;
    this_object->base.channel_supported = (supported & SOUND_EAX30_CHANNEL_REQUIRED) == SOUND_EAX30_CHANNEL_REQUIRED;

    return this_object->base.listener_supported || this_object->base.channel_supported;
}

void Eax3Backend::apply_channel(sound_effect_object * this_object_base, int32_t channel_index)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    directsound_channel *channel_state;
    void *property_set;
    sound_property_set_fn set;
    uint32_t supported;
    uint8_t deferred;
    int32_t obstruction_at_1000, obstruction_at_0, occlusion_millibels, self_obstruction_millibels;
    int32_t underwater_direct, underwater_room;
    int32_t unused_id20 = 0, id10_value = 0;
    int32_t id12_value = k_float_bits_0_2;

    obstruction_at_1000 = k_sound_minimum_volume;
    obstruction_at_0 = k_sound_minimum_volume;
    occlusion_millibels = 0;
    self_obstruction_millibels = 0;
    underwater_direct = 0;
    underwater_room = 0;

    property_set = this_object->channel_property_sets[channel_index];
    if (property_set == 0) {
        return;
    }

    channel_state = &directsound_channels[channel_index];

    if (channel_state->spatialized) {
        float eax_value = channel_state->eax_value;
        if (channel_state->underwater) {
            underwater_room = gain::to_directsound_volume(sound_underwater_direct_gain, 0);
            underwater_direct = gain::to_directsound_volume(sound_underwater_direct_gain, 1000);
        }
        obstruction_at_1000 = gain::to_directsound_volume(1.0f - eax_value, 1000);
        obstruction_at_0 = gain::to_directsound_volume(1.0f - eax_value, 0);
        occlusion_millibels = gain::to_millibels(channel_state->occlusion);
        self_obstruction_millibels = gain::to_millibels(channel_state->obstruction);
    }

    supported = this_object->base.supported_properties;
    deferred = (supported & 1) != 0;
    set = (sound_property_set_fn)(*(void ***)property_set)[4];

    {
        struct { uint32_t bit; uint32_t id; int32_t *value; } fields[9] = {
            {1u << 5, 5,    &underwater_direct},
            {1u << 6, 6,    &underwater_room},
            {1u << 7, 7,    &obstruction_at_1000},
            {1u << 8, 8,    &obstruction_at_0},
            {1u << 20, 0x14, &unused_id20},
            {1u << 9, 9,    &occlusion_millibels},
            {1u << 10, 0x0a, &id10_value},
            {1u << 11, 0x0b, &self_obstruction_millibels},
            {1u << 12, 0x0c, &id12_value},
        };
        int32_t i;
        for (i = 0; i < 9; i++) {
            if (supported & fields[i].bit) {
                uint32_t id = deferred ? (fields[i].id | k_eax_deferred) : fields[i].id;
                set(property_set, sound_eax30_buffer_property_guid, id, 0, 0, fields[i].value, 4);
            }
        }
    }

    directsound_deferred_dirty = 1;
}

void Eax3Backend::apply_listener(sound_effect_object * this_object_base, const SoundEnvironment * environment)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    int32_t i;
    void *property_set;
    sound_property_set_fn set;
    int32_t value;
    uint32_t id;
    uint8_t deferred;

    property_set = this_object->channel_property_sets[0];
    set = (sound_property_set_fn)(*(void ***)property_set)[4];
    deferred = (this_object->base.supported_properties & 1) != 0;

    for (i = 0; i < 12; i++) {
        if (this_object->base.supported_properties & k_fields_sound_eax30_effect_apply_listener[i].bit) {
            value = sound_eax30_convert_field((const uint8_t *)environment, &k_fields_sound_eax30_effect_apply_listener[i]);
            id = deferred ? (k_fields_sound_eax30_effect_apply_listener[i].id | k_eax_deferred) : k_fields_sound_eax30_effect_apply_listener[i].id;
            set(property_set, sound_eax30_listener_property_guid, id, 0, 0, &value, 4);
        }
    }

    directsound_deferred_dirty = 1;
}

void Eax3Backend::set_environment_index(sound_effect_object * this_object_base, int32_t environment)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    void *property_set = this_object->channel_property_sets[0];
    sound_property_set_fn set = (sound_property_set_fn)(*(void ***)property_set)[halo::sound::dsound_slot::ks_set];

    set(property_set, sound_eax30_listener_property_guid, 2, 0, 0, &environment, 4);
    ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[halo::sound::dsound_slot::lst_commit_deferred_settings])(directsound_listener);
}

void Eax3Backend::set_room_gain(sound_effect_object * this_object_base, float gain)
{
    sound_eax_effect_object *this_object = reinterpret_cast<sound_eax_effect_object *>(this_object_base);
    void *property_set = this_object->channel_property_sets[0];
    sound_property_set_fn set;
    int32_t value;

    if (gain != 0.0f) {
        value = (int32_t)((double)gain * 12000.0 - 10000.0);
    } else {
        value = k_sound_minimum_volume;
    }

    set = (sound_property_set_fn)(*(void ***)property_set)[halo::sound::dsound_slot::ks_set];
    set(property_set, sound_eax30_listener_property_guid, 0xe, 0, 0, &value, 4);
    ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[halo::sound::dsound_slot::lst_commit_deferred_settings])(directsound_listener);
}

int32_t Eax1Backend::initialize_channel(sound_effect_object * this_object, int32_t channel_index)
{
    return 1;
}

int32_t Eax1Backend::channel_supported(sound_effect_object * this_object)
{
    return 0;
}

void Eax1Backend::apply_channel(sound_effect_object * this_object, int32_t channel_index)
{
}

namespace {

constinit Eax1Backend g_eax1_backend;
constinit Eax2Backend g_eax2_backend;
constinit Eax3Backend g_eax3_backend;

}  // namespace

/** Returns the EAX 1.0 backend. */
Eax1Backend &eax1_backend()
{
    return g_eax1_backend;
}

/** Returns the EAX 2.0 backend. */
Eax2Backend &eax2_backend()
{
    return g_eax2_backend;
}

/** Returns the EAX 3.0 backend. */
Eax3Backend &eax3_backend()
{
    return g_eax3_backend;
}

}  // namespace halo::sound
