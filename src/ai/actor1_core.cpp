#include "halo/ai/actor_core.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/ai/ai_constants.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

namespace c_actor_action_has_queued_secondary {
}


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
    actor *self = halo::ai::actor_at(actor_index);

    if (self->secondary_action != (int16_t)-1) {
        return 1;
    }
    if (self->unit_index != (datum_index)k_datum_index_none && halo::units::unit_is_in_busy_animation_state(self->unit_index)) {
        return 1;
    }
    return 0;
}

namespace halo::ai {
uint8_t actor_action_has_queued_secondary(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).action_has_queued_secondary();
}
}

namespace c_actor_apply_perception_scale {
}


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
        actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

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

namespace halo::ai {
uint8_t actor_apply_perception_scale(datum_index actor_index, const uint8_t *zone, float *in_out_value)
{
    return halo::ai::actor_ref(actor_index).apply_perception_scale(zone, in_out_value);
}
}

namespace c_actor_attach_to_unit {
}


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
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    object_header *header = &((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask];
    object *unit_object = header->data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

    if (unit->actor_index == actor_index) {
        return;
    }

    if (unit->swarm_actor_index != (datum_index)k_datum_index_none) {
        halo::ai::actor_remove_from_unit_cluster(unit->swarm_actor_index, unit_index);
    }
    if (unit->actor_index != (datum_index)k_datum_index_none) {
        halo::ai::actor_delete(unit->actor_index, 0);
    }
    if (self->unit_index != (datum_index)k_datum_index_none) {
        halo::ai::actor_unlink_unit(actor_index);
    }

    self->unit_index = unit_index;
    unit->actor_index = actor_index;

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[self->encounter_index & halo::k_slot_mask];
        halo::ai::ai_encounter_stamp_team_from_unit(self->encounter_index, unit_index);
        unit_object->owner_team = enc->team;
    }
    self->team = unit_object->owner_team;

    if (*(int16_t *)((uint8_t *)unit_object + 0xbe) > 99) {
        self->counts_toward_encounter = 1;
        if (self->encounter_index != (datum_index)k_datum_index_none) {
            encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[self->encounter_index & halo::k_slot_mask];
            enc->live_count = enc->live_count + 1;
        }
    }

    halo::ai::actor_refresh_combat_context(actor_index);

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

namespace halo::ai {
void actor_attach_to_unit(datum_index actor_index, datum_index unit_index)
{
    halo::ai::actor_ref(actor_index).attach_to_unit(unit_index);
}
}

namespace c_actor_classify_communication_object_type {
static auto &actor_type_procs = halo::link::ref<actor_type_table_entry *[16]>(halo::ai::vars().actor_type_procs);
}


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

    a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    flags = actor_type_procs[a->type]->flags;

    result = -1;
    if ((flags & 2) != 0) {
        return 0;
    }
    if ((flags & 4) != 0) {
        result = 1;
    }
    return result;
}

namespace halo::ai {
int32_t actor_classify_communication_object_type(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).classify_communication_object_type();
}
}

namespace c_actor_command_list_permits_escalation {
}


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

    self = halo::ai::actor_at(actor_index);

    if (self->conversation_index == (datum_index)k_datum_index_none) {
        return 1;
    }

    conv = halo::ai::conversation_at(self->conversation_index);
    definition = (ScenarioAIConversation *)halo::scenario::globals().scenario->ai_conversations.pointer;
    definition = definition + conv->definition_index;
    flags = definition->flags;

    if ((((flags & 2) == 0 || self->tally.by_threat_class[8] == 0) &&
         ((flags & 4) == 0 || self->target_combat_status < 9)) &&
        ((flags & 8) == 0 || self->target_combat_status < 6)) {
        return 0;
    }
    return 1;
}

namespace halo::ai {
uint8_t actor_command_list_permits_escalation(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).command_list_permits_escalation();
}
}

namespace c_actor_command_list_reset_record {
}


/**
 * actor_command_list_reset_record: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_command_list_reset_record.c.txt.
 *
 * @address 0x406dd0
 */
void halo::ai::actor_ref::command_list_reset_record(datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    using namespace c_actor_command_list_reset_record;
    uint32_t actor_index = datum;

    (void)actor_index;
    (void)unit_index;
    (void)command_list_index;
    memset(action, 0, sizeof(*action));
    action->command_index = 0xff;
    if (*(uint8_t *)(uintptr_t)callback_extra != 0) {
        action->flags |= 1;
    } else {
        action->flags &= 0xfe;
    }
    if (aim != 0) {
        memset(aim, 0, sizeof(*aim));
        aim->movement_style = -1;
    }
}

