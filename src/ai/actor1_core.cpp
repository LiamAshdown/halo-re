#include "halo/ai/actor_core.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

namespace c_actor_action_has_queued_secondary {
extern "C" {
extern data_array *actor_data;
}
}

extern "C" uint8_t actor_action_has_queued_secondary(datum_index actor_index);

/**
 * actor_action_has_queued_secondary: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_action_has_queued_secondary.c.txt.
 *
 * @address 0x417b70
 */
uint8_t halo::ai::actor_ref::action_has_queued_secondary()
{
    using namespace c_actor_action_has_queued_secondary;
    datum_index actor_index = datum;
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->secondary_action != (int16_t)-1) {
        return 1;
    }
    if (self->unit_index != (datum_index)k_datum_index_none && halo::units::unit_is_in_busy_animation_state(self->unit_index)) {
        return 1;
    }
    return 0;
}

extern "C" uint8_t actor_action_has_queued_secondary(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).action_has_queued_secondary();
}

namespace c_actor_apply_perception_scale {
extern "C" {
extern data_array *actor_data;
}
}

extern "C" uint8_t actor_apply_perception_scale(datum_index actor_index, const uint8_t *zone, float *in_out_value);

/**
 * actor_apply_perception_scale: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_apply_perception_scale.c.txt.
 *
 * @address 0x42aa90
 */
uint8_t halo::ai::actor_ref::apply_perception_scale(const uint8_t *zone, float *in_out_value)
{
    using namespace c_actor_apply_perception_scale;
    datum_index actor_index = datum;
    uint8_t scaled = 0;

    if (actor_index == (datum_index)k_datum_index_none) {
        return 0;
    }

    {
        actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

        if ((zone[4] & 8) != 0) {
            if (self->perception_scale > 0.0f) {
                scaled = 1;
                *in_out_value = *in_out_value * self->perception_scale;
            }
        }
        if (self->playfight != 0) {
            *in_out_value = *in_out_value * 0.3f;
            return 1;
        }
    }
    return scaled;
}

extern "C" uint8_t actor_apply_perception_scale(datum_index actor_index, const uint8_t *zone, float *in_out_value)
{
    return halo::ai::actor_ref(actor_index).apply_perception_scale(zone, in_out_value);
}

namespace c_actor_attach_to_unit {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *encounter_data;

extern void actor_unlink_unit(datum_index actor_index);
extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index);
extern void actor_delete(datum_index actor_index, uint32_t flag);
extern void actor_refresh_combat_context(datum_index actor_index);
extern void ai_encounter_stamp_team_from_unit(datum_index encounter_index, datum_index unit_index);
}
}

extern "C" void actor_attach_to_unit(datum_index actor_index, datum_index unit_index);

/**
 * actor_attach_to_unit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_attach_to_unit.c.txt.
 *
 * @address 0x427560
 */
void halo::ai::actor_ref::attach_to_unit(datum_index unit_index)
{
    using namespace c_actor_attach_to_unit;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    object *unit_object = header->data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

    if (unit->actor_index == actor_index) {
        return;
    }

    if (unit->swarm_actor_index != (datum_index)k_datum_index_none) {
        actor_remove_from_unit_cluster(unit->swarm_actor_index, unit_index);
    }
    if (unit->actor_index != (datum_index)k_datum_index_none) {
        actor_delete(unit->actor_index, 0);
    }
    if (self->unit_index != (datum_index)k_datum_index_none) {
        actor_unlink_unit(actor_index);
    }

    self->unit_index = unit_index;
    unit->actor_index = actor_index;

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
        ai_encounter_stamp_team_from_unit(self->encounter_index, unit_index);
        ((struct object *)unit_object)->owner_team = enc->team;
    }
    self->team = ((struct object *)unit_object)->owner_team;

    if (*(int16_t *)((uint8_t *)unit_object + 0xbe) > 99) {
        self->counts_toward_encounter = 1;
        if (self->encounter_index != (datum_index)k_datum_index_none) {
            encounter *enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
            enc->live_count = enc->live_count + 1;
        }
    }

    actor_refresh_combat_context(actor_index);

    {
        uint8_t flags_before = header->flags;
        header->flags = flags_before & ~_object_header_in_pvs_pass_bit;
        if ((flags_before & _object_header_active_bit) == 0) {
            halo::objects::object_mark_pending_delete(unit_index);
        }
        if (self->keep_unit_alive == 0) {
            halo::objects::object_mark_pending_delete(unit_index);
        } else if ((header->flags & _object_header_active_bit) != 0) {
            header->flags &= ~_object_header_active_bit;
        }
    }

    halo::units::unit_refresh_targeting_flag_and_weapons(unit_index, 1);
}

