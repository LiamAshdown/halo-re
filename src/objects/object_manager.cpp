#include "halo/objects/object_manager.hpp"
#include "game.h"
#include "physics.h"
#include "structures.h"
#include "hs.h"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"

extern "C" {
extern void *ai_gc_callback_table;
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern datum_index *collideable_cluster_first;
extern void *collideable_cluster_partition;
extern data_array *collideable_object_references;
extern uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, collision_bsp_sphere_result *result, uint32_t *breakable_surfaces, real_point3d *center, float radius);
extern void console_print_error_va(const char *format, ...);
extern uint8_t *game_state_base;
extern uint32_t game_state_crc;
extern int32_t game_state_cursor;
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
extern memory_pool *game_state_new_pool(char *name, int32_t pool_size);
extern game_time_globals *game_time;
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern uint8_t *global_scenario;
extern ScenarioStructureBSP *global_structure_bsp;
extern uint32_t global_structure_collision_bsp;
extern datum_index *light_cluster_first;
extern data_array *light_cluster_references;
extern data_array *light_data;
extern data_array *light_object_references;
extern void lights_dispose_all(void);
extern void lights_initialize(void);
extern player_globals *local_player_globals;
extern uint8_t *main_game_globals;
extern char network_log_path_format[];
extern datum_index *noncollideable_cluster_first;
extern void *noncollideable_cluster_partition;
extern data_array *noncollideable_object_references;
extern void object_block_data_free(data_array *array, datum_index object_index);
extern void object_clear_pending_delete_flag(uint32_t object_index);
extern int32_t object_cluster_stamp;
extern data_array *object_data;
extern void object_delete(uint32_t object_index);
extern void object_delete_4f9030(uint32_t object_index, char recurse_siblings);
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings);
extern void object_delete_unparented(uint32_t object_index);
extern void object_dump_accumulate_stats(uint32_t object_index, object_memory_dump_record *record);
extern int object_dump_compare_by_total_size(const object_memory_dump_record *a, const object_memory_dump_record *b);
extern void object_dump_write(object_memory_dump_record *record, void *file);
extern uint32_t object_get_root_object_index(uint32_t object_index);
extern object_globals *object_globals_pointer;
extern object *object_iterator_next(object_iterator *iterator);
extern void object_list_membership_set(uint32_t object_index, char add);
extern void object_mark_pending_delete(uint32_t object_index);
extern memory_pool *object_memory_pool;
extern datum_index *object_name_list;
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location);
extern int32_t object_sound_event_last_tick;
extern uint8_t object_test_in_atmosphere_zone(uint32_t object_index);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void object_type_definition_chain_build(void);
extern object_type_definition *object_type_definition_list;
extern object_type_definition *object_type_definitions[k_maximum_object_types];
extern void object_type_definitions_notify_0x54(uint32_t object_index);
extern uint32_t object_unknown_006b8c60;
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);
extern void object_update(uint32_t object_index);
extern uint16_t object_visibility_computed_mask;
extern void objects_garbage_collection(void);
extern void objects_get_statistics(void *out);
extern int32_t sprintf(char *buffer, const char *format, ...);
extern widget_type_definition widget_type_definitions[k_maximum_widget_types];
extern void widgets_dispose(void);
extern void widgets_dispose_clear_flag(void);
extern void widgets_initialize(void);
}

/**
 * Deletes unparented scenery and light fixtures.
 *
 * Original register convention: none (void).
 *
 * @address 0x004f47c0
 */
void halo::objects::ObjectManager::delete_unparented_of_type_mask()
{
    object_iterator iterator;
    object *obj;
    int32_t role;

    iterator.type_mask = _object_mask_scenery_and_light_fixture;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != 0) {
        if (obj->render_cache_slot == -1) {
            role = obj->network_role;
            if (role == 0) {
                object_delete_unparented(iterator.handle);
            }
            if (role == 0 || role == 3) {
                object_delete_recursive(iterator.handle, 0);
            }
        }
        obj = object_iterator_next(&iterator);
    }
}

namespace {
static cluster_reference_group &collideable_cluster_first__as_objects_initialize = reinterpret_cast<cluster_reference_group &>(collideable_cluster_first);
static cluster_reference_group &noncollideable_cluster_first__as_objects_initialize = reinterpret_cast<cluster_reference_group &>(noncollideable_cluster_first);
}

