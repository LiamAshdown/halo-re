/**
 * @file src/sound/game_looping_sounds.cpp
 * Game-state looping sounds bound to objects, scripts and the BSP background sound.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "internal/state.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/objects/api.hpp"

namespace halo::sound {

namespace {

/** Current performance-counter time in milliseconds. */
int32_t game_sound_update_now_ms(void)
{
    large_integer counter;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    return (int32_t)((counter.quad_part * 1000) / performance_frequency);
}

}  // namespace

namespace game_looping {

void initialize(void)
{
    int32_t size = sizeof(game_sound_globals);
    game_sound_globals *globals = (game_sound_globals *)(game_state_base + game_state_cursor);

    game_looping_sound_data = (data_array *)game_state_new((char *)"object looping sounds", k_maximum_game_looping_sounds, sizeof(game_looping_sound));

    game_state_cursor = game_state_cursor + size;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    game_sound_globals_ptr = globals;
}

void revert_scripting_sounds(void)
{
    datum_index index = halo::memory::datum_next(-1, game_looping_sound_data);

    while (index != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)index];
        SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[self->definition_index & 0xffff].data;

        if (*(uint32_t *)&definition->runtime_scripting_sound == (uint32_t)index) {
            *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)k_datum_index_none;
        }

        index = halo::memory::datum_next((int16_t)index, game_looping_sound_data);
    }
}

void reconcile_scripting_state(void)
{
    tag_iterator iterator;
    datum_index index = halo::memory::datum_next(-1, game_looping_sound_data);
    datum_index tag_id;

    while (index != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)index];

        if ((self->flags & _game_looping_sound_scripted_bit) != 0) {
            SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[self->definition_index & 0xffff].data;

            if ((definition->flags & 0x02) == 0) {
                *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)index;
            } else {
                halo::memory::datum_delete(game_looping_sound_data, index);
            }
        }

        index = halo::memory::datum_next((int16_t)index, game_looping_sound_data);
    }

    iterator.next_index = 0;
    iterator.group_tag = 0x736e6421;

    tag_id = halo::cache::tag_iterator_next(&iterator);
    while (tag_id != (datum_index)0xffffffff) {
        Sound *sound_tag = (Sound *)halo::cache::globals().tag_instances[(uint16_t)tag_id].data;
        sound_tag->scripting_time = 0xffffffff;
        tag_id = halo::cache::tag_iterator_next(&iterator);
    }
}

datum_index create(datum_index object_index, datum_index definition_index, char *marker_name, int16_t function_index)
{
    datum_index handle = k_datum_index_none;
    object_marker marker;

    if (definition_index == k_datum_index_none) {
        return handle;
    }

    if (object_index != k_datum_index_none) {
        if ((int16_t)halo::objects::object_get_node_local_transform(object_index, marker_name, &marker, 1) == 0) {
            return handle;
        }
    }

    handle = halo::memory::datum_new(game_looping_sound_data);
    if (handle != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)handle];

        self->object_index = object_index;
        self->definition_index = definition_index;
        self->state = _game_looping_sound_stopped;
        self->flags = 0;
        self->function_index = function_index;
        self->last_update = -1;

        if (object_index != k_datum_index_none) {
            self->node_index = marker.node_index;
            self->position.x = marker.transform.position.x;
            self->position.y = marker.transform.position.y;
            self->position.z = marker.transform.position.z;
            self->forward.i = marker.transform.forward.i;
            self->forward.j = marker.transform.forward.j;
            self->forward.k = marker.transform.forward.k;
        }
    }

    return handle;
}

void touch_if_valid(datum_index looping_sound_index)
{
    game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)looping_sound_index];
    uint8_t stale = (self->last_update == -1) || (self->last_update == game_sound_globals_ptr->update_count - 1);
    uint8_t function_live;

    if ((self->flags & _game_looping_sound_script_gain_bit) == 0) {
        if (self->function_index == -1) {
            function_live = 1;
        } else {
            object_header *header = &((object_header *)object_data->data)[self->object_index & 0xffff];
            object *obj = (object *)header->data;
            function_live = (obj->function_valid_flags & (1 << (self->function_index & 0x1f))) != 0;
        }
    } else {
        function_live = (~(self->flags >> 1)) & 1;
    }

    if (function_live != 0 || (self->state != _game_looping_sound_stopped && stale)) {
        looping::touch((int32_t)looping_sound_index);
    }
}

