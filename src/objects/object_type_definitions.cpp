#include "halo/objects/object_type_definitions.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"

extern "C" {
extern data_array *object_data;
extern network_id_table *object_network_id_table;
extern object_type_definition *object_type_definition_list;
extern object_type_definition *object_type_definitions[k_maximum_object_types];
}

/**
 * Builds the chain list of object type definitions from their sub-definitions.
 *
 * @address 0x004f3db0
 */
void halo::objects::ObjectTypeDefinitions::chain_build()
{
    object_type_definition **tail = &object_type_definition_list;
    object_type_definition *def;
    object_type_definition *sub;
    int16_t type_index;
    int16_t sub_index;

    for (type_index = 0; type_index < k_maximum_object_types; type_index++) {
        def = object_type_definitions[type_index];
        *tail = def;
        tail = &def->next;
        for (sub_index = 0; sub_index < k_maximum_object_subdefinitions; sub_index++) {
            sub = def->subdefinitions[sub_index];
            if (sub == 0) {
                break;
            }
            if (sub->next == 0) {
                *tail = sub;
                tail = &sub->next;
            }
        }
    }
    *tail = 0;

    for (def = object_type_definition_list; def != 0; def = def->next) {
        if (def->initialize != 0) {
            ((void (*)(void))def->initialize)();
        }
    }
}

/**
 * Broadcasts the creation notification (vtable slot 0x24) to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX -- read only for the type lookup, never passed to the
 * callee, so Ghidra could not see it as a declared parameter).
 *
 * @address 0x004f3e30
 */
void halo::objects::ObjectTypeDefinitions::notify_0x24(uint32_t object_index, uint32_t argument)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_created != 0) {
            ((void (*)(uint32_t, uint32_t))sub->notify_created)(object_index, argument);
        }
    }
}

/**
 * Queries vtable slot 0x28 across the object's type chain and returns the first answer.
 *
 * @address 0x004f3ea0
 */
uint8_t halo::objects::ObjectTypeDefinitions::query_0x28(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->query_create != 0) {
            uint8_t result = ((uint8_t (*)(uint32_t))sub->query_create)(object_index);
            if (result == 0) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * Broadcasts vtable slot 0x2c with an event argument to the object's type chain.
 *
 * Original register convention: stack -> object_index, event_argument.
 *
 * @address 0x004f3f20
 */
void halo::objects::ObjectTypeDefinitions::notify_two_args_0x2c(uint32_t object_index, uint32_t event_argument)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_two_args_2c != 0) {
            ((void (*)(uint32_t, uint32_t))sub->notify_two_args_2c)(object_index, event_argument);
        }
    }
}

/**
 * Broadcasts vtable slot 0x30 to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
 *
 * @address 0x004f3f90
 */
void halo::objects::ObjectTypeDefinitions::notify_0x30(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_delete != 0) {
            ((void (*)(uint32_t))sub->notify_delete)(object_index);
        }
    }
}

/**
 * Queries vtable slot 0x34 across the object's type chain.
 *
 * Original register convention: stack -> object_index.
 *
 * @address 0x004f4000
 */
uint8_t halo::objects::ObjectTypeDefinitions::query_0x34(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;
    int any_true = 0;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->query_34 != 0) {
            if (((int (*)(uint32_t))sub->query_34)(object_index) != 0) {
                any_true = 1;
            }
        }
    }
    return any_true;
}

/**
 * Broadcasts vtable slot 0x38 to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
 *
 * @address 0x004f4080
 */
void halo::objects::ObjectTypeDefinitions::notify_0x38(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_38 != 0) {
            ((void (*)(uint32_t))sub->notify_38)(object_index);
        }
    }
}

/**
 * Broadcasts vtable slot 0x3c with an argument to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
 *
 * @address 0x004f40f0
 */
void halo::objects::ObjectTypeDefinitions::notify_0x3c(uint32_t object_index, uint32_t argument)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_3c != 0) {
            ((void (*)(uint32_t, uint32_t))sub->notify_3c)(object_index, argument);
        }
    }
}

/**
 * Broadcasts the region damage notification to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
 *
 * @address 0x004f4160
 */
void halo::objects::ObjectTypeDefinitions::notify_region_damage(uint32_t object_index, uint32_t argument_1,
    uint32_t argument_2)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_region_damage != 0) {
            ((void (*)(uint32_t, uint32_t, uint32_t))sub->notify_region_damage)(object_index, argument_1, argument_2);
        }
    }
}

/**
 * Queries vtable slot 0x44 across the object's type chain.
 *
 * Original register convention: stack -> object_index.
 *
 * @address 0x004f41d0
 */
uint8_t halo::objects::ObjectTypeDefinitions::query_0x44(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;
    uint8_t any_true = 0;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->query_44 != 0) {
            if (((int (*)(uint32_t))sub->query_44)(object_index) != 0) {
                any_true = 1;
            }
        }
    }
    return any_true;
}

/**
 * Broadcasts vtable slot 0x48 with an event argument to the object's type chain.
 *
 * Original register convention: stack -> object_index, event_argument.
 *
 * @address 0x004f4250
 */
void halo::objects::ObjectTypeDefinitions::notify_two_args_0x48(uint32_t object_index, uint32_t event_argument)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_two_args_48 != 0) {
            ((void (*)(uint32_t, uint32_t))sub->notify_two_args_48)(object_index, event_argument);
        }
    }
}

/**
 * Broadcasts vtable slot 0x4c with an argument to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
 *
 * @address 0x004f42c0
 */
void halo::objects::ObjectTypeDefinitions::notify_0x4c(uint32_t object_index, uint32_t argument)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_4c != 0) {
            ((void (*)(uint32_t, uint32_t))sub->notify_4c)(object_index, argument);
        }
    }
}