/**
 * Allocates the object game state, data arrays and cluster tables.
 *
 * Original register convention: none (void), matches Ghidra's __cdecl void(void) signature.
 *
 * @address 0x004f4ad0
 */
void halo::objects::ObjectManager::initialize()
{
    uint8_t *globals_region;
    uint8_t *name_list_region;
    int32_t size;

    widgets_initialize();
    object_type_definition_chain_build();
    lights_initialize();
    object_data = game_state_new((char *)"object", k_maximum_objects, 0xc  );

    object_memory_pool = game_state_new_pool((char *)"objects", 0x200000);

    globals_region = game_state_base + game_state_cursor;
    game_state_cursor = game_state_cursor + 0x98;
    size = 0x98;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    name_list_region = game_state_base + game_state_cursor;
    game_state_cursor = game_state_cursor + 0x800;
    size = 0x800;
    object_globals_pointer = (object_globals *)globals_region;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    object_name_list = (datum_index *)name_list_region;

    halo::structures::cluster_partition_new(&collideable_cluster_first__as_objects_initialize, (char *)"collideable object");
    halo::structures::cluster_partition_new(&noncollideable_cluster_first__as_objects_initialize, (char *)"noncollideable object");
}

/**
 * Resets the object system for a new map.
 *
 * Original register convention: none (void), matches Ghidra's __cdecl void(void) signature.
 *
 * @address 0x004f4bb0
 */
void halo::objects::ObjectManager::reset()
{
    object_type_definition *def;
    datum_index *slot;
    int32_t i;

    object_unknown_006b8c60 = 0xffffffff;
    object_sound_event_last_tick = 0;
    widgets_dispose();
    object_visibility_computed_mask = 0;

    for (def = object_type_definition_list; def != 0; def = def->next) {
        if (def->reset != 0) {
            ((void (*)(void))def->reset)();
        }
    }

    lights_dispose_all();

    object_data->valid = 1;
    halo::memory::data_delete_all(object_data);

    slot = object_name_list;
    for (i = k_maximum_object_names; i != 0; i--) {
        *slot = k_datum_index_none;
        slot++;
    }

    slot = collideable_cluster_first;
    for (i = k_maximum_clusters; i != 0; i--) {
        *slot = k_datum_index_none;
        slot++;
    }

    ((data_array *)collideable_cluster_partition)->valid = 1;
    halo::memory::data_delete_all((data_array *)collideable_cluster_partition);
    collideable_object_references->valid = 1;
    halo::memory::data_delete_all(collideable_object_references);

    slot = noncollideable_cluster_first;
    for (i = k_maximum_clusters; i != 0; i--) {
        *slot = k_datum_index_none;
        slot++;
    }

    ((data_array *)noncollideable_cluster_partition)->valid = 1;
    halo::memory::data_delete_all((data_array *)noncollideable_cluster_partition);
    noncollideable_object_references->valid = 1;
    halo::memory::data_delete_all(noncollideable_object_references);

    for (i = 0; i < 16; i++) {
        object_globals_pointer->cluster_pvs_previous[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        object_globals_pointer->cluster_pvs_current[i] = 0;
    }
    object_globals_pointer->ambient_cluster_mode = 0;
    object_globals_pointer->collecting_in_clusters = 0;
    object_cluster_stamp = 0;
    object_globals_pointer->active_garbage_object_count = 0;
    object_globals_pointer->last_garbage_collection_time = 0;
    object_globals_pointer->first_tracked_object = k_datum_index_none;
}

/**
 * Flushes pending dirty state of every object.
 *
 * Original register convention: none (void), matches the callers seen elsewhere in this batch.
 *
 * @address 0x004f4cc0
 */
void halo::objects::ObjectManager::flush_dirty_state()
{
    object_type_definition *def;
    datum_index index;

    widgets_dispose_clear_flag();

    for (def = object_type_definition_list; def != 0; def = def->next) {
        if (def->flush != 0) {
            ((void (*)(void))def->flush)();
        }
    }

    light_data->valid = 0;
    if (light_object_references->valid != 0) {
        light_object_references->valid = 0;
    }
    if (light_cluster_references->valid != 0) {
        light_cluster_references->valid = 0;
    }

    if (object_data->valid != 0) {
        index = halo::memory::datum_next(-1, object_data);
        while (index != k_datum_index_none) {
            object_block_data_free(object_data, index);
            index = halo::memory::datum_next((int16_t)index, object_data);
        }
        object_data->valid = 0;
    }

    if (((data_array *)collideable_cluster_partition)->valid != 0) {
        ((data_array *)collideable_cluster_partition)->valid = 0;
    }
    if (collideable_object_references->valid != 0) {
        collideable_object_references->valid = 0;
    }
    if (((data_array *)noncollideable_cluster_partition)->valid != 0) {
        ((data_array *)noncollideable_cluster_partition)->valid = 0;
    }
    if (noncollideable_object_references->valid != 0) {
        noncollideable_object_references->valid = 0;
    }
}

/**
 * Disposes the object system and its game state.
 *
 * Original register convention: none (void), matches Ghidra's __cdecl void(void) signature.
 *
 * @address 0x004f4db0
 */
void halo::objects::ObjectManager::dispose()
{
    int i;
    object_type_definition *def;

    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].reset != 0) {
            ((void (*)(void))widget_type_definitions[i].reset)();
        }
    }

    for (def = object_type_definition_list; def != 0; def = def->next) {
        if (def->dispose != 0) {
            ((void (*)(void))def->dispose)();
        }
    }

    if (light_cluster_first != 0) {
        light_cluster_first = 0;
    }
    if (light_object_references != 0) {
        light_object_references = 0;
    }
    if (light_cluster_references != 0) {
        light_cluster_references = 0;
    }
    if (object_data != 0) {
        object_data = 0;
    }
    if (object_memory_pool != 0) {
        object_memory_pool = 0;
    }
    if (collideable_cluster_first != 0) {
        collideable_cluster_first = 0;
    }
    if (collideable_cluster_partition != 0) {
        collideable_cluster_partition = 0;
    }
    if (collideable_object_references != 0) {
        collideable_object_references = 0;
    }
    if (noncollideable_cluster_first != 0) {
        noncollideable_cluster_first = 0;
    }
    if (noncollideable_cluster_partition != 0) {
        noncollideable_cluster_partition = 0;
    }
    if (noncollideable_object_references != 0) {
        noncollideable_object_references = 0;
    }
}