void update_sound(datum_index looping_sound_index, int32_t *root_location)
{
    game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[looping_sound_index & 0xffff];
    SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[self->definition_index & 0xffff].data;
    uint8_t stale = self->last_update == -1 || self->last_update == game_sound_globals_ptr->update_count - 1;
    uint32_t flags = self->flags;
    uint8_t alternate = (uint8_t)((flags >> 3) & 1);
    sound_location location;
    uint8_t live;

    if ((flags & _game_looping_sound_script_gain_bit) == 0) {
        object *obj = ((object_header *)object_data->data)[self->object_index & 0xffff].data;

        if (self->function_index == -1) {
            location.scale = 1.0f;
            live = 1;
        } else {
            location.scale = obj->function_out_values[self->function_index];
            live = (obj->function_valid_flags & (uint8_t)(1 << (self->function_index & 0x1f))) != 0;
        }
    } else {
        location.scale = self->scale;
        live = (uint8_t)(~(flags >> 1) & 1);
    }

    if (!live) {
        if (self->state == _game_looping_sound_stopped) {
            goto done;
        }
        if (!stale) {
            self->state = _game_looping_sound_stopped;
            goto done;
        }
    }

    location.obstruction = 0.0f;
    location.occlusion = 0.0f;
    if (self->object_index != k_datum_index_none) {
        object *obj = ((object_header *)object_data->data)[self->object_index & 0xffff].data;
        real_matrix4x3 *node_matrix = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset + self->node_index * 0x34);

        halo::math::matrix4x3_transform_point(*((real_point3d *)&location.position), *((real_point3d *)&self->position), *node_matrix);
        halo::math::matrix4x3_transform_normal(*((real_vector3d *)&location.forward), *((real_vector3d *)&self->forward), *node_matrix);
        halo::objects::object_get_root_object_velocities(self->object_index, (real_vector3d *)&location.velocity,
            (real_vector3d *)0);
        location.leaf_index = root_location[0];
        *(int32_t *)&location.cluster_index = root_location[1];
        location.type = _sound_location_absolute;
    } else {
        location.type = _sound_location_none;
    }
    location.gain = 1.0f;

    if (live) {
        int16_t new_state = (self->state == _game_looping_sound_playing || !stale) ? 1 : 0;

        self->state = _game_looping_sound_playing;
        if (looping::set_state((int32_t)looping_sound_index, self->definition_index, &location,
                new_state, alternate, 0.0f) == 0) {
            goto done;
        }
        if ((self->flags & _game_looping_sound_script_gain_bit) == 0) {
            self->state = _game_looping_sound_stopped;
            goto done;
        }
        if (*(uint32_t *)&definition->runtime_scripting_sound == (uint32_t)looping_sound_index) {
            *(uint32_t *)&definition->runtime_scripting_sound = 0xffffffff;
        }
        halo::memory::datum_delete(game_looping_sound_data, looping_sound_index);
        goto done;
    }

    if (looping::set_state((int32_t)looping_sound_index, self->definition_index, &location,
            2, alternate, (flags & _game_looping_sound_stopped_by_music_bit) != 0 ? 4.0f : 0.0f) == 0) {
        self->state = _game_looping_sound_stopping;
        goto done;
    }
    if ((self->flags & _game_looping_sound_script_gain_bit) == 0) {
        self->state = _game_looping_sound_stopped;
        goto done;
    }
    halo::memory::datum_delete(game_looping_sound_data, looping_sound_index);

done:
    self->last_update = game_sound_globals_ptr->update_count;
}