extern "C" void actor_attach_to_unit(datum_index actor_index, datum_index unit_index)
{
    halo::ai::actor_ref(actor_index).attach_to_unit(unit_index);
}

namespace c_actor_classify_communication_object_type {
extern "C" {
extern data_array *actor_data;
extern void *actor_type_procs[16];
}
}

extern "C" int32_t actor_classify_communication_object_type(datum_index actor_index);

/**
 * actor_classify_communication_object_type: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_classify_communication_object_type.c.txt.
 *
 * @address 0x42f9a0
 */
int32_t halo::ai::actor_ref::classify_communication_object_type()
{
    using namespace c_actor_classify_communication_object_type;
    datum_index actor_index = datum;
    actor *a;
    uint16_t flags;
    int32_t result;

    a = &((actor *)actor_data->data)[actor_index & 0xffff];
    flags = *(uint16_t *)((uint8_t *)actor_type_procs[a->type] + 4);

    result = -1;
    if ((flags & 2) != 0) {
        return 0;
    }
    if ((flags & 4) != 0) {
        result = 1;
    }
    return result;
}

extern "C" int32_t actor_classify_communication_object_type(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).classify_communication_object_type();
}

namespace c_actor_command_list_permits_escalation {
extern "C" {
extern data_array *actor_data;
extern data_array *ai_conversation_data;
}
}

extern "C" uint8_t actor_command_list_permits_escalation(datum_index actor_index);

/**
 * actor_command_list_permits_escalation: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_command_list_permits_escalation.c.txt.
 *
 * @address 0x40d580
 */
uint8_t halo::ai::actor_ref::command_list_permits_escalation()
{
    using namespace c_actor_command_list_permits_escalation;
    datum_index actor_index = datum;
    actor *self;
    ai_conversation *conv;
    ScenarioAIConversation *definition;
    ScenarioAIConversationFlags flags;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->conversation_index == (datum_index)k_datum_index_none) {
        return 1;
    }

    conv = (ai_conversation *)((uint8_t *)ai_conversation_data->data +
                                (self->conversation_index & 0xffff) * sizeof(ai_conversation));
    definition = (ScenarioAIConversation *)((TagReflexive *)((uint8_t *)halo::scenario::globals().scenario + 0x468))->pointer;
    definition = definition + conv->definition_index;
    flags = definition->flags;

    if ((((flags & 2) == 0 || self->tally.by_threat_class[8] == 0) &&
         ((flags & 4) == 0 || self->target_combat_status < 9)) &&
        ((flags & 8) == 0 || self->target_combat_status < 6)) {
        return 0;
    }
    return 1;
}

extern "C" uint8_t actor_command_list_permits_escalation(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).command_list_permits_escalation();
}

namespace c_actor_command_list_reset_record {
}

extern "C" void actor_command_list_reset_record(uint32_t actor_index, datum_index unit_index, uint16_t extra, void *component_record, int32_t secondary_record, uint32_t callback_extra);

/**
 * actor_command_list_reset_record: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_command_list_reset_record.c.txt.
 *
 * @address 0x406dd0
 */
void halo::ai::actor_ref::command_list_reset_record(datum_index unit_index, uint16_t extra, void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    using namespace c_actor_command_list_reset_record;
    uint32_t actor_index = datum;
    uint8_t *record = (uint8_t *)component_record;
    uint8_t *secondary = (uint8_t *)(uintptr_t)secondary_record;

    (void)actor_index;
    (void)unit_index;
    (void)extra;
    memset(record, 0, 0x24);
    record[0] = 0xff;
    if (*(uint8_t *)(uintptr_t)callback_extra != 0) {
        record[4] |= 1;
    } else {
        record[4] &= 0xfe;
    }
    if (secondary != 0) {
        memset(secondary, 0, 0x58);
        *(int16_t *)(secondary + 2) = -1;
    }
}

extern "C" void actor_command_list_reset_record(uint32_t actor_index, datum_index unit_index, uint16_t extra, void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    halo::ai::actor_ref(actor_index).command_list_reset_record(unit_index, extra, component_record, secondary_record, callback_extra);
}