/**
 * Per-tick update of every object and the system around it.
 *
 * Original register convention: none (void), matches the other module init/update entry points.
 *
 * @address 0x004f4e90
 */
void halo::objects::ObjectManager::update()
{
    object_globals *globals = object_globals_pointer;
    int restrict_to_units;
    int16_t cluster_count;
    int32_t word_count;
    int32_t i;
    int changed;
    object_header *headers;
    int16_t last_index;

    restrict_to_units = (*(uint8_t *)((uint8_t *)game_time + 0xc) & 1) != 0 && main_game_globals[2] != 0;

    globals->active_garbage_object_count = 0;

    cluster_count = *(int16_t *)&global_structure_bsp->clusters.count;
    word_count = (cluster_count + 0x1f) >> 5;

    for (i = 0; i < word_count; i++) {
        globals->cluster_pvs_previous[i] = globals->cluster_pvs_current[i];
    }
    for (i = 0; i < word_count; i++) {
        globals->cluster_pvs_current[i] = *(uint32_t *)&local_player_globals->cluster_pvs[i];
    }

    changed = 0;
    for (i = 0; i < word_count; i++) {
        if (globals->cluster_pvs_previous[i] != globals->cluster_pvs_current[i]) {
            changed = 1;
            break;
        }
    }

    if (changed) {
        headers = (object_header *)object_data->data;
        last_index = object_data->last_index;
        for (i = 0; i < last_index; i++) {
            object_header *header = &headers[i];
            uint8_t flags;
            uint32_t handle;

            if (header->identifier == 0) {
                continue;
            }
            flags = header->flags;
            if ((flags & (_object_header_in_pvs_pass_bit | _object_header_connected_bit)) !=
                (_object_header_in_pvs_pass_bit | _object_header_connected_bit)) {
                continue;
            }
            handle = ((uint32_t)header->identifier << 16) | (uint16_t)i;
            if ((flags & _object_header_active_bit) == 0) {
                if ((int8_t)flags >= 0 && header->cluster_index != -1 &&
                    (globals->cluster_pvs_current[header->cluster_index >> 5] &
                     (1u << (header->cluster_index & 0x1f))) != 0) {
                    object_mark_pending_delete(handle);
                }
            } else if ((globals->cluster_pvs_current[header->cluster_index >> 5] &
                        (1u << (header->cluster_index & 0x1f))) == 0) {
                if ((header->data->flags & _object_connected_to_map_bit) == 0) {
                    object_clear_pending_delete_flag(handle);
                } else {
                    object_delete(handle);
                }
            }
        }
        halo::structures::structure_decals_update_switch_transitions(globals->cluster_pvs_previous, globals->cluster_pvs_current, cluster_count);
    }

    headers = (object_header *)object_data->data;
    last_index = object_data->last_index;
    for (i = 0; i < last_index; i++) {
        object_header *header = &headers[i];
        if (header->identifier != 0 && (header->flags & _object_header_active_bit) != 0 &&
            (header->flags & _object_header_needs_update_bit) == 0) {
            if (!restrict_to_units ||
                (((1 << (header->type & 0x1f)) & _object_mask_unit) != 0 &&
                 *(int32_t *)((uint8_t *)header->data + 0x218) != -1)) {
                object_update(((uint32_t)header->identifier << 16) | (uint16_t)i);
            }
        }
    }

    headers = (object_header *)object_data->data;
    last_index = object_data->last_index;
    for (i = 0; i < last_index; i++) {
        object_header *header = &headers[i];
        if (header->identifier != 0) {
            uint8_t original_flags = header->flags;
            uint32_t handle = ((uint32_t)header->identifier << 16) | (uint16_t)i;

            header->flags = original_flags & (uint8_t)~_object_header_just_created_bit;
            if ((original_flags & _object_header_needs_update_bit) != 0) {
                header->flags = original_flags &
                    (uint8_t)~(_object_header_needs_update_bit | _object_header_just_created_bit);
                object_update(handle);
            }
            if ((header->flags & _object_header_delete_pending_bit) != 0) {
                object_delete_4f9030(handle, 0);
            }
        }
    }

    objects_garbage_collection();
}