/**
 * Broadcasts vtable slot 0x50 to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
 *
 * @address 0x004f4330
 */
void halo::objects::ObjectTypeDefinitions::notify_0x50(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_reset_scale != 0) {
            ((void (*)(uint32_t))sub->notify_reset_scale)(object_index);
        }
    }
}

/**
 * Broadcasts vtable slot 0x54 to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
 *
 * @address 0x004f43a0
 */
void halo::objects::ObjectTypeDefinitions::notify_0x54(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_54 != 0) {
            ((void (*)(uint32_t))sub->notify_54)(object_index);
        }
    }
}

/**
 * Broadcasts vtable slot 0x5c to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
 *
 * @address 0x004f4410
 */
void halo::objects::ObjectTypeDefinitions::notify_0x5c(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_5c != 0) {
            ((void (*)(uint32_t))sub->notify_5c)(object_index);
        }
    }
}

/**
 * Broadcasts vtable slot 0x58 with two arguments to the object's type chain.
 *
 * Original register convention: object index in EBX (unaff_EBX), not forwarded to the callee.
 *
 * @address 0x004f4480
 */
void halo::objects::ObjectTypeDefinitions::notify_0x58(uint32_t object_index, uint32_t argument_1,
    uint32_t argument_2)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_58 != 0) {
            ((void (*)(uint32_t, uint32_t, uint32_t))sub->notify_58)(object_index, argument_1, argument_2);
        }
    }
}

/**
 * Calls the last override of vtable slot 0x64 to fill a buffer.
 *
 * Original register convention: ESI -> object_index; stack -> buffer, buffer_size.
 *
 * @address 0x004f44f0
 */
int halo::objects::ObjectTypeDefinitions::override_get_0x64(uint32_t object_index, void *buffer, int32_t buffer_size)
{
    object *obj = halo::objects::object_try_and_get(object_index, _object_mask_all);
    object_type_definition *def;
    int16_t i;

    if (obj == 0) {
        return 0;
    }
    def = object_type_definitions[obj->type];
    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_get_64 != 0) {
            return ((int (*)(uint32_t, void *, int32_t))sub->override_get_64)(
                object_index, buffer, buffer_size);
        }
    }
    return 0;
}

/**
 * Calls the last override of vtable slot 0x68 in the object's type chain.
 *
 * Original register convention: none (void); operates on the "current object".
 *
 * @address 0x004f4560
 */
void halo::objects::ObjectTypeDefinitions::override_call_0x68(uint32_t object_index)
{
    object *obj = halo::objects::object_try_and_get(object_index, _object_mask_all);
    object_type_definition *def;
    int16_t i;

    if (obj == 0) {
        return;
    }
    def = object_type_definitions[obj->type];
    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_68 != 0) {
            ((void (*)(uint32_t))sub->override_call_68)(object_index);
            return;
        }
    }
}

/**
 * Calls the last override of vtable slot 0x6c with a buffer and bit budget.
 *
 * Original register convention: object index in EDI (unaff_EDI), resolved directly through object_data rather than
 * through object_try_and_get.
 *
 * @address 0x004f45b0
 */
int halo::objects::ObjectTypeDefinitions::override_call_0x6c(uint32_t object_index, void *buffer, int32_t bit_budget,
    int32_t full_update)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_6c != 0) {

            return ((int (*)(uint32_t, void *, int32_t, int32_t))sub->override_call_6c)(object_index, buffer, bit_budget, full_update);
        }
    }
    return 0;
}

/**
 * Calls the last override of vtable slot 0x70 with two arguments.
 *
 * Original register convention: ESI -> object_index, EDI -> edi_argument; stack -> stack_argument.
 *
 * @address 0x004f4620
 */
void halo::objects::ObjectTypeDefinitions::override_call_0x70(uint32_t object_index, uint32_t edi_argument,
    uint32_t stack_argument)
{
    object *obj = halo::objects::object_try_and_get(object_index, _object_mask_all);
    object_type_definition *def;
    int16_t i;

    if (obj == 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)edi_argument);
        return;
    }
    def = object_type_definitions[obj->type];
    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_70 != 0) {
            ((void (*)(uint32_t, uint32_t, uint32_t))sub->override_call_70)(
                object_index, edi_argument, stack_argument);
            return;
        }
    }
}

/**
 * Releases a pooled node record for a client through slot 0x70 handling.
 *
 * Original register convention: EAX -> record, stack -> param_1.
 *
 * @address 0x004f4680
 */
void halo::objects::ObjectTypeDefinitions::override_call_0x70_release_node(int32_t *record, uint32_t client)
{
    int32_t **slot = (int32_t **)((uint8_t *)record + 0x44);
    int32_t node = **slot;
    int32_t next = -1;

    if (node != 0) {
        next = ((int32_t *)object_network_id_table->handles)[node];
    }
    **slot = next;
    halo::objects::object_type_override_call_0x70(0, 0, 0);
}

/**
 * Calls the last override of vtable slot 0x74 and returns its answer.
 *
 * Original register convention: object index in EDI (unaff_EDI).
 *
 * @address 0x004f4700
 */
uint8_t halo::objects::ObjectTypeDefinitions::override_call_0x74(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_74 != 0) {
            return ((uint8_t (*)(uint32_t))sub->override_call_74)(object_index);
        }
    }
    return 1;
}

/**
 * Calls the last override of vtable slot 0x7c.
 *
 * Original register convention: object index in ESI (unaff_ESI).
 *
 * @address 0x004f4760
 */
void halo::objects::ObjectTypeDefinitions::override_call_0x7c(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_7c != 0) {
            ((void (*)(uint32_t))sub->override_call_7c)(object_index);
            return;
        }
    }
}