namespace c_actor_delete {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

extern void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters);
extern void ai_actor_unlink_from_unassigned_list(datum_index actor_index);
extern void actor_unlink_unit(datum_index actor_index);
extern void actor_delete_swarm(datum_index actor_index);
extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index);
extern void actor_clear_perceived_props(datum_index actor_index);
extern void ai_conversation_clear_participant(datum_index actor_index);
}
}

extern "C" void actor_delete(datum_index actor_index, uint32_t flag);

/**
 * actor_delete: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_delete.c.txt.
 *
 * @address 0x427e60
 */
void halo::ai::actor_ref::delete_(uint32_t flag)
{
    using namespace c_actor_delete;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    data_iterator iterator;
    prop *p;

    if (self->encounterless == 0) {
        encounter_remove_actor(actor_index, (uint8_t)flag);
    } else {
        ai_actor_unlink_from_unassigned_list(actor_index);
    }

    if (self->swarm == 0) {
        actor_unlink_unit(actor_index);
    } else {
        actor_delete_swarm(actor_index);
        while (self->cluster_unit_index != (datum_index)k_datum_index_none) {
            actor_remove_from_unit_cluster(actor_index, self->cluster_unit_index);
        }
    }

    actor_clear_perceived_props(actor_index);

    iterator.data = prop_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (prop *)halo::memory::data_iterator_next(&iterator);
    while (p != 0) {
        if (p->owner_actor_index == actor_index) {
            p->owner_actor_index = (datum_index)k_datum_index_none;
        }
        p = (prop *)halo::memory::data_iterator_next(&iterator);
    }

    ai_conversation_clear_participant(actor_index);
    halo::memory::datum_delete(actor_data, actor_index);
}

extern "C" void actor_delete(datum_index actor_index, uint32_t flag)
{
    halo::ai::actor_ref(actor_index).delete_(flag);
}

namespace c_actor_delete_or_release_unit {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;

extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index);
extern void actor_delete(datum_index actor_index, uint32_t flag);
extern uint8_t actor_attempt_grenade_throw(datum_index actor_index);
}
}

extern "C" void actor_delete_or_release_unit(datum_index actor_index, uint8_t is_dead);

/**
 * actor_delete_or_release_unit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_delete_or_release_unit.c.txt.
 *
 * @address 0x4288e0
 */
void halo::ai::actor_ref::delete_or_release_unit(uint8_t is_dead)
{
    using namespace c_actor_delete_or_release_unit;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm == 0) {
        datum_index unit_index = self->unit_index;
        actor_attempt_grenade_throw(actor_index);
        if (is_dead != 0) {
            halo::objects::object_delete_recursive(unit_index, 0);
            halo::objects::object_delete_4f9030(unit_index, 0);
            return;
        }
        halo::objects::object_delete(unit_index);
        return;
    }

    for (;;) {
        datum_index unit_index = self->cluster_unit_index;
        if (unit_index == (datum_index)k_datum_index_none) {
            actor_delete(actor_index, 1);
            return;
        }
        actor_remove_from_unit_cluster(actor_index, unit_index);
        if (is_dead == 0) {
            object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
            int32_t network_role = unit_object->network_role;
            if (network_role == 0) {
                halo::objects::object_delete_unparented(unit_index);
                halo::objects::object_delete_recursive(unit_index, 0);
            } else if (network_role == 3) {
                halo::objects::object_delete_recursive(unit_index, 0);
            }
        } else {
            halo::objects::object_delete_recursive(unit_index, 0);
            halo::objects::object_delete_4f9030(unit_index, 0);
        }
    }
}

extern "C" void actor_delete_or_release_unit(datum_index actor_index, uint8_t is_dead)
{
    halo::ai::actor_ref(actor_index).delete_or_release_unit(is_dead);
}

namespace c_actor_dispatch_perception_reset {
extern "C" {
extern data_array *actor_data;
extern data_array *swarm_data;

extern void actor_reset_perception_scratch(datum_index unit_index);
}
}

extern "C" void actor_dispatch_perception_reset(datum_index actor_index);

/**
 * actor_dispatch_perception_reset: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_dispatch_perception_reset.c.txt.
 *
 * @address 0x429000
 */
void halo::ai::actor_ref::dispatch_perception_reset()
{
    using namespace c_actor_dispatch_perception_reset;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->swarm == 0) {
        actor_reset_perception_scratch(self->unit_index);
        self->unit_control_pending = 1;
        return;
    }

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        int16_t i;
        for (i = 0; i < s->component_count; i++) {
            actor_reset_perception_scratch(s->unit_index[i]);
        }
    }
    self->unit_control_pending = 1;
}