/**
 * Refreshes the cluster membership of every object.
 *
 * Original register convention: no parameters. Confirmed against objdump -d -M intel bin/halo.exe: the whole function
 * reads nothing from the incoming stack or registers before building its own local iterator.
 *
 * @address 0x004f74f0
 */
void halo::objects::ObjectManager::sweep_refresh_cluster_membership()
{
    object_iterator iterator;
    object *obj;

    iterator.type_mask = 0xffffffff;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        if (((obj->flags & _object_needs_cluster_update_bit) != 0) &&
            (obj->parent_object == k_datum_index_none)) {
            object_unlink_cluster_or_notify_parent(iterator.handle);
            obj->flags |= _object_needs_cluster_update_bit;
        }
        object_type_definitions_notify_0x54(iterator.handle);
        obj = object_iterator_next(&iterator);
    }
}

/**
 * Recomputes the cluster membership of every object.
 *
 * Original register convention: no parameters (matches functions.md: a bulk per-tick sweep).
 *
 * @address 0x004f7570
 */
void halo::objects::ObjectManager::recompute_cluster_membership()
{
    object_iterator iterator;
    object *obj;

    iterator.type_mask = 0xffffffff;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        if (((obj->flags & _object_needs_cluster_update_bit) != 0) &&
            (obj->parent_object == k_datum_index_none)) {
            object_header *header = (object_header *)object_data->data + (iterator.handle & 0xffff);
            int32_t leaf;
            int16_t cluster;
            bsp_leaf_reference location;
            collision_bsp_sphere_result sphere;

            obj->flags &= ~(uint32_t)_object_needs_cluster_update_bit;
            obj->location_cluster_index = -1;
            header->cluster_index = -1;

            leaf = (int32_t)bsp3d_node_find_leaf(0, global_collision_bsp, &obj->bounding_center);
            cluster = (leaf == -1) ? -1 :
                *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
            if (leaf == -1 || cluster == -1) {
                collision_bsp_query_sphere_init((ModelCollisionGeometryBSP *)global_structure_collision_bsp, 0,
                    &sphere, 0, &obj->bounding_center, obj->bounding_radius);
                if (sphere.leaf_count != 0) {
                    leaf = sphere.leaves[0];
                } else {
                    leaf = (int32_t)bsp3d_node_find_leaf(0, global_collision_bsp, &obj->position);
                }
                cluster = (leaf == -1) ? -1 :
                    *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
            }

            location.leaf_index = leaf;
            location.cluster_index = cluster;
            object_set_cluster_and_parent(iterator.handle, &location);
        }
        obj = object_iterator_next(&iterator);
    }
}