namespace halo::ai {
void actor_command_list_reset_record(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index, actor_squad_action_state *action, actor_command_aim *aim, uint32_t callback_extra)
{
    halo::ai::actor_ref(actor_index).command_list_reset_record(unit_index, command_list_index, action, aim, callback_extra);
}
}

namespace c_actor_delete {
}


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
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    data_iterator iterator;
    prop *p;

    if (self->encounterless == 0) {
        halo::ai::encounter_remove_actor(actor_index, (uint8_t)flag);
    } else {
        halo::ai::ai_actor_unlink_from_unassigned_list(actor_index);
    }

    if (self->swarm == 0) {
        halo::ai::actor_unlink_unit(actor_index);
    } else {
        halo::ai::actor_delete_swarm(actor_index);
        while (self->cluster_unit_index != (datum_index)k_datum_index_none) {
            halo::ai::actor_remove_from_unit_cluster(actor_index, self->cluster_unit_index);
        }
    }

    halo::ai::actor_clear_perceived_props(actor_index);

    iterator.data = halo::ai::globals().prop_data;
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

    halo::ai::ai_conversation_clear_participant(actor_index);
    halo::memory::datum_delete(halo::ai::globals().actor_data, actor_index);
}

namespace halo::ai {
void actor_delete(datum_index actor_index, uint32_t flag)
{
    halo::ai::actor_ref(actor_index).delete_(flag);
}
}

namespace c_actor_delete_or_release_unit {
}


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
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->swarm == 0) {
        datum_index unit_index = self->unit_index;
        halo::ai::actor_attempt_grenade_throw(actor_index);
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
            halo::ai::actor_delete(actor_index, 1);
            return;
        }
        halo::ai::actor_remove_from_unit_cluster(actor_index, unit_index);
        if (is_dead == 0) {
            object *unit_object = halo::ai::object_at(unit_index);
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

namespace halo::ai {
void actor_delete_or_release_unit(datum_index actor_index, uint8_t is_dead)
{
    halo::ai::actor_ref(actor_index).delete_or_release_unit(is_dead);
}
}

namespace c_actor_dispatch_perception_reset {
}


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
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];

    if (self->swarm == 0) {
        halo::ai::actor_reset_perception_scratch(self->unit_index);
        self->unit_control_pending = 1;
        return;
    }

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)halo::ai::globals().swarm_data->data)[self->swarm_index & halo::k_slot_mask];
        int16_t i;
        for (i = 0; i < s->component_count; i++) {
            halo::ai::actor_reset_perception_scratch(s->unit_index[i]);
        }
    }
    self->unit_control_pending = 1;
}

namespace halo::ai {
void actor_dispatch_perception_reset(datum_index actor_index)
{
    halo::ai::actor_ref(actor_index).dispatch_perception_reset();
}
}

namespace c_actor_dispatch_squad_order {
}


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
        halo::ai::actor_queue_search_and_relay_perception(prop_index, actor_index);
    } else if (order->type == 3) {

        prop *p = &((prop *)halo::ai::globals().prop_data->data)[prop_index & halo::k_slot_mask];
        if (p->owner_actor_index != (datum_index)k_datum_index_none) {
            datum_index ordered = *(datum_index *)((uint8_t *)order + 0x18);
            prop *other = (prop *)halo::memory::datum_get(ordered, halo::ai::globals().prop_data);

            if (other != 0) {
                halo::ai::actor_target_data_acquire(actor_index, other->object_index, p->owner_actor_index, ordered);
            }
        }
    } else if (order->type == 4) {
        halo::ai::actor_scan_ally_death_panic_reaction(prop_index, actor_index);
    }
}

namespace halo::ai {
void actor_dispatch_squad_order(datum_index prop_index, const actor_squad_order_header *order, datum_index actor_index)
{
    halo::ai::actor_ref::dispatch_squad_order(prop_index, order, actor_index);
}
}

namespace c_actor_dispatch_type_vtable_0x10 {
static auto &actor_type_procs = halo::link::ref<actor_type_table_entry *[16]>(halo::ai::vars().actor_type_procs);
}


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
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    actor_type_table_entry *entry = actor_type_procs[self->type];

    if (entry->proc_10 != 0) {
        ((void (*)(datum_index))entry->proc_10)(actor_index);
    }
}