void update(void)
{
    int32_t now_ms = game_sound_update_now_ms();

    if ((uint32_t)(now_ms - game_sound_globals_ptr->last_update_time) < k_game_sound_update_interval_ms) {
        datum_index index = halo::memory::datum_next(-1, game_looping_sound_data);
        while (index != k_datum_index_none) {
            game_looping::touch_if_valid(index);
            index = halo::memory::datum_next((int16_t)index, game_looping_sound_data);
        }
        return;
    }

    {
        int32_t cluster_info[2];
        SoundEnvironment *environment;
        uint8_t valid;
        datum_index background_index;
        datum_index index;

        spatial::environment_update((uint32_t *)cluster_info, (void **)&environment, &valid);
        sound_environment = *environment;

        spatial::build_cluster_range_bitmap();

        background_index = *(datum_index *)&game_sound_globals_ptr->background_sound_index;

        if ((uint32_t)cluster_info[0] == 0xffffffff) {
            if (background_index != k_datum_index_none) {
                game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)background_index];
                self->flags |= _game_looping_sound_stop_requested_bit;
                *(uint32_t *)&game_sound_globals_ptr->background_sound_index = 0xffffffff;
            }
        } else if (background_index == k_datum_index_none) {
            background_index = looping::start_ambient(k_datum_index_none, (datum_index)cluster_info[0], 1.0f);
            *(uint32_t *)&game_sound_globals_ptr->background_sound_index = (uint32_t)background_index;
        } else {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)background_index];
            if ((uint32_t)self->definition_index != (uint32_t)cluster_info[0]) {
                self->flags |= _game_looping_sound_stop_requested_bit;
                background_index = looping::start_ambient(k_datum_index_none, (datum_index)cluster_info[0], 1.0f);
                *(uint32_t *)&game_sound_globals_ptr->background_sound_index = (uint32_t)background_index;
            }
        }

        index = halo::memory::datum_next(-1, game_looping_sound_data);
        while (index != k_datum_index_none) {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)index];
            datum_index object_index = self->object_index;

            if (object_index == k_datum_index_none) {
                game_looping::update_sound(index, (int32_t *)0);
            } else if ((self->flags & _game_looping_sound_script_gain_bit) == 0 ||
                       halo::objects::object_try_and_get(object_index, 0xffffffff) != 0) {
                object *obj = (object *)((object_header *)object_data->data)[object_index & 0xffff].data;

                if ((obj->flags & _object_needs_cluster_update_bit) != 0) {
                    int32_t leaf_cluster[2];

                    halo::objects::object_get_root_location(leaf_cluster, object_index);

                    if ((int16_t)leaf_cluster[1] != -1 &&
                        (sound_cluster_audible_bitmap[(int16_t)leaf_cluster[1] >> 5] &
                         (1u << (leaf_cluster[1] & 0x1f))) != 0) {
                        game_looping::update_sound(index, leaf_cluster);
                    }
                }
            } else {
                SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[self->definition_index & 0xffff].data;
                if (*(uint32_t *)&definition->runtime_scripting_sound == (uint32_t)index) {
                    *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)k_datum_index_none;
                }
                halo::memory::datum_delete(game_looping_sound_data, index);
            }

            index = halo::memory::datum_next((int16_t)index, game_looping_sound_data);
        }

        game_sound_globals_ptr->update_count += 1;
        game_sound_globals_ptr->last_update_time = game_sound_update_now_ms();
    }
}

void stop_loops_conflicting_with_music(void)
{
    datum_index index = halo::memory::datum_next(-1, game_looping_sound_data);

    while (index != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)index];

        if (self->object_index == k_datum_index_none) {
            datum_index looping_definition = self->definition_index;
            uint32_t has_music_loop = looping::definition_has_music_loop(looping_definition);

            if (has_music_loop != 0 && looping_definition != k_datum_index_none) {
                SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[looping_definition & 0xffff].data;
                datum_index scripted_sound = *(datum_index *)&definition->runtime_scripting_sound;

                if (scripted_sound != k_datum_index_none) {
                    game_looping_sound *scripted = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)scripted_sound];

                    scripted->flags &= ~_game_looping_sound_scripted_bit;
                    scripted->flags |= _game_looping_sound_stop_requested_bit;
                    *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)k_datum_index_none;
                    scripted->flags |= _game_looping_sound_stopped_by_music_bit;
                }
            }
        }

        index = halo::memory::datum_next((int16_t)index, game_looping_sound_data);
    }
}

}  // namespace game_looping


}  // namespace halo::sound