/**
 * Writes the object count statistics.
 *
 * Original register convention: out pointer in EDX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f7953 mov
 * ecx,edx at entry. // blam-cc: EDX -> out.
 *
 * @address 0x004f7950
 */
void halo::objects::ObjectManager::get_statistics(object_statistics *out)
{
    int32_t used_end;
    int16_t i;

    out->count = 0;
    out->active_count = 0;

    for (i = 0; i < object_data->last_index; i++) {
        object_header *header = (object_header *)object_data->data + i;
        if (header->identifier != 0) {
            out->count = out->count + 1;
            if ((header->flags & _object_header_active_bit) != 0) {
                out->active_count = out->active_count + 1;
            }
        }
    }

    if (object_memory_pool->last_block == 0) {
        used_end = 0;
    } else {
        used_end = (int32_t)object_memory_pool->last_block + object_memory_pool->last_block->size -
                   (int32_t)object_memory_pool->base;
    }

    out->pool_fullness_fraction = 1.0f - (float)(object_memory_pool->size - used_end) * 4.7683716e-07f;
}

/**
 * Overrides the ambient cluster for a local player.
 *
 * Original register convention: a local-player index in AX. Confirmed against objdump -d -M intel bin/halo.exe:
 * 0x4f79d0 cmp ax,0xffff at entry, no stack access. // blam-cc: AX -> local_player_index.
 *
 * @address 0x004f79d0
 */
void halo::objects::ObjectManager::set_ambient_cluster_override(int16_t local_player_index)
{
    if (local_player_index != -1) {
        uint8_t *player_base = *(uint8_t **)(global_scenario + 0x4f4);
        real_point3d *point = (real_point3d *)(player_base + local_player_index * 0x68 + 0x28);
        int32_t leaf = bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)global_collision_bsp, point);

        if (leaf != -1) {

            int16_t cluster = *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer +
                                           (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
            if (cluster != -1) {
                object_globals_pointer->ambient_cluster_mode = _object_ambient_cluster_override;
                object_globals_pointer->ambient_cluster_index = cluster;
                return;
            }
        }
    }

    object_globals_pointer->ambient_cluster_mode = _object_ambient_cluster_none;
}

/**
 * Returns the cluster used for ambient lighting.
 *
 * Original register convention: no parameters. Confirmed against objdump -d -M intel bin/halo.exe: the whole function
 * reads only object_globals_pointer before branching on ambient_cluster_mode.
 *
 * @address 0x004f7a50
 */
int16_t halo::objects::ObjectManager::get_ambient_cluster()
{
    if (object_globals_pointer->ambient_cluster_mode == _object_ambient_cluster_from_tracked_object) {
        datum_index handle = (uint16_t)object_globals_pointer->ambient_cluster_index;
        if (object_try_and_get(handle, 0xffffffff) == 0) {
            object_globals_pointer->ambient_cluster_mode = _object_ambient_cluster_none;
        } else {
            uint32_t root_index = object_get_root_object_index(handle);
            object *root = ((object_header *)object_data->data)[root_index & 0xffff].data;
            if ((root->flags & _object_needs_cluster_update_bit) != 0) {
                if (root->location_cluster_index != -1) {
                    return root->location_cluster_index;
                }
            }
        }
        return -1;
    }

    if (object_globals_pointer->ambient_cluster_mode == _object_ambient_cluster_override) {
        return object_globals_pointer->ambient_cluster_index;
    }

    return -1;
}

/**
 * Deletes objects marked for deletion and objects that fell out of the world.
 *
 * Original register convention: no parameters (matches functions.md: a periodic sweep with no caller inputs).
 *
 * @address 0x004f9c60
 */