namespace halo::ai {
void actor_dispatch_type_vtable_0x10(datum_index actor_index)
{
    halo::ai::actor_ref(actor_index).dispatch_type_vtable_0x10();
}
}

namespace c_actor_dispatch_type_vtable_0x18 {
static auto &actor_type_procs = halo::link::ref<actor_type_table_entry *[16]>(halo::ai::vars().actor_type_procs);
}


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
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    actor_type_table_entry *entry = actor_type_procs[self->type];

    ((void (*)(datum_index))entry->proc_18)(actor_index);
}

namespace halo::ai {
void actor_dispatch_type_vtable_0x18(datum_index actor_index)
{
    halo::ai::actor_ref(actor_index).dispatch_type_vtable_0x18();
}
}

namespace c_actor_dispatch_type_vtable_0x1c {
static auto &actor_type_procs = halo::link::ref<actor_type_table_entry *[16]>(halo::ai::vars().actor_type_procs);
}


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
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    actor_type_table_entry *entry = actor_type_procs[self->type];

    if (entry->proc_1c != 0) {
        ((void (*)(datum_index, uint32_t, uint32_t, uint32_t))entry->proc_1c)(actor_index, a, b, c);
    }
}

namespace halo::ai {
void actor_dispatch_type_vtable_0x1c(datum_index actor_index, uint32_t a, uint32_t b, uint32_t c)
{
    halo::ai::actor_ref(actor_index).dispatch_type_vtable_0x1c(a, b, c);
}
}

namespace c_actor_get_actor_definition {
}


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

    self = halo::ai::actor_at(actor_index);
    default_definition = halo::cache::globals().tag_instances[self->actor_variant_tag & halo::k_slot_mask].data;

    weapon_object = halo::ai::actor_get_threat_weapon_object_index(actor_index);
    if (weapon_object != (datum_index)k_datum_index_none) {
        object_header *hdr = (object_header *)halo::objects::globals().object_data->data + (weapon_object & halo::k_slot_mask);
        object *obj = hdr->data;
        Weapon *weapon_definition = halo::ai::tag_data<Weapon>(obj->definition_tag);
        if (weapon_definition != 0) {
            uint32_t override_index = static_cast<uint32_t>(halo::ai::tag_handle(weapon_definition->actor_firing_parameters));
            if (override_index != (uint32_t)-1) {
                return halo::cache::globals().tag_instances[override_index & halo::k_slot_mask].data;
            }
        }
    }
    return default_definition;
}

namespace halo::ai {
void * actor_get_actor_definition(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).get_actor_definition();
}
}

namespace c_actor_get_body_axis_vector {
}


/**
 * actor_get_body_axis_vector: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_get_body_axis_vector.c.txt.
 *
 * @address 0x405390
 */
void halo::ai::actor_ref::get_body_axis_vector(uint32_t unit_index, actor_squad_action_state *request)
{
    using namespace c_actor_get_body_axis_vector;
    uint32_t actor_index = datum;
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
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
        request->direction = reference;
        return;
    case 1:
        request->direction.i = -reference.i;
        request->direction.j = -reference.j;
        request->direction.k = -reference.k;
        return;
    case 2:
    case 3:
        {
            real_vector3d perp;

            halo::math::vector3d_cross_product(perp, reference, *halo::math::globals().global_up3d_pointer);
            if (halo::math::vector3d_normalize_with_length(perp) == 0.0f) {
                object *obj = halo::ai::object_at(unit_index);

                halo::math::vector3d_cross_product(perp, reference, obj->up);
                if (halo::math::vector3d_normalize_with_length(perp) == 0.0f) {
                    perp = *halo::math::globals().global_forward3d_pointer;
                }
            }
            if (request->axis == 2) {
                request->direction = perp;
            } else {
                request->direction.i = -perp.i;
                request->direction.j = -perp.j;
                request->direction.k = -perp.k;
            }
        }
        return;
    }
}

namespace halo::ai {
void actor_get_body_axis_vector(uint32_t actor_index, uint32_t unit_index, actor_squad_action_state *request)
{
    halo::ai::actor_ref(actor_index).get_body_axis_vector(unit_index, request);
}
}