extern "C" void actor_dispatch_perception_reset(datum_index actor_index)
{
    halo::ai::actor_ref(actor_index).dispatch_perception_reset();
}

namespace c_actor_dispatch_squad_order {
extern "C" {
extern data_array *prop_data;
extern data_array *actor_data;

extern void actor_queue_search_and_relay_perception(datum_index prop_index, datum_index actor_index);
extern void actor_scan_ally_death_panic_reaction(datum_index target_prop_index, datum_index actor_index);
extern uint8_t actor_target_data_acquire(datum_index actor_index, datum_index object_index,
    datum_index owner_reference, datum_index pair_reference);
}
}

extern "C" void actor_dispatch_squad_order(datum_index prop_index, const actor_squad_order_header *order, datum_index actor_index);

/**
 * actor_dispatch_squad_order: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_dispatch_squad_order.c.txt.
 *
 * @address 0x42a540
 */
void halo::ai::actor_ref::dispatch_squad_order(datum_index prop_index, const actor_squad_order_header *order, datum_index actor_index)
{
    using namespace c_actor_dispatch_squad_order;
    if (order == 0) {
        return;
    }

    if (order->type == 2) {
        actor_queue_search_and_relay_perception(prop_index, actor_index);
    } else if (order->type == 3) {

        prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
        if (p->owner_actor_index != (datum_index)k_datum_index_none) {
            datum_index ordered = *(datum_index *)((uint8_t *)order + 0x18);
            prop *other = (prop *)halo::memory::datum_get(ordered, prop_data);

            if (other != 0) {
                actor_target_data_acquire(actor_index, other->object_index, p->owner_actor_index, ordered);
            }
        }
    } else if (order->type == 4) {
        actor_scan_ally_death_panic_reaction(prop_index, actor_index);
    }
}

extern "C" void actor_dispatch_squad_order(datum_index prop_index, const actor_squad_order_header *order, datum_index actor_index)
{
    halo::ai::actor_ref::dispatch_squad_order(prop_index, order, actor_index);
}

namespace c_actor_dispatch_type_vtable_0x10 {
extern "C" {
extern data_array *actor_data;
extern void *actor_type_procs[16];
}
}

extern "C" void actor_dispatch_type_vtable_0x10(datum_index actor_index);

/**
 * actor_dispatch_type_vtable_0x10: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_dispatch_type_vtable_0x10.c.txt.
 *
 * @address 0x426670
 */
void halo::ai::actor_ref::dispatch_type_vtable_0x10()
{
    using namespace c_actor_dispatch_type_vtable_0x10;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    actor_type_table_entry *entry = (actor_type_table_entry *)actor_type_procs[self->type];

    if (entry->proc_10 != 0) {
        ((void (*)(datum_index))entry->proc_10)(actor_index);
    }
}

extern "C" void actor_dispatch_type_vtable_0x10(datum_index actor_index)
{
    halo::ai::actor_ref(actor_index).dispatch_type_vtable_0x10();
}

namespace c_actor_dispatch_type_vtable_0x18 {
extern "C" {
extern data_array *actor_data;
extern void *actor_type_procs[16];
}
}

extern "C" void actor_dispatch_type_vtable_0x18(datum_index actor_index);

/**
 * actor_dispatch_type_vtable_0x18: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_dispatch_type_vtable_0x18.c.txt.
 *
 * @address 0x4266a0
 */
void halo::ai::actor_ref::dispatch_type_vtable_0x18()
{
    using namespace c_actor_dispatch_type_vtable_0x18;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    actor_type_table_entry *entry = (actor_type_table_entry *)actor_type_procs[self->type];

    ((void (*)(datum_index))entry->proc_18)(actor_index);
}

extern "C" void actor_dispatch_type_vtable_0x18(datum_index actor_index)
{
    halo::ai::actor_ref(actor_index).dispatch_type_vtable_0x18();
}

namespace c_actor_dispatch_type_vtable_0x1c {
extern "C" {
extern data_array *actor_data;
extern void *actor_type_procs[16];
}
}

extern "C" void actor_dispatch_type_vtable_0x1c(datum_index actor_index, uint32_t a, uint32_t b, uint32_t c);

/**
 * actor_dispatch_type_vtable_0x1c: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_dispatch_type_vtable_0x1c.c.txt.
 *
 * @address 0x4266d0
 */