void halo::objects::ObjectManager::garbage_collection()
{
    static datum_index list[2044];
    char free_text[512];
    char critical_text[512];
    char removing_text[512];
    char callback_text[512];
    int32_t mode;
    int16_t count = 0;
    uint8_t done = 0;
    int32_t used;
    datum_index handle;

    if (object_globals_pointer->unknown_02[0] != 0) {
        mode = 0;
    } else {
        used = (object_memory_pool->last_block == 0) ? 0 :
            (int32_t)((uint8_t *)object_memory_pool->last_block + object_memory_pool->last_block->size -
                      (uint8_t *)object_memory_pool->base);
        if (object_memory_pool->size - used <= 0x19999) {
            halo::memory::block_list_compact(object_memory_pool);
            used = (object_memory_pool->last_block == 0) ? 0 :
                (int32_t)((uint8_t *)object_memory_pool->last_block + object_memory_pool->last_block->size -
                          (uint8_t *)object_memory_pool->base);
            if (object_memory_pool->size - used > 0x33333) {
                object_globals_pointer->unknown_02[0] = 0;
                return;
            }
            mode = 2;
        } else if (0x800 - object_data->actual_count <= 0x66) {
            mode = 2;
        } else if (object_globals_pointer->active_garbage_object_count < 0x32) {
            object_globals_pointer->unknown_02[0] = 0;
            return;
        } else {
            mode = 1;
        }
    }

    for (handle = object_globals_pointer->first_tracked_object; handle != k_datum_index_none;
         handle = *(datum_index *)((uint8_t *)((object_header *)object_data->data)[handle & 0xffff].data + 0x110)) {
        list[count++] = handle;
    }

    for (;;) {
        object_header *header;
        uint8_t eligible;

        if (mode == 0) {
            done = 0;
        } else if (mode == 1) {
            done = (uint8_t)(object_globals_pointer->active_garbage_object_count <= 0x1e);
            if (done) {
                break;
            }
        } else if (mode == 2) {
            if (object_memory_pool->free_bytes >= 0x33333 && 0x800 - object_data->last_index >= 0xcc) {
                done = 1;
                break;
            }
            done = 0;
        } else if (done) {
            break;
        }
        if (count == 0) {
            break;
        }
        count--;
        handle = list[count];
        header = (object_header *)object_data->data + (handle & 0xffff);
        eligible = (mode == 1) ? (uint8_t)(header->flags & _object_header_active_bit) : 1;
        if (object_test_in_atmosphere_zone(handle) != 0 || eligible == 0) {
            continue;
        }
        if ((header->flags & _object_header_active_bit) != 0) {
            object_globals_pointer->active_garbage_object_count--;
        }
        object_list_membership_set(handle, 0);
        object_delete_recursive(handle, 0);
        object_delete_4f9030(handle, 0);
    }

    halo::memory::block_list_compact(object_memory_pool);
    if (done) {
        object_globals_pointer->unknown_02[0] = 0;
        return;
    }

    {
        void **entry = (void **)&ai_gc_callback_table;
        uint8_t prepared = 0;
        uint8_t retried = 0;
        uint8_t reported = 0;
        uint8_t stale;
        uint32_t last = object_globals_pointer->last_garbage_collection_time;

        stale = (uint8_t)(last == 0xffffffff || !((int32_t)last + 0x96 >= game_time->game_time));

        for (;;) {
            uint8_t significant = 0;
            uint8_t critical = 0;
            const char *qualifier;

            if (mode == 2) {
                int32_t free_bytes;
                int32_t free_slots;

                used = (object_memory_pool->last_block == 0) ? 0 :
                    (int32_t)((uint8_t *)object_memory_pool->last_block + object_memory_pool->last_block->size -
                              (uint8_t *)object_memory_pool->base);
                free_bytes = object_memory_pool->size - used;
                free_slots = 0x800 - object_data->last_index;
                if (free_bytes <= 0xcccc) {
                    critical = 1;
                    significant = 1;
                    sprintf(free_text, "%4.2f%% memory free", (double)((float)free_bytes * 100.0f * 4.7683716e-07f));
                } else if (free_slots <= 0x33) {
                    critical = 1;
                    significant = 1;
                    sprintf(free_text, "%d slots free", free_slots);
                } else if (free_bytes <= 0x19999) {
                    significant = 1;
                    sprintf(free_text, "%4.2f%% memory free", (double)((float)free_bytes * 100.0f * 4.7683716e-07f));
                } else if (free_slots > 0x66) {
                    sprintf(free_text, "%4.2f%% memory free", (double)((float)free_bytes * 100.0f * 4.7683716e-07f));
                } else {
                    significant = 1;
                    sprintf(free_text, "%d slots free", free_slots);
                }
            }

            if (critical) {
                qualifier = retried ? "still " : "";
            } else if (retried) {
                qualifier = "not ";
            } else {
                if ((significant && stale) || reported) {
                    break;
                }
                object_globals_pointer->unknown_02[0] = 0;
                return;
            }

            sprintf(critical_text, "garbage collection %scritical (%s)", qualifier, free_text);
            console_print_error_va(network_log_path_format, critical_text);
            reported = 1;
            if (!critical || entry[1] == 0) {
                break;
            }

            {
                uint8_t removed = 0;

                do {
                    uint8_t more = 0;

                    if (!prepared && entry[0] != 0) {
                        ((void (*)(void *, int32_t))entry[0])(list, 0x1000);
                        prepared = 1;
                    }
                    removed = ((uint8_t (*)(char *, uint8_t *, void *, int32_t))entry[1])(callback_text, &more,
                        list, 0x1000);
                    if (removed) {
                        sprintf(removing_text, "removing objects: %s", callback_text);
                        console_print_error_va(network_log_path_format, removing_text);
                    }
                    if (!more) {
                        entry += 2;
                        prepared = 0;
                    }
                } while (!removed && entry[1] != 0);

                if (!removed) {
                    break;
                }
            }
            retried = 1;
            halo::memory::block_list_compact(object_memory_pool);
        }
    }

    object_globals_pointer->last_garbage_collection_time = (uint32_t)game_time->game_time;
    object_globals_pointer->unknown_02[0] = 0;
}