namespace c_actor_get_current_mode_combat_grade {
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
}


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
    actor *self = halo::ai::actor_at(actor_index);
    return actor_mode_definitions[self->mode].combat_grade;
}

namespace halo::ai {
int16_t actor_get_current_mode_combat_grade(datum_index actor_index)
{
    return halo::ai::actor_ref(actor_index).get_current_mode_combat_grade();
}
}

namespace c_actor_get_ranged_attack_vector {
}


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

    self = halo::ai::actor_at(actor_index);
    target = halo::ai::prop_at(target_prop_index);

    if (target->swarm_owned != 0) {
        return 0;
    }

    if (target->is_parented == 0) {
        if (target->owner_actor_index != k_datum_index_none) {
            return halo::ai::actor_get_cached_wander_position(target->owner_actor_index, out_vector);
        }
        return 0;
    }

    unit_obj = halo::ai::object_at(target->object_index);
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    *out_vector = unit->aiming_vector;

    if (target->shooting == 0 && (int8_t)self->tally.unit_props > 0) {
        prop_index = self->first_prop;
        while (prop_index != k_datum_index_none) {
            ally = halo::ai::prop_at(prop_index);
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

namespace halo::ai {
uint8_t actor_get_ranged_attack_vector(datum_index target_prop_index, datum_index actor_index, real_vector3d *out_vector)
{
    return halo::ai::actor_ref::get_ranged_attack_vector(target_prop_index, actor_index, out_vector);
}
}

namespace c_actor_handle_death {
}


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

    self = halo::ai::actor_at(actor_index);

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
        halo::ai::actor_consider_target_candidate(actor_index, (datum_index)previous_target);
    }
    if (self->swarm == 0) {
        halo::ai::actor_check_melee_target_reachable(actor_index, reinterpret_cast<actor_mode_flee_data *>(local_data));
        if (*(int16_t *)(local_data + 8) != -1) {
            halo::ai::actor_set_mode(actor_index, halo::ai::actor_mode::flee, local_data);
            return 1;
        }
    }
    return 0;
}

namespace halo::ai {
uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3)
{
    return halo::ai::actor_ref(actor_index).handle_death(param_2, param_3);
}
}

namespace c_actor_invoke_type_handler {
static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
}


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
    actor *a = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    void (*update_proc)(uint32_t) = (void (*)(uint32_t))actor_mode_definitions[a->mode].update_proc;

    if (update_proc != 0) {
        update_proc(actor_index);
    }
}

namespace halo::ai {
void actor_invoke_type_handler(uint32_t actor_index)
{
    halo::ai::actor_ref(actor_index).invoke_type_handler();
}
}

namespace c_actor_iterator_new {
}


/**
 * actor_iterator_new: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_iterator_new.c.txt.
 *
 * @address 0x436a30
 */
void halo::ai::actor_ref::iterator_new(actor_iterator_state *out_iterator, uint8_t active_only)
{
    using namespace c_actor_iterator_new;
    if (halo::ai::globals().state->actors_valid != 0) {
        out_iterator->filter_array = halo::ai::globals().encounter_data;
        out_iterator->next_index = 0;
        out_iterator->cursor = -1;
        out_iterator->signature = (uint32_t)halo::ai::globals().encounter_data ^ halo::ai::k_iterator_signature_key;
        out_iterator->encounterless_done = 0;
        out_iterator->active = active_only;
        out_iterator->actor_index = (datum_index)k_datum_index_none;
        out_iterator->next_actor_index = -1;
    }
}

namespace halo::ai {
void actor_iterator_new(actor_iterator_state *out_iterator, uint8_t active_only)
{
    halo::ai::actor_ref::iterator_new(out_iterator, active_only);
}
}