void halo::ai::actor_ref::dispatch_type_vtable_0x1c(uint32_t a, uint32_t b, uint32_t c)
{
    using namespace c_actor_dispatch_type_vtable_0x1c;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    actor_type_table_entry *entry = (actor_type_table_entry *)actor_type_procs[self->type];

    if (entry->proc_1c != 0) {
        ((void (*)(datum_index, uint32_t, uint32_t, uint32_t))entry->proc_1c)(actor_index, a, b, c);
    }
}

extern "C" void actor_dispatch_type_vtable_0x1c(datum_index actor_index, uint32_t a, uint32_t b, uint32_t c)
{
    halo::ai::actor_ref(actor_index).dispatch_type_vtable_0x1c(a, b, c);
}

namespace c_actor_get_actor_definition {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index);
}
}

extern "C" void * actor_get_actor_definition(datum_index actor_index);

/**
 * actor_get_actor_definition: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_actor_definition.c.txt.
 *
 * @address 0x40fa70
 */
void * halo::ai::actor_ref::get_actor_definition()
{
    using namespace c_actor_get_actor_definition;
    datum_index actor_index = datum;
    actor *self;
    void *default_definition;
    datum_index weapon_object;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    default_definition = halo::cache::globals().tag_instances[self->actor_variant_tag & 0xffff].data;

    weapon_object = actor_get_threat_weapon_object_index(actor_index);
    if (weapon_object != (datum_index)k_datum_index_none) {
        object_header *hdr = (object_header *)object_data->data + (weapon_object & 0xffff);
        object *obj = hdr->data;
        void *weapon_definition = halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;
        if (weapon_definition != 0) {
            uint32_t override_index = *(uint32_t *)((uint8_t *)weapon_definition + 0x3c8);
            if (override_index != (uint32_t)-1) {
                return halo::cache::globals().tag_instances[override_index & 0xffff].data;
            }
        }
    }
    return default_definition;
}

extern "C" void * actor_get_actor_definition(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).get_actor_definition();
}

namespace c_actor_get_body_axis_vector {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;


}
}

extern "C" void actor_get_body_axis_vector(uint32_t actor_index, uint32_t unit_index, actor_axis_request *request);

/**
 * actor_get_body_axis_vector: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_body_axis_vector.c.txt.
 *
 * @address 0x405390
 */
void halo::ai::actor_ref::get_body_axis_vector(uint32_t unit_index, actor_axis_request *request)
{
    using namespace c_actor_get_body_axis_vector;
    uint32_t actor_index = datum;
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    real_vector3d reference;

    if (unit_index == a->unit_index) {
        reference.i = a->facing.i;
        reference.j = a->facing.j;
        reference.k = a->facing.k;
    } else {

        halo::units::unit_get_forward_vector_or_marker_normal(unit_index, &reference);
    }

    switch (request->axis) {
    case 0:
        request->result = reference;
        return;
    case 1:
        request->result.i = -reference.i;
        request->result.j = -reference.j;
        request->result.k = -reference.k;
        return;
    case 2:
    case 3:
        {
            real_vector3d perp;

            halo::math::vector3d_cross_product(perp, reference, *halo::math::globals().global_up3d_pointer);
            if (halo::math::vector3d_normalize_with_length(perp) == 0.0f) {
                object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;

                halo::math::vector3d_cross_product(perp, reference, obj->up);
                if (halo::math::vector3d_normalize_with_length(perp) == 0.0f) {
                    perp = *halo::math::globals().global_forward3d_pointer;
                }
            }
            if (request->axis == 2) {
                request->result = perp;
            } else {
                request->result.i = -perp.i;
                request->result.j = -perp.j;
                request->result.k = -perp.k;
            }
        }
        return;
    }
}

extern "C" void actor_get_body_axis_vector(uint32_t actor_index, uint32_t unit_index, actor_axis_request *request)
{
    halo::ai::actor_ref(actor_index).get_body_axis_vector(unit_index, request);
}

namespace c_actor_get_current_mode_combat_grade {
extern "C" {
extern data_array *actor_data;
extern actor_mode_definition actor_mode_definitions[16];
}
}

extern "C" int16_t actor_get_current_mode_combat_grade(datum_index actor_index);

/**
 * actor_get_current_mode_combat_grade: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_current_mode_combat_grade.c.txt.
 *
 * @address 0x40e760
 */
int16_t halo::ai::actor_ref::get_current_mode_combat_grade()
{
    using namespace c_actor_get_current_mode_combat_grade;
    datum_index actor_index = datum;
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    return actor_mode_definitions[self->mode].combat_grade;
}