/**
 * Comparison callback ordering dump records by total size.
 *
 * Original register convention: __cdecl, both parameters on the stack (Ghidra already recovered this fully, including
 * the calling convention).
 *
 * @address 0x004fa3a0
 */
int halo::objects::ObjectMemoryDumpRecordView::compare_by_total_size(const object_memory_dump_record *b)
{
    const object_memory_dump_record *a = self;
    if (a->total_size < b->total_size) {
        return 1;
    }
    return (a->total_size <= b->total_size) ? 0 : -1;
}

/**
 * Adds an object's memory use to its type's dump record.
 *
 * @address 0x004fa3d0
 */
void halo::objects::ObjectMemoryDumpRecordView::accumulate_stats(uint32_t object_index)
{
    object_memory_dump_record *record = self;
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;

    if (record->maximum_size < header->block_size) {
        record->maximum_size = header->block_size;
    }
    record->count = record->count + 1;
    record->total_size = record->total_size + header->block_size;
    if ((header->flags & _object_header_active_bit) != 0) {
        record->active_count = record->active_count + 1;
    }
    if ((obj->flags & _object_in_tracked_list_bit) != 0) {
        record->garbage_count = record->garbage_count + 1;
    }
    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        record->dead_count = record->dead_count + 1;
    }
    if ((obj->flags & _object_at_rest_bit) != 0) {
        record->at_rest_count = record->at_rest_count + 1;
    }

    {
        uint32_t root = k_datum_index_none;
        uint32_t current = object_index;
        while (current != k_datum_index_none) {
            root = current;
            current = ((object_header *)object_data->data)[root & 0xffff].data->parent_object;
        }
        {
            object *root_obj = ((object_header *)object_data->data)[root & 0xffff].data;
            if (((root_obj->flags & _object_outside_map_bit) != 0) || (root_obj->location_cluster_index == -1)) {
                record->outside_map_count = record->outside_map_count + 1;
            }
        }
    }
}

/**
 * Writes one dump record as a line to the open file.
 *
 * Original register convention: the record in EAX, the output FILE* as the sole stack parameter. Confirmed against
 * objdump -d -M intel bin/halo.exe: 0x4fa490 mov edx,[eax] at entry. // blam-cc: EAX -> record, stack -> file.
 *
 * @address 0x004fa490
 */
void halo::objects::ObjectMemoryDumpRecordView::write(void *file)
{
    object_memory_dump_record *record = self;
    const char *name = "unknown";

    if (record->definition_tag == k_datum_index_none) {
        if (record->type != -1) {
            name = object_type_definitions[record->type]->name;
        }
    } else {
        name = halo::cache::globals().tag_instances[(int16_t)record->definition_tag].path;
    }

    fprintf((FILE *)file, "% 6d (% 6d) [% 7d/% 7d/% 7d/% 7d] % 7d % 7d %s\r\n",
        record->count, record->active_count, record->garbage_count, record->dead_count,
        record->outside_map_count, record->at_rest_count, record->maximum_size,
        record->total_size, name);
}