namespace c_actor_iterator_next {
}


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

    if (halo::ai::globals().state->actors_valid == 0) {
        return 0;
    }

    next = iterator->next_actor_index;
    while (next == (datum_index)k_datum_index_none) {
        encounter *enc = (encounter *)halo::memory::data_iterator_next((data_iterator *)iterator);
        if (enc == 0) {
            if (iterator->encounterless_done == 0) {
                iterator->next_actor_index = halo::ai::globals().state->first_encounterless_actor;
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
        a = &((actor *)halo::ai::globals().actor_data->data)[next & halo::k_slot_mask];
        iterator->next_actor_index = a->next_in_encounter;
    } while (iterator->active != 0 && a->active == 0);

    return a;
}

namespace halo::ai {
actor * actor_iterator_next(actor_iterator_state *iterator)
{
    return halo::ai::actor_ref::iterator_next(iterator);
}
}

namespace c_actor_link_to_unit_cluster {
}


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
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    object_header *header = &((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_slot_mask];
    object *unit_object = header->data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
    datum_index new_component = (datum_index)k_datum_index_none;

    if (unit->swarm_actor_index == actor_index) {
        return 1;
    }

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        new_component = halo::memory::datum_new(halo::ai::globals().swarm_component_data);
        if (new_component == (datum_index)k_datum_index_none) {
            return 0;
        }
    }

    if (unit->swarm_actor_index != (datum_index)k_datum_index_none) {
        halo::ai::actor_remove_from_unit_cluster(unit->swarm_actor_index, unit_index);
    }
    if (unit->actor_index != (datum_index)k_datum_index_none) {
        halo::ai::actor_delete(unit->actor_index, 0);
    }
    if (self->unit_index != (datum_index)k_datum_index_none) {
        halo::ai::actor_unlink_unit(actor_index);
    }

    unit->swarm_actor_index = actor_index;
    unit->swarm_next_unit_index = self->cluster_unit_index;
    unit->swarm_previous_unit_index = halo::k_dword_none;
    if (self->cluster_unit_index != (datum_index)k_datum_index_none) {
        object *head_object = halo::ai::object_at(self->cluster_unit_index);
        halo::units::unit_data_of(head_object)->swarm_previous_unit_index = unit_index;
    }
    self->cluster_unit_index = unit_index;

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        halo::ai::swarm_add_component(new_component, unit_index, self->swarm_index);
    }

    self->cluster_count = self->cluster_count + 1;
    self->total_cluster_count = self->total_cluster_count + 1;

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)halo::ai::globals().encounter_data->data)[self->encounter_index & halo::k_slot_mask];
        halo::ai::ai_encounter_stamp_team_from_unit(self->encounter_index, unit_index);
        unit_object->owner_team = enc->team;
    }
    self->team = unit_object->owner_team;

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

namespace halo::ai {
uint8_t actor_link_to_unit_cluster(datum_index actor_index, datum_index unit_index)
{
    return halo::ai::actor_ref(actor_index).link_to_unit_cluster(unit_index);
}
}

namespace c_actor_mark_units_and_release {
}


/**
 * actor_mark_units_and_release: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mark_units_and_release.c.txt.
 *
 * @address 0x4289c0
 */
void halo::ai::actor_ref::mark_units_and_release(uint8_t use_alternate_flag, datum_index actor_index, uint8_t suppress_release)
{
    using namespace c_actor_mark_units_and_release;
    actor *self = &((actor *)halo::ai::globals().actor_data->data)[actor_index & halo::k_slot_mask];
    datum_index encounter_index = self->encounter_index;

    if (self->swarm == 0) {
        object *unit_object = halo::ai::object_at(self->unit_index);
        uint8_t *flags = (uint8_t *)unit_object + 0x106;
        *flags |= use_alternate_flag == 0 ? 0x20 : 0x40;

        if (suppress_release != 0) {
            return;
        }
        halo::ai::actor_unlink_unit(actor_index);
    } else {
        datum_index unit_index = self->cluster_unit_index;
        while (unit_index != (datum_index)k_datum_index_none) {
            object *unit_object = halo::ai::object_at(unit_index);
            uint8_t *flags = (uint8_t *)unit_object + 0x106;
            *flags |= use_alternate_flag == 0 ? 0x20 : 0x40;

            if (suppress_release == 0) {
                halo::ai::actor_remove_from_unit_cluster(actor_index, unit_index);
            }
            unit_index = halo::units::unit_data_of(unit_object)->swarm_next_unit_index;
        }
        if (suppress_release != 0) {
            return;
        }
    }

    halo::ai::actor_delete(actor_index, 1);
    if (encounter_index != (datum_index)k_datum_index_none) {
        halo::ai::encounter_recompute_morale(encounter_index);
    }
}

namespace halo::ai {
void actor_mark_units_and_release(uint8_t use_alternate_flag, datum_index actor_index, uint8_t suppress_release)
{
    halo::ai::actor_ref::mark_units_and_release(use_alternate_flag, actor_index, suppress_release);
}
}