extern "C" int16_t actor_get_current_mode_combat_grade(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).get_current_mode_combat_grade();
}

namespace c_actor_get_ranged_attack_vector {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *object_data;

extern uint8_t actor_get_cached_wander_position(datum_index actor_index, real_vector3d *out_position);
}
}

extern "C" uint8_t actor_get_ranged_attack_vector(datum_index target_prop_index, datum_index actor_index, real_vector3d *out_vector);

/**
 * actor_get_ranged_attack_vector: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_ranged_attack_vector.c.txt.
 *
 * @address 0x420970
 */
uint8_t halo::ai::actor_ref::get_ranged_attack_vector(datum_index target_prop_index, datum_index actor_index, real_vector3d *out_vector)
{
    using namespace c_actor_get_ranged_attack_vector;
    actor *self;
    prop *target;
    object *unit_obj;
    unit_data *unit;
    datum_index prop_index;
    prop *ally;
    real_vector3d delta;
    float length;
    float dot;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

    if (target->swarm_owned != 0) {
        return 0;
    }

    if (target->is_parented == 0) {
        if (target->owner_actor_index != k_datum_index_none) {
            return actor_get_cached_wander_position(target->owner_actor_index, out_vector);
        }
        return 0;
    }

    unit_obj = ((object_header *)object_data->data)[target->object_index & 0xffff].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    *out_vector = unit->aiming_vector;

    if (target->shooting == 0 && (int8_t)self->tally.unit_props > 0) {
        prop_index = self->first_prop;
        while (prop_index != k_datum_index_none) {
            ally = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));
            prop_index = ally->next_in_actor;

            if ((1 < ally->state && ally->state < 4) && ally->enemy != 0) {
                delta.i = ally->last_known_position.x - target->last_known_position.x;
                delta.j = ally->last_known_position.y - target->last_known_position.y;
                delta.k = ally->last_known_position.z - target->last_known_position.z;
                length = halo::math::vector3d_normalize_with_length(delta);

                if (length > 0.0f) {
                    dot = delta.i * out_vector->i + delta.j * out_vector->j + delta.k * out_vector->k;
                    if (dot > 0.5f) {
                        return 1;
                    }
                }
            }
        }
        return 0;
    }

    return target->shooting;
}

extern "C" uint8_t actor_get_ranged_attack_vector(datum_index target_prop_index, datum_index actor_index, real_vector3d *out_vector)
{
    return halo::ai::actor_ref::get_ranged_attack_vector(target_prop_index, actor_index, out_vector);
}

namespace c_actor_handle_death {
extern "C" {
extern data_array *actor_data;

extern uint16_t actor_consider_target_candidate(datum_index actor_index,
                                                datum_index candidate_prop_index);

extern void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order);
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data);
}
}

extern "C" uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3);

/**
 * actor_handle_death: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_handle_death.c.txt.
 *
 * @address 0x40dd50
 */
uint8_t halo::ai::actor_ref::handle_death(uint8_t param_2, uint8_t param_3)
{
    using namespace c_actor_handle_death;
    datum_index actor_index = datum;
    actor *self;
    uint8_t local_data[0x30];
    int32_t previous_target;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->order_committed != 0) {
        return 0;
    }

    previous_target = self->target_unit_index;

    for (uint32_t i = 0; i < sizeof(local_data) / 4; i++) {
        ((uint32_t *)local_data)[i] = 0;
    }
    *(int16_t *)(local_data + 0xc) = 0;
    *(int16_t *)(local_data + 0) = 0;
    *(int16_t *)(local_data + 8) = -1;
    local_data[4] = param_2;
    local_data[5] = param_3;
    *(int32_t *)(local_data + 0x1c) = previous_target;

    if (previous_target != -1) {
        actor_consider_target_candidate(actor_index, (datum_index)previous_target);
    }
    if (self->swarm == 0) {
        actor_check_melee_target_reachable(actor_index, (int16_t *)local_data);
        if (*(int16_t *)(local_data + 8) != -1) {
            actor_set_mode(actor_index, 4, local_data);
            return 1;
        }
    }
    return 0;
}

extern "C" uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3)
{
    return halo::ai::actor_ref(actor_index).handle_death(param_2, param_3);
}

namespace c_actor_invoke_type_handler {
extern "C" {
extern data_array *actor_data;
extern actor_mode_definition actor_mode_definitions[16];
}
}

extern "C" void actor_invoke_type_handler(uint32_t actor_index);

