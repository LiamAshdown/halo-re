#include "halo/objects/widgets.hpp"

extern "C" {
extern void data_delete_all(data_array *array);
extern void datum_delete(data_array *array, datum_index handle);
extern datum_index datum_new(data_array *array);
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
extern data_array *object_data;
extern tag_instance *tag_instances;
extern data_array *widget_data;
extern widget_type_definition widget_type_definitions[k_maximum_widget_types];
}

/**
 * Creates the widget data array and initialises the widget types.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004ff9d0
 */
void halo::objects::WidgetSystem::initialize()
{
    int32_t i;

    widget_data = game_state_new((char *)"widget", k_maximum_widgets, 0xc  );
    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].initialize != 0) {
            ((void (*)(void))widget_type_definitions[i].initialize)();
        }
    }
}

/**
 * Disposes the widget system.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004ffa10
 */
void halo::objects::WidgetSystem::dispose()
{
    int32_t i;

    widget_data->valid = 1;
    data_delete_all(widget_data);

    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].dispose != 0) {
            ((void (*)(void))widget_type_definitions[i].dispose)();
        }
    }
}

/**
 * Clears the widget disposing flag.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004ffa50
 */
void halo::objects::WidgetSystem::dispose_clear_flag()
{
    int32_t i;

    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].dispose_clear_flag != 0) {
            ((void (*)(void))widget_type_definitions[i].dispose_clear_flag)();
        }
    }
    widget_data->valid = 0;
}

/**
 * Creates the widgets defined by an object's tag.
 *
 * Original register convention: Ghidra shows a single unresolved `in_EAX`; by the object_data lookup shape shared
 * with every function in this module that resolves an object from a bare index, EAX is the object index.
 *
 * @address 0x004ffa80
 */
void halo::objects::WidgetSystem::create(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    int32_t i;

    obj->first_widget = (datum_index)0xffffffff;

    for (i = 0; i < (int32_t)tag->widgets.count; i++) {
        ObjectWidget *attachment = &((ObjectWidget *)tag->widgets.pointer)[i];
        int32_t type;

        for (type = 0; type < k_maximum_widget_types; type++) {
            if (widget_type_definitions[type].group_tag == *(uint32_t *)attachment) {
                if (*(int32_t *)&((struct ObjectWidget *)attachment)->reference.tag_id != -1) {
                    datum_index handle = datum_new(widget_data);
                    if (handle != (datum_index)0xffffffff) {
                        widget *entry = &((widget *)widget_data->data)[handle & 0xffff];
                        entry->type = (int16_t)type;

                        if (widget_type_definitions[type].new_instance == 0) {
                            entry->instance = (datum_index)0xffffffff;
                            entry->next_widget = obj->first_widget;
                            obj->first_widget = handle;
                        } else {
                            datum_index (*new_instance)(TagID) =
                                (datum_index (*)(TagID))widget_type_definitions[type].new_instance;
                            datum_index instance = new_instance(((struct ObjectWidget *)attachment)->reference.tag_id);

                            entry->instance = instance;
                            if (instance == (datum_index)0xffffffff) {
                                datum_delete(widget_data, handle);
                            } else {
                                entry->next_widget = obj->first_widget;
                                obj->first_widget = handle;
                            }
                        }
                    }
                }
                break;
            }
        }
    }
}

/**
 * Deletes every widget of an object.
 *
 * Original register convention: Ghidra shows a single unresolved `in_EAX`; by the same reasoning as widget_new.c, EAX
 * is the object index.
 *
 * @address 0x004ffbe0
 */
void halo::objects::WidgetSystem::delete_all(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    datum_index handle = obj->first_widget;

    while (handle != (datum_index)0xffffffff) {
        uint16_t index = (uint16_t)handle;
        widget *entry = &((widget *)widget_data->data)[index];
        datum_index next = entry->next_widget;
        datum_index instance = entry->instance;

        if (instance != (datum_index)0xffffffff) {
            void (*delete_instance)(datum_index) =
                (void (*)(datum_index))widget_type_definitions[entry->type].delete_instance;
            delete_instance(instance);
        }
        datum_delete(widget_data, handle);
        handle = next;
    }

    obj->first_widget = (datum_index)0xffffffff;
}

/**
 * Returns whether any widget in a list carries the flag.
 *
 * @address 0x004ffc60
 */
int8_t halo::objects::WidgetSystem::list_has_flag(datum_index first_widget)
{
    datum_index handle = first_widget;

    if (handle == (datum_index)0xffffffff) {
        return 0;
    }

    for (;;) {
        widget *entry = &((widget *)widget_data->data)[handle & 0xffff];
        if (widget_type_definitions[entry->type].flag != 0) {
            return 1;
        }
        handle = entry->next_widget;
        if (handle == (datum_index)0xffffffff) {
            return 0;
        }
    }
}

namespace {
typedef void (*widget_render_proc)(uint32_t object_index, datum_index instance,
    uint32_t render_arg, void *render_context);
}

/**
 * Calls the render hook of every widget of an object.
 *
 * Original register convention: Ghidra shows a single unresolved `unaff_EDI`; by the object_data lookup shape shared
 * with widget_new.c / widget_delete_all.c, EDI is the object index.
 *
 * @address 0x004ffca0
 */
void halo::objects::WidgetSystem::list_notify(uint32_t object_index, uint32_t render_arg, void *render_context)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    datum_index handle = obj->first_widget;

    while (handle != (datum_index)0xffffffff) {
        widget *entry = &((widget *)widget_data->data)[handle & 0xffff];
        if (widget_type_definitions[entry->type].render != 0) {
            ((widget_render_proc)widget_type_definitions[entry->type].render)(
                object_index, entry->instance, render_arg, render_context);
        }
        handle = entry->next_widget;
    }
}

/**
 * Runs the update of every widget type for dt seconds.
 *
 * Original register convention: one stack parameter (dt), forwarded unchanged to each callback.
 *
 * @address 0x004ffd10
 */
void halo::objects::WidgetSystem::update_all(float dt)
{
    int32_t i;

    for (i = 0; i < k_maximum_widget_types; i++) {
        if (widget_type_definitions[i].update != 0) {
            ((void (*)(float))widget_type_definitions[i].update)(dt);
        }
    }
}