/**
 * Writes a per-type object memory usage dump.
 *
 * Original register convention: no parameters (matches functions.md: a one-shot debug dump).
 *
 * @address 0x004fa500
 */
void halo::objects::ObjectManager::dump_memory()
{
    object_memory_dump_record by_type[k_maximum_object_types];
    object_memory_dump_record by_definition[0x400];
    int16_t definition_count = 0;
    int16_t overflow_count = 0;
    uint8_t stats_buffer[8];
    int16_t i;
    object_iterator iterator;
    object *obj;

    for (i = 0; i < k_maximum_object_types; i++) {
        by_type[i].definition_tag = k_datum_index_none;
        by_type[i].type = i;
        by_type[i].maximum_size = 0;
        by_type[i].total_size = 0;
        by_type[i].count = 0;
        by_type[i].active_count = 0;
        by_type[i].garbage_count = 0;
        by_type[i].dead_count = 0;
        by_type[i].outside_map_count = 0;
        by_type[i].at_rest_count = 0;
    }

    iterator.type_mask = 0xffffffff;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        int16_t slot = -1;
        int16_t j;

        for (j = 0; j < definition_count; j++) {
            if (by_definition[j].definition_tag == obj->definition_tag) {
                slot = j;
                break;
            }
        }

        if (slot == -1) {
            if (definition_count < 0x400) {
                by_definition[definition_count].type = -1;
                by_definition[definition_count].definition_tag = obj->definition_tag;
                by_definition[definition_count].maximum_size = 0;
                by_definition[definition_count].total_size = 0;
                by_definition[definition_count].count = 0;
                by_definition[definition_count].active_count = 0;
                by_definition[definition_count].garbage_count = 0;
                by_definition[definition_count].dead_count = 0;
                by_definition[definition_count].outside_map_count = 0;
                by_definition[definition_count].at_rest_count = 0;
                slot = definition_count;
                definition_count++;
            } else {
                overflow_count++;
            }
        }

        if (slot != -1) {
            object_dump_accumulate_stats(iterator.handle, &by_definition[slot]);
        }
        object_dump_accumulate_stats(iterator.handle, &by_type[obj->type]);

        obj = object_iterator_next(&iterator);
    }

    qsort(by_definition, definition_count, sizeof(object_memory_dump_record), (int (*)(const void *, const void *))object_dump_compare_by_total_size);
    qsort(by_type, k_maximum_object_types, sizeof(object_memory_dump_record), (int (*)(const void *, const void *))object_dump_compare_by_total_size);

    {
        void *file = fopen("object_memory.txt", "a+b");
        if (file != 0) {
            float fraction;

            objects_get_statistics(stats_buffer);
            fraction = *(float *)(stats_buffer + 4);
            overflow_count = *(int16_t *)stats_buffer;

            fprintf((FILE *)file, "#%d objects (#%d active) using %3.2f%% of available memory\n\n", -1, -1, (double)(fraction * 100.0f));
            fprintf((FILE *)file, "OBJECTS BY TYPE\n");
            fprintf((FILE *)file, "number (active) [garbage/   dead/outside/at-rest] maxsize totsize\n");
            for (i = 0; i < k_maximum_object_types; i++) {
                object_dump_write(&by_type[i], file);
            }
            fprintf((FILE *)file, "\n");
            fprintf((FILE *)file, "OBJECTS BY DEFINITION\n");
            fprintf((FILE *)file, "number (active) [garbage/   dead/outside/at-rest] maxsize totsize\n");
            for (i = 0; i < definition_count; i++) {
                fprintf((FILE *)file, "% 6d (% 6d) [% 7d/% 7d/% 7d/% 7d] % 7d % 7d %s\r\n",
                    by_definition[i].count, by_definition[i].active_count, by_definition[i].garbage_count,
                    by_definition[i].dead_count, by_definition[i].outside_map_count, by_definition[i].at_rest_count,
                    by_definition[i].maximum_size, by_definition[i].total_size);

            }
            fprintf((FILE *)file, "\n");
            if (overflow_count > 0) {
                fprintf((FILE *)file, "WARNING: overflowed MAXIMUM_DUMPS (%d), this dump does not include %d objects that would not fit!\n", 0x400);
            }
            fprintf((FILE *)file, "\n");
            fclose((FILE *)file);
        }
    }
}