/**
 * actor_invoke_type_handler: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_invoke_type_handler.c.txt.
 *
 * @address 0x409e70
 */
void halo::ai::actor_ref::invoke_type_handler()
{
    using namespace c_actor_invoke_type_handler;
    uint32_t actor_index = datum;
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    void (*update_proc)(uint32_t) = (void (*)(uint32_t))actor_mode_definitions[a->mode].update_proc;

    if (update_proc != 0) {
        update_proc(actor_index);
    }
}

extern "C" void actor_invoke_type_handler(uint32_t actor_index)
{
    halo::ai::actor_ref(actor_index).invoke_type_handler();
}

namespace c_actor_iterator_new {
extern "C" {
extern data_array *encounter_data;
extern ai_globals *ai_globals_ptr;
}
}

extern "C" void actor_iterator_new(actor_iterator_state *out_iterator, uint8_t active_only);

/**
 * actor_iterator_new: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_iterator_new.c.txt.
 *
 * @address 0x436a30
 */
void halo::ai::actor_ref::iterator_new(actor_iterator_state *out_iterator, uint8_t active_only)
{
    using namespace c_actor_iterator_new;
    if (ai_globals_ptr->actors_valid != 0) {
        out_iterator->filter_array = encounter_data;
        out_iterator->next_index = 0;
        out_iterator->cursor = -1;
        out_iterator->signature = (uint32_t)encounter_data ^ 0x69746572;
        out_iterator->encounterless_done = 0;
        out_iterator->active = active_only;
        out_iterator->actor_index = (datum_index)k_datum_index_none;
        out_iterator->next_actor_index = -1;
    }
}

extern "C" void actor_iterator_new(actor_iterator_state *out_iterator, uint8_t active_only)
{
    halo::ai::actor_ref::iterator_new(out_iterator, active_only);
}

namespace c_actor_iterator_next {
extern "C" {
extern ai_globals *ai_globals_ptr;
extern data_array *actor_data;

}
}

extern "C" actor * actor_iterator_next(actor_iterator_state *iterator);

/**
 * actor_iterator_next: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_iterator_next.c.txt.
 *
 * @address 0x436a70
 */
actor * halo::ai::actor_ref::iterator_next(actor_iterator_state *iterator)
{
    using namespace c_actor_iterator_next;
    datum_index next;
    actor *a;

    if (ai_globals_ptr->actors_valid == 0) {
        return 0;
    }

    next = iterator->next_actor_index;
    while (next == (datum_index)k_datum_index_none) {
        encounter *enc = (encounter *)halo::memory::data_iterator_next((data_iterator *)iterator);
        if (enc == 0) {
            if (iterator->encounterless_done == 0) {
                iterator->next_actor_index = ai_globals_ptr->first_encounterless_actor;
                iterator->encounterless_done = 1;
            }
            break;
        }
        if (iterator->active == 0 || enc->units_active != 0) {
            iterator->next_actor_index = enc->first_actor;
        }
        next = iterator->next_actor_index;
    }

    do {
        next = iterator->next_actor_index;
        iterator->actor_index = next;
        if (next == (datum_index)k_datum_index_none) {
            return 0;
        }
        a = &((actor *)actor_data->data)[next & 0xffff];
        iterator->next_actor_index = a->next_in_encounter;
    } while (iterator->active != 0 && a->active == 0);

    return a;
}

extern "C" actor * actor_iterator_next(actor_iterator_state *iterator)
{
    return halo::ai::actor_ref::iterator_next(iterator);
}

namespace c_actor_link_to_unit_cluster {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *swarm_data;
extern data_array *swarm_component_data;
extern data_array *encounter_data;

extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index);
extern void actor_delete(datum_index actor_index, uint32_t flag);
extern void actor_unlink_unit(datum_index actor_index);
extern void swarm_add_component(datum_index component_index, uint32_t unit_index, datum_index swarm_index);
extern void ai_encounter_stamp_team_from_unit(datum_index encounter_index, datum_index unit_index);
}
}

extern "C" uint8_t actor_link_to_unit_cluster(datum_index actor_index, datum_index unit_index);

/**
 * actor_link_to_unit_cluster: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_link_to_unit_cluster.c.txt.
 *
 * @address 0x4279f0
 */
uint8_t halo::ai::actor_ref::link_to_unit_cluster(datum_index unit_index)
{
    using namespace c_actor_link_to_unit_cluster;
    datum_index actor_index = datum;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    object *unit_object = header->data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
    datum_index new_component = (datum_index)k_datum_index_none;

    if (unit->swarm_actor_index == actor_index) {
        return 1;
    }

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        new_component = halo::memory::datum_new(swarm_component_data);
        if (new_component == (datum_index)k_datum_index_none) {
            return 0;
        }
    }

    if (unit->swarm_actor_index != (datum_index)k_datum_index_none) {
        actor_remove_from_unit_cluster(unit->swarm_actor_index, unit_index);
    }
    if (unit->actor_index != (datum_index)k_datum_index_none) {
        actor_delete(unit->actor_index, 0);
    }
    if (self->unit_index != (datum_index)k_datum_index_none) {
        actor_unlink_unit(actor_index);
    }

    unit->swarm_actor_index = actor_index;
    unit->swarm_next_unit_index = self->cluster_unit_index;
    *(uint32_t *)((uint8_t *)unit_object + 0x200) = 0xffffffff;
    if (self->cluster_unit_index != (datum_index)k_datum_index_none) {
        object *head_object = ((object_header *)object_data->data)[self->cluster_unit_index & 0xffff].data;
        *(uint32_t *)((uint8_t *)head_object + 0x200) = unit_index;
    }
    self->cluster_unit_index = unit_index;

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        swarm_add_component(new_component, unit_index, self->swarm_index);
    }

    self->cluster_count = self->cluster_count + 1;
    self->total_cluster_count = self->total_cluster_count + 1;

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
        ai_encounter_stamp_team_from_unit(self->encounter_index, unit_index);
        ((struct object *)unit_object)->owner_team = enc->team;
    }
    self->team = ((struct object *)unit_object)->owner_team;

    {
        uint8_t flags_before = header->flags;
        header->flags = flags_before & ~_object_header_in_pvs_pass_bit;
        if ((flags_before & _object_header_active_bit) == 0) {
            halo::objects::object_mark_pending_delete(unit_index);
        }
        if (self->keep_unit_alive == 0) {
            halo::objects::object_mark_pending_delete(unit_index);
        } else if ((header->flags & _object_header_active_bit) != 0) {
            header->flags &= ~_object_header_active_bit;
        }
    }

    halo::units::unit_refresh_targeting_flag_and_weapons(unit_index, 1);
    return 1;
}

extern "C" uint8_t actor_link_to_unit_cluster(datum_index actor_index, datum_index unit_index)
{
    return halo::ai::actor_ref(actor_index).link_to_unit_cluster(unit_index);
}

namespace c_actor_mark_units_and_release {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;

extern void actor_unlink_unit(datum_index actor_index);
extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index);
extern void actor_delete(datum_index actor_index, uint32_t flag);
extern void encounter_recompute_morale(datum_index encounter_index);
}
}

extern "C" void actor_mark_units_and_release(uint8_t use_alternate_flag, datum_index actor_index, uint8_t suppress_release);

/**
 * actor_mark_units_and_release: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mark_units_and_release.c.txt.
 *
 * @address 0x4289c0
 */
void halo::ai::actor_ref::mark_units_and_release(uint8_t use_alternate_flag, datum_index actor_index, uint8_t suppress_release)
{
    using namespace c_actor_mark_units_and_release;
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    datum_index encounter_index = self->encounter_index;

    if (self->swarm == 0) {
        object *unit_object = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
        uint8_t *flags = (uint8_t *)unit_object + 0x106;
        *flags |= use_alternate_flag == 0 ? 0x20 : 0x40;

        if (suppress_release != 0) {
            return;
        }
        actor_unlink_unit(actor_index);
    } else {
        datum_index unit_index = self->cluster_unit_index;
        while (unit_index != (datum_index)k_datum_index_none) {
            object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
            uint8_t *flags = (uint8_t *)unit_object + 0x106;
            *flags |= use_alternate_flag == 0 ? 0x20 : 0x40;

            if (suppress_release == 0) {
                actor_remove_from_unit_cluster(actor_index, unit_index);
            }
            unit_index = *(datum_index *)((uint8_t *)unit_object + 0x1fc);
        }
        if (suppress_release != 0) {
            return;
        }
    }

    actor_delete(actor_index, 1);
    if (encounter_index != (datum_index)k_datum_index_none) {
        encounter_recompute_morale(encounter_index);
    }
}

extern "C" void actor_mark_units_and_release(uint8_t use_alternate_flag, datum_index actor_index, uint8_t suppress_release)
{
    halo::ai::actor_ref::mark_units_and_release(use_alternate_flag, actor_index, suppress_release);
}

