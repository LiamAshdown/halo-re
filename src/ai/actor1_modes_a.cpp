#include "halo/ai/actor_modes.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"

namespace c_actor_mode_alert_movement_cancelled {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

extern "C" void actor_mode_alert_movement_cancelled(datum_index actor_index);

/**
 * actor_mode_alert_movement_cancelled: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_movement_cancelled.c.txt.
 *
 * @address 0x401460
 */
void halo::ai::alert_mode::movement_cancelled()
{
    using namespace c_actor_mode_alert_movement_cancelled;
    datum_index actor_index = datum;
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    *(int16_t *)(mode_data + 0x6) = -1;
    *(int16_t *)(mode_data + 0x8) = -1;
}

extern "C" void actor_mode_alert_movement_cancelled(datum_index actor_index)
{
    halo::ai::alert_mode(actor_index).movement_cancelled();
}

#undef ACTOR

namespace c_actor_mode_alert_process {
extern "C" {
extern data_array *actor_data;

#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

extern float actor_compute_accuracy_scale(datum_index actor_index);
extern int32_t actor_select_move_position(uint32_t actor_index, int16_t select_mode, int32_t position_index,
    uint8_t *direction_flag);
extern uint8_t actor_movement_set_destination_move_position(datum_index actor_index, int16_t move_position_index);
}
}

extern "C" uint8_t actor_mode_alert_process(uint32_t actor_index);

/**
 * actor_mode_alert_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_process.c.txt.
 *
 * @address 0x4010e0
 */
uint8_t halo::ai::alert_mode::process()
{
    using namespace c_actor_mode_alert_process;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    int16_t count = W(0x9c);

    if (count != 0 && W(0xa4) == -1) {
        int16_t current = W(0xa2);
        int ready = 1;

        if (current != -1 && B(0x4a8) != 0) {
            float distance_squared = halo::math::vector3d_distance_squared(*(real_point3d *)(actor + 0xa8), ((struct actor *)actor)->body_position);
            float radius = actor_compute_accuracy_scale(actor_index);

            if (!(radius > 0.5f)) {
                radius = 0.5f;
            }
            if (distance_squared > radius * radius) {
                ready = 0;
            }
        }
        if (ready && !(W(0x9e) > 0) && B(0xa6) == 0) {
            uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[D(0x18) & halo::k_slot_mask].data;

            if (unit[0x2a3] != 0x1c) {
                W(0xa4) = (int16_t)actor_select_move_position(actor_index, count, current, actor + 0xa0);
            }
        }
    }

    if (B(0x4c) == 0 || B(0x13) != 0 || W(0xa4) == -1) {
        return 0;
    }
    if (D(0x34) != halo::k_dword_none) {
        uint8_t *encounter = (uint8_t *)halo::scenario::globals().scenario->encounters.pointer + (D(0x34) & halo::k_slot_mask) * 0xb0;
        uint8_t *squad = *(uint8_t **)(encounter + 0x84) + W(0x3a) * 0xe8;
        int16_t next = W(0xa4);

        if (next >= 0 && next < *(int32_t *)(squad + 0xc4)) {
            uint8_t *position = *(uint8_t **)(squad + 0xc8) + next * 0x50;
            float wait = halo::math::random_real_range(*(float *)(position + 0x14), *(float *)(position + 0x18)) * 30.0f;

            W(0xa2) = W(0xa4);
            W(0xa4) = -1;
            memcpy(actor + 0xa8, position, 0x50);
            W(0x9e) = (int16_t)(int32_t)wait;
            B(0xa6) = 1;
            if (actor_movement_set_destination_move_position(actor_index, W(0xa2))) {
                return 0;
            }
        }
    }
    W(0xa2) = W(0xa4);
    W(0xa4) = -1;
    W(0x9e) = 0;
    B(0xa6) = 0;
    return 0;
}

extern "C" uint8_t actor_mode_alert_process(uint32_t actor_index)
{
    return halo::ai::alert_mode(actor_index).process();
}

#undef B
#undef D
#undef F
#undef W

namespace c_actor_mode_alert_target_cleared {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

extern "C" void actor_mode_alert_target_cleared(datum_index actor_index);

/**
 * actor_mode_alert_target_cleared: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_target_cleared.c.txt.
 *
 * @address 0x401490
 */
void halo::ai::alert_mode::target_cleared()
{
    using namespace c_actor_mode_alert_target_cleared;
    datum_index actor_index = datum;
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    *(int16_t *)(mode_data + 0x34) = -1;
    *(int32_t *)(mode_data + 0x58) = -1;
}

extern "C" void actor_mode_alert_target_cleared(datum_index actor_index)
{
    halo::ai::alert_mode(actor_index).target_cleared();
}

#undef ACTOR

namespace c_actor_mode_alert_tick {
extern "C" {
extern data_array *actor_data;

#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

}
}

extern "C" void actor_mode_alert_tick(uint32_t actor_index);

/**
 * actor_mode_alert_tick: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_tick.c.txt.
 *
 * @address 0x4012e0
 */
void halo::ai::alert_mode::tick()
{
    using namespace c_actor_mode_alert_tick;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;

    if (B(0x13) != 0 || W(0xa2) == -1) {
        return;
    }
    if (B(0x4a8) != 0 && B(0x484) == 0) {
        float dx = F(0xa8) - F(0x12c);
        float dy = F(0xac) - F(0x130);
        float dz = F(0xb0) - F(0x134);

        if (!(dz * dz + dy * dy + dx * dx < 0.25f)) {
            return;
        }
    }
    if (W(0x9e) > 0) {
        W(0x9e) = (int16_t)(W(0x9e) - 1);
    }
    if (B(0xa6) == 0) {
        return;
    }
    if (W(0xc4) != -1) {
        uint8_t *animation = (uint8_t *)halo::scenario::globals().scenario->ai_animation_references.pointer + W(0xc4) * 0x3c;
        datum_index graph = *(datum_index *)(animation + 0x2c);

        if (graph == k_datum_index_none) {
            uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[D(0x18) & halo::k_slot_mask].data;

            graph = *(datum_index *)((uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)unit & halo::k_slot_mask].data + 0x44);
        }
        halo::units::unit_start_user_animation(D(0x18), graph, (const char *)animation, 1);
    }
    B(0xa6) = 0;
}

extern "C" void actor_mode_alert_tick(uint32_t actor_index)
{
    halo::ai::alert_mode(actor_index).tick();
}

#undef B
#undef D
#undef F
#undef W

namespace c_actor_mode_alert_update {
extern "C" {
extern data_array *actor_data;

#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))
}
}

extern "C" void actor_mode_alert_update(uint32_t actor_index);

/**
 * actor_mode_alert_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_alert_update.c.txt.
 *
 * @address 0x401410
 */
void halo::ai::alert_mode::update()
{
    using namespace c_actor_mode_alert_update;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;

    W(0x3fc) = 1;
    if (*(uint8_t *)halo::cache::globals().tag_instances[D(0x58) & halo::k_slot_mask].data & 0x40) {
        B(0x426) = 1;
        B(0x427) = 1;
    }
}

extern "C" void actor_mode_alert_update(uint32_t actor_index)
{
    halo::ai::alert_mode(actor_index).update();
}

#undef B
#undef D
#undef F
#undef W

namespace c_actor_mode_avoid_update {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

extern "C" void actor_mode_avoid_update(datum_index actor_index);

/**
 * actor_mode_avoid_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_avoid_update.c.txt.
 *
 * @address 0x401850
 */
void halo::ai::avoid_mode::update()
{
    using namespace c_actor_mode_avoid_update;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);

    if (((actor *)act)->target_combat_status >= 5) {
        act[0x454] = 1;
        ((actor *)act)->flee_reason = 7;
        ((actor *)act)->flee_source.code = 2;
    } else {
        ((actor *)act)->flee_reason = 5;
        ((actor *)act)->flee_source.code = ((actor *)act)->danger_type > 0 ? 5 : 2;
    }
    ((struct actor *)act)->look_posture = 4;
    act[0x426] = act[0x358];
    act[0x427] = 0;
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 0;
}

extern "C" void actor_mode_avoid_update(datum_index actor_index)
{
    halo::ai::avoid_mode(actor_index).update();
}

#undef ACTOR

namespace c_actor_mode_converse_exit {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)

extern void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b);
}
}

extern "C" void actor_mode_converse_exit(datum_index actor_index);

/**
 * actor_mode_converse_exit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_converse_exit.c.txt.
 *
 * @address 0x402f40
 */
void halo::ai::converse_mode::exit()
{
    using namespace c_actor_mode_converse_exit;
    datum_index actor_index = datum;
    datum_index conversation = ((struct actor *)ACTOR(actor_index))->conversation_index;

    if (conversation != k_datum_index_none) {
        ai_conversation_stop(conversation, 0, 0);
    }
}

extern "C" void actor_mode_converse_exit(datum_index actor_index)
{
    halo::ai::converse_mode(actor_index).exit();
}

#undef ACTOR

namespace c_actor_mode_converse_process {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)

extern data_array *prop_data;
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
                                                    char create_if_missing, uint32_t flag);
extern void actor_movement_action_stop(datum_index actor_index);
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
                                                          float radius);
}
}

extern "C" uint8_t actor_mode_converse_process(datum_index actor_index);

/**
 * actor_mode_converse_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_converse_process.c.txt.
 *
 * @address 0x402d70
 */
uint8_t halo::ai::converse_mode::process()
{
    using namespace c_actor_mode_converse_process;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    datum_index partner;

    if (!act[0x4c]) {
        return act[0xa0];
    }
    if (((struct actor *)act)->mode_data.converse.partner_prop == k_datum_index_none && ((struct actor *)act)->mode_data.converse.partner_unit != k_datum_index_none) {
        ((struct actor *)act)->mode_data.converse.partner_prop = actor_find_or_create_shared_prop(((struct actor *)act)->mode_data.converse.partner_unit, actor_index, 1, 1);
    }
    partner = ((struct actor *)act)->mode_data.converse.partner_prop;
    if (partner == k_datum_index_none) {
        act[0xa0] = 1;
        return act[0xa0];
    }
    if (!act[0xa1]) {
        uint8_t *p = (uint8_t *)prop_data->data + (partner & halo::k_slot_mask) * k_prop_size;
        float distance = ((prop *)p)->distance;

        if ((((struct prop *)p)->visual_perception >= 2 && distance < ((struct actor *)act)->mode_data.converse.approach_distance) || distance < 0.7f) {
            act[0xa1] = 1;
        }
    }
    if (act[0xa1]) {
        actor_movement_action_stop(actor_index);
        return act[0xa0];
    }
    if (!actor_movement_set_destination_near_target(partner, actor_index, ((struct actor *)act)->mode_data.converse.approach_distance)) {
        act[0xa0] = 1;
    }
    return act[0xa0];
}

extern "C" uint8_t actor_mode_converse_process(datum_index actor_index)
{
    return halo::ai::converse_mode(actor_index).process();
}

#undef ACTOR

namespace c_actor_mode_converse_replace_reference {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

extern "C" void actor_mode_converse_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference);

/**
 * actor_mode_converse_replace_reference: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_converse_replace_reference.c.txt.
 *
 * @address 0x402f00
 */
void halo::ai::converse_mode::replace_reference(datum_index old_reference, datum_index new_reference)
{
    using namespace c_actor_mode_converse_replace_reference;
    datum_index actor_index = datum;
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    if (((actor_mode_converse_data *)mode_data)->partner_prop == old_reference) {
        ((actor_mode_converse_data *)mode_data)->partner_prop = new_reference;
    }
}

extern "C" void actor_mode_converse_replace_reference(datum_index actor_index, datum_index old_reference, datum_index new_reference)
{
    halo::ai::converse_mode(actor_index).replace_reference(old_reference, new_reference);
}

#undef ACTOR

namespace c_actor_mode_converse_update {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)

extern data_array *ai_conversation_data;
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
}
}

extern "C" void actor_mode_converse_update(datum_index actor_index);

/**
 * actor_mode_converse_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_converse_update.c.txt.
 *
 * @address 0x402e70
 */
void halo::ai::converse_mode::update()
{
    using namespace c_actor_mode_converse_update;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    datum_index conversation = ((struct actor *)act)->mode_data.converse.conversation;
    uint8_t *record = 0;
    datum_index look_prop = k_datum_index_none;

    if (conversation != k_datum_index_none) {
        record = (uint8_t *)ai_conversation_data->data + (conversation & halo::k_slot_mask) * k_ai_conversation_size;
    }
    if (((struct actor *)act)->mode_data.converse.partner_prop != k_datum_index_none) {
        look_prop = ((struct actor *)act)->mode_data.converse.partner_prop;
    } else if (record != 0 && *(datum_index *)(record + 0x10) != k_datum_index_none) {
        look_prop = actor_find_prop_for_object(*(datum_index *)(record + 0x10), actor_index);
    }
    ((struct actor *)act)->look_posture = 1;
    if (look_prop != k_datum_index_none) {
        ((actor *)act)->flee_reason = 3;
        ((actor *)act)->flee_source.code = 1;
        *(datum_index *)(act + 0x3f0) = look_prop;
    }
}

extern "C" void actor_mode_converse_update(datum_index actor_index)
{
    halo::ai::converse_mode(actor_index).update();
}

#undef ACTOR

namespace c_actor_mode_obey_enter {
extern "C" {
extern data_array *actor_data;
extern void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback,
    uint32_t callback_extra, uint16_t *caller_record);
extern void actor_obey_member_enter(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index,
    void *component_record, int32_t secondary_record, uint32_t callback_extra);
}
}

extern "C" void actor_mode_obey_enter(uint32_t actor_index);

/**
 * actor_mode_obey_enter: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_enter.c.txt.
 *
 * @address 0x407280
 */
void halo::ai::obey_mode::enter()
{
    using namespace c_actor_mode_obey_enter;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;

    actor_swarm_for_each_component(actor_index, 0, (actor_swarm_member_callback)actor_obey_member_enter, 0, (uint16_t *)(actor + 0x9c));
}

extern "C" void actor_mode_obey_enter(uint32_t actor_index)
{
    halo::ai::obey_mode(actor_index).enter();
}

namespace c_actor_mode_obey_exit {
extern "C" {
extern data_array *actor_data;
extern void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback,
    uint32_t callback_extra, uint16_t *caller_record);
extern void actor_obey_member_exit(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index,
    void *component_record, int32_t secondary_record, uint32_t callback_extra);
}
}

extern "C" void actor_mode_obey_exit(uint32_t actor_index);

/**
 * actor_mode_obey_exit: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_exit.c.txt.
 *
 * @address 0x4072c0
 */
void halo::ai::obey_mode::exit()
{
    using namespace c_actor_mode_obey_exit;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;

    actor_swarm_for_each_component(actor_index, 0, (actor_swarm_member_callback)actor_obey_member_exit, 0, (uint16_t *)(actor + 0x9c));
}

extern "C" void actor_mode_obey_exit(uint32_t actor_index)
{
    halo::ai::obey_mode(actor_index).exit();
}

namespace c_actor_mode_obey_process {
extern "C" {
extern data_array *actor_data;
extern void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback,
    uint32_t callback_extra, uint16_t *caller_record);
extern void actor_squad_action_list_process(uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index,
    uint8_t *state, uint8_t *aim_state, uint8_t *out);
}
}

extern "C" uint8_t actor_mode_obey_process(uint32_t actor_index);

/**
 * actor_mode_obey_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_process.c.txt.
 *
 * @address 0x407340
 */
uint8_t halo::ai::obey_mode::process()
{
    using namespace c_actor_mode_obey_process;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t *mode_data = actor + 0x9c;
    uint8_t still_running = 1;

    actor_swarm_for_each_component(actor_index, 0, (actor_swarm_member_callback)actor_squad_action_list_process,
        (uint32_t)&still_running, (uint16_t *)mode_data);
    if (still_running && mode_data[5] == 0) {
        uint8_t *list = (uint8_t *)halo::scenario::globals().scenario->command_lists.pointer + *(int16_t *)mode_data * 0x60;
        int mark = 1;

        if ((list[0x20] & 0x10) && actor[0x15c] != 0) {
            uint8_t *variant = (uint8_t *)halo::cache::globals().tag_instances[((struct actor *)actor)->actor_definition_tag & halo::k_slot_mask].data;

            if ((*(uint32_t *)variant & 0x200000) == 0) {
                mark = 0;
            }
        }
        if (mark) {
            ((struct actor *)actor)->command_list_finished_time = halo::game::globals().game_time->game_time;
            mode_data[5] = 1;
        }
    }
    return (uint8_t)(((struct actor *)actor)->mode == 0xb && mode_data[5] != 0);
}

extern "C" uint8_t actor_mode_obey_process(uint32_t actor_index)
{
    return halo::ai::obey_mode(actor_index).process();
}

namespace c_actor_mode_obey_tick_members {
extern "C" {
extern data_array *actor_data;
extern void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback,
    uint32_t callback_extra, uint16_t *caller_record);
extern void actor_obey_member_tick(uint32_t actor_index, datum_index unit_index, uint16_t command_list_index,
    void *component_record, int32_t secondary_record, uint32_t callback_extra);
}
}

extern "C" void actor_mode_obey_tick_members(uint32_t actor_index);

/**
 * actor_mode_obey_tick_members: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_tick_members.c.txt.
 *
 * @address 0x407300
 */
void halo::ai::obey_mode::tick_members()
{
    using namespace c_actor_mode_obey_tick_members;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;

    actor_swarm_for_each_component(actor_index, 0, (actor_swarm_member_callback)actor_obey_member_tick, 0, (uint16_t *)(actor + 0x9c));
}

extern "C" void actor_mode_obey_tick_members(uint32_t actor_index)
{
    halo::ai::obey_mode(actor_index).tick_members();
}

namespace c_actor_mode_obey_update {
extern "C" {
extern data_array *actor_data;
extern real_vector2d *global_forward2d_pointer;

extern uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action, uint32_t payload[2]);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);

#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

static void copy12(uint8_t *actor, int to, int from)
{
    D(to) = D(from);
    D(to + 4) = D(from + 4);
    D(to + 8) = D(from + 8);
}
}
}

extern "C" void actor_mode_obey_update(uint32_t actor_index);

/**
 * actor_mode_obey_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_obey_update.c.txt.
 *
 * @address 0x407400
 */
void halo::ai::obey_mode::update()
{
    using namespace c_actor_mode_obey_update;
    uint32_t actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;

    if (B(0xfe) != 0) {
        W(0x3e8) = 7;
        W(0x3ec) = 2;
        W(0x3fc) = 4;
        B(0x454) = 1;
        B(0x457) = 1;
        B(0x45d) = 1;
        copy12(actor, 0x460, 0x100);
        D(0x458) = D(0x10c);
    } else if (B(0xe0) != 0 && (B(0x4a8) == 0 || B(0x484) != 0)) {
        W(0x3e8) = 4;
        W(0x3ec) = 3;
        copy12(actor, 0x3f0, 0xe4);
        if (B(0x99) == 0) {
            D(0x3f8) = D(0x128);
        }
        W(0x3fc) = (int16_t)(W(0x6a) < 3 ? 1 : 4);
    } else if (W(0xca) == 3 || W(0xca) == 1) {
        W(0x3ec) = 0;
        W(0x3fc) = 0;
        W(0x3e8) = 7;
    } else if (W(0x6e) >= 5 && (B(0xa8) & 1)) {
        W(0x3ec) = 2;
        W(0x3fc) = 4;
        B(0x454) = 1;
        W(0x3e8) = 7;
    } else {
        W(0x3e8) = 0;
        if (B(0x9f) != 0) {
            W(0x3fc) = (int16_t)(W(0x6a) < 3 ? 1 : 4);
        } else {
            W(0x3fc) = 0;
        }
    }

    if (B(0x110) != 0) {
        B(0x45c) = 1;
        B(0x110) = 0;
    }
    B(0x426) = B(0xc8);
    B(0x427) = B(0xc8);
    W(0x42c) = W(0xca);

    if (B(0xf8) != 0 && W(0x418) == -1 &&
        (D(0x18) == halo::k_dword_none || !halo::units::unit_is_in_busy_animation_state(D(0x18)))) {
        if (W(0xfa) != -1) {
            uint32_t direction[2];

            direction[0] = D(0x5a4);
            direction[1] = D(0x5a8);
            halo::math::vector2d_normalize_with_length(*(real_vector2d *)direction);
            actor_queue_secondary_action(actor_index, W(0xfa), direction);
        }
        if (W(0xfc) != -1) {
            ai_communication_broadcast(W(0xfc), D(0x18), halo::k_dword_none, -1, halo::k_dword_none, halo::k_dword_none, 0);
        }
        B(0xf8) = 0;
    }

    if (B(0xa9) & 1) {
        B(0x430) = 1;
        copy12(actor, 0x434, 0xb0);
        W(0x42e) = W(0xac);
    }
    if ((B(0xa9) & 4) == 0) {
        return;
    }
    if (B(0xa9) & 8) {
        if (!(W(0xac) > 0)) {
            return;
        }
    } else if (W(0xac) == 0 && B(0x15c) == 0 && !halo::units::unit_is_in_busy_animation_state(D(0x18))) {
        real_vector2d direction;
        float x;
        float y;

        direction = *(real_vector2d *)&((struct actor *)actor)->facing.i;
        if (halo::math::vector2d_normalize_with_length(direction) == 0.0f) {
            x = global_forward2d_pointer->i;
            y = global_forward2d_pointer->j;
        } else {
            x = direction.i;
            y = direction.j;
        }
        B(0x440) = 1;
        B(0x441) = (uint8_t)(F(0xb0) * 0.7f > F(0xb4));
        B(0x442) = (uint8_t)((B(0xa9) >> 4) & 1);
        F(0x448) = y;
        F(0x444) = x;
        D(0x44c) = D(0xb0);
        D(0x450) = D(0xb4);
        B(0xa9) = (uint8_t)(B(0xa9) | 8);
        if ((B(0xa9) & 0x10) == 0) {
            W(0xac) = 0xf;
        }
        return;
    }
    copy12(actor, 0x434, 0x174);
    B(0x430) = 1;
    W(0x42e) = 0;
}

extern "C" void actor_mode_obey_update(uint32_t actor_index)
{
    halo::ai::obey_mode(actor_index).update();
}

#undef B
#undef D
#undef F
#undef W

namespace c_actor_mode_search_enter {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)

}
}

extern "C" void actor_mode_search_enter(datum_index actor_index);

/**
 * actor_mode_search_enter: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_enter.c.txt.
 *
 * @address 0x407940
 */
void halo::ai::search_mode::enter()
{
    using namespace c_actor_mode_search_enter;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[((actor *)act)->actor_definition_tag & halo::k_slot_mask].data;
    float lo;
    float hi;
    float t;
    int32_t ticks;

    if (((struct actor *)act)->mode_data.search.stage == 0) {
        lo = *(float *)(actor_tag + 0x344);
        hi = *(float *)(actor_tag + 0x348);
    } else {
        lo = *(float *)(actor_tag + 0x34c);
        hi = *(float *)(actor_tag + 0x350);
    }
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    t = (float)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f;
    ticks = (int32_t)(((hi - lo) * t + lo) * 30.0f);
    ((struct actor *)act)->mode_data.search.duration_ticks = ticks;
    ((struct actor *)act)->mode_data.search.remaining_ticks = ticks;
}

extern "C" void actor_mode_search_enter(datum_index actor_index)
{
    halo::ai::search_mode(actor_index).enter();
}

#undef ACTOR

namespace c_actor_mode_search_movement_cancelled {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

extern "C" void actor_mode_search_movement_cancelled(datum_index actor_index);

/**
 * actor_mode_search_movement_cancelled: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_movement_cancelled.c.txt.
 *
 * @address 0x407f00
 */
void halo::ai::search_mode::movement_cancelled()
{
    using namespace c_actor_mode_search_movement_cancelled;
    datum_index actor_index = datum;
    uint8_t *mode_data = ACTOR(actor_index) + 0x9c;

    if (((actor_mode_search_data *)mode_data)->stage == 1) {
        ((actor_mode_search_data *)mode_data)->firing_position = -1;
        mode_data[0x0] = 1;
    }
}

extern "C" void actor_mode_search_movement_cancelled(datum_index actor_index)
{
    halo::ai::search_mode(actor_index).movement_cancelled();
}

#undef ACTOR

namespace c_actor_mode_search_process {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)

extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying);
extern void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator);
extern uint8_t actor_targets_share_descriptor(datum_index actor_a, datum_index actor_b);
extern uint8_t ai_pursuit_note_object(datum_index object_index, datum_index encounter_index, int16_t type,
                                      int32_t min_last_tick);
extern void actor_movement_action_stop(datum_index actor_index);
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
                                                          float radius);
extern uint8_t actor_movement_set_destination_firing_position(datum_index actor_index, int16_t formation_slot,
                                                              path_find_context *path_context);
}
}

extern "C" uint8_t actor_mode_search_process(datum_index actor_index);

/**
 * actor_mode_search_process: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_process.c.txt.
 *
 * @address 0x407a10
 */
uint8_t halo::ai::search_mode::process()
{
    using namespace c_actor_mode_search_process;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    int16_t kind;
    uint8_t ok;

    if (act[0x6] || !act[0x4c] || act[0x9c]) {
        return act[0x9c];
    }
    kind = ((struct actor *)act)->mode_data.search.stage;
    ((struct actor *)act)->mode_data.search.reachable = 1;
    if (kind == 0 && ((actor *)act)->target_unit_index != k_datum_index_none) {
        uint8_t *target = PROP(((actor *)act)->target_unit_index);
        float radius = ((struct actor *)target)->original_squad_index == 0 ? 1.7f : 0.7f;
        float distance_squared = halo::math::vector3d_distance_squared(((struct actor *)act)->body_position, *(real_point3d *)(target + 0xbc));

        ((struct actor *)act)->mode_data.search.reachable = (uint8_t)(radius * radius > distance_squared);
    } else if (kind == 1 && ((struct actor *)act)->mode_data.search.firing_position != -1) {
        float distance_squared = halo::math::vector3d_distance_squared(((struct actor *)act)->body_position, *(&((struct actor *)act)->mode_data.search.position));

        if (distance_squared < 0.49f) {
            ((struct actor *)act)->mode_data.search.reachable = 1;
        } else if (distance_squared > 6.25f) {
            ((struct actor *)act)->mode_data.search.reachable = 0;
        } else {
            real_point3d in_view;

            halo::units::unit_add_marker_relative_offset(((actor *)act)->unit_index, 1, (float *)(act + 0xb0), 0, 0, &in_view);
            ((struct actor *)act)->mode_data.search.reachable = (uint8_t)(actor_evaluate_engagement_reachability(*(int16_t *)(act + 0x148), ((struct actor *)act)->mode_data.search.target_cluster,
                                                                         &in_view, &((struct actor *)act)->aim_origin, 0, 0, -1,
                                                                         (uint8_t)(((actor *)act)->active_unit_index !=
                                                                                   k_datum_index_none)) == 0);
        }
    }
    if (!((struct actor *)act)->mode_data.search.reachable && !act[0xa1]) {
        actor_prop_iterator iterator;
        datum_index prop_index;
        int16_t sharing = 0;
        int32_t close_idle = 0;

        actor_prop_iterator_init(actor_index, &iterator);
        for (prop_index = iterator.next; prop_index != k_datum_index_none;) {
            uint8_t *p = PROP(prop_index);
            int16_t prop_kind = ((struct prop *)p)->state;

            prop_index = ((struct prop *)p)->next_in_actor;
            if (prop_kind >= 2 && prop_kind <= 3 && !p[0x60] && !p[0x127] &&
                *(datum_index *)(p + 0x1c) != k_datum_index_none &&
                actor_targets_share_descriptor(actor_index, ((struct prop *)p)->owner_actor_index)) {
                uint8_t *other = ACTOR(((struct prop *)p)->owner_actor_index);

                sharing++;
                if (!other[0x6] && !other[0x504] &&
                    halo::math::vector3d_distance_squared(((struct actor *)other)->body_position, ((struct actor *)act)->body_position) < 0.64000005f) {
                    close_idle++;
                }
            }
        }
        if (!act[0xa0] && sharing >= (((struct actor *)act)->mode_data.search.stage != 1 ? 4 : 2)) {
            datum_index last_seen = -1;

            act[0x9c] = 1;
            if (((actor *)act)->target_unit_index != k_datum_index_none) {
                last_seen = *(datum_index *)(PROP(((actor *)act)->target_unit_index) + 0x7c);
            }
            if (((actor *)act)->encounter_index != k_datum_index_none) {
                ai_pursuit_note_object(actor_index, ((actor *)act)->encounter_index, ((struct actor *)act)->mode_data.search.firing_position, last_seen);
            }
        } else if ((int16_t)close_idle > 0) {
            ((struct actor *)act)->mode_data.search.reachable = 1;
        }
    }
    if (((struct actor *)act)->mode_data.search.reachable) {
        actor_movement_action_stop(actor_index);
        return act[0x9c];
    }
    kind = ((struct actor *)act)->mode_data.search.stage;
    if (kind == 0) {
        ok = actor_movement_set_destination_near_target(((actor *)act)->target_unit_index, actor_index, 2.5f);
    } else if (kind == 1) {
        ((actor *)act)->firing_position_index = -1;
        ok = actor_movement_set_destination_firing_position(actor_index, ((struct actor *)act)->mode_data.search.firing_position, 0);
    } else {
        actor_movement_action_stop(actor_index);
        return act[0x9c];
    }
    if (!ok) {
        act[0x9c] = 1;
        act[0x9d] = 1;
    }
    return act[0x9c];
}

extern "C" uint8_t actor_mode_search_process(datum_index actor_index)
{
    return halo::ai::search_mode(actor_index).process();
}

#undef ACTOR
#undef PROP

namespace c_actor_mode_search_tick {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern data_array *prop_data;
extern datum_index actor_get_target_prop_object_index(datum_index actor_index);
}
}

extern "C" void actor_mode_search_tick(datum_index actor_index);

/**
 * actor_mode_search_tick: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_tick.c.txt.
 *
 * @address 0x407d80
 */
void halo::ai::search_mode::tick()
{
    using namespace c_actor_mode_search_tick;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag;
    datum_index unit_index;

    if (act[0x9c]) {
        return;
    }
    actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    if (*(int16_t *)&((Actor *)actor_tag)->defensive_crouch_type == 4) {
        act[0x9f] = 1;
    } else {
        act[0x9f] = 0;
        if ((actor_tag[0] & 2) && ((struct actor *)act)->mode_data.search.stage == 0 && ((actor *)act)->target_combat_status == 5 &&
            (int8_t)((uint8_t *)prop_data->data + (((actor *)act)->target_unit_index & halo::k_slot_mask) * k_prop_size)[0x121] <= 2) {
            act[0x9f] = 1;
        }
    }
    if (!((struct actor *)act)->mode_data.search.reachable) {
        if (!act[0x504] && !act[0x6]) {
            ((struct actor *)act)->mode_data.search.elapsed_ticks += 1;
            if (((struct actor *)act)->mode_data.search.elapsed_ticks > 120) {
                act[0x9d] = 1;
                act[0x9c] = 1;
            }
        }
        return;
    }
    if (((struct actor *)act)->mode_data.search.remaining_ticks > 0) {
        ((struct actor *)act)->mode_data.search.remaining_ticks -= 1;
    }
    if (((struct actor *)act)->mode_data.search.remaining_ticks == 0) {
        act[0x9c] = 1;
    }
    unit_index = ((actor *)act)->unit_index;
    if (unit_index == k_datum_index_none) {
        return;
    }
    if (((struct actor *)act)->mode_data.search.stage == 0) {
        if (act[0x3bd]) {
            return;
        }
        if (act[0x9c] || ((struct actor *)act)->mode_data.search.remaining_ticks + 90 < ((struct actor *)act)->mode_data.search.duration_ticks) {
            ai_communication_broadcast(0xd, unit_index, actor_get_target_prop_object_index(actor_index), -1, -1, -1, 0);
            act[0x3bd] = 1;
        }
    } else if (((struct actor *)act)->mode_data.search.remaining_ticks == 0) {
        ai_communication_broadcast(0x12, unit_index, actor_get_target_prop_object_index(actor_index), -1, -1, -1, 0);
    }
}

extern "C" void actor_mode_search_tick(datum_index actor_index)
{
    halo::ai::search_mode(actor_index).tick();
}

#undef ACTOR
#undef TAG_DATA

namespace c_actor_mode_search_update {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
}
}

extern "C" void actor_mode_search_update(datum_index actor_index);

/**
 * actor_mode_search_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_search_update.c.txt.
 *
 * @address 0x407f40
 */
void halo::ai::search_mode::update()
{
    using namespace c_actor_mode_search_update;
    datum_index actor_index = datum;
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);

    if (act[0x504]) {
        ((actor *)act)->flee_reason = 3;
        ((actor *)act)->flee_source.code = 0;
    } else {
        int32_t total = ((struct actor *)act)->mode_data.search.duration_ticks;
        int32_t third = total / 3;

        if (third <= 90) {
            third = 90;
        }
        if (total - ((struct actor *)act)->mode_data.search.remaining_ticks < third && ((struct actor *)act)->mode_data.search.stage == 0) {
            ((actor *)act)->flee_reason = 3;
            ((actor *)act)->flee_source.code = 2;
        } else if (total - ((struct actor *)act)->mode_data.search.remaining_ticks < third && ((struct actor *)act)->mode_data.search.stage == 1) {
            ((actor *)act)->flee_reason = 3;
            ((actor *)act)->flee_source.code = 3;
            *(real_point3d *)(act + 0x3f0) = ((struct actor *)act)->mode_data.search.position;
        } else {
            ((actor *)act)->flee_reason = 1;
        }
    }
    ((struct actor *)act)->look_posture = 3;
    if (((struct actor *)act)->mode_data.search.stage == 0) {
        act[0x454] = (uint8_t)(((actor *)act)->target_combat_status >= ((actor_tag[0] & 0x10) ? 5 : 6));
    }
    act[0x426] = act[0x9f];
    act[0x427] = act[0x9f];
    act[0x428] = 0;
    act[0x424] = 0;
    act[0x425] = 1;
}

extern "C" void actor_mode_search_update(datum_index actor_index)
{
    halo::ai::search_mode(actor_index).update();
}

#undef ACTOR
#undef TAG_DATA

namespace c_actor_mode_sleep_update {
extern "C" {
extern data_array *actor_data;

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
}
}

extern "C" void actor_mode_sleep_update(datum_index actor_index);

/**
 * actor_mode_sleep_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_mode_sleep_update.c.txt.
 *
 * @address 0x408090
 */
void halo::ai::sleep_mode::update()
{
    using namespace c_actor_mode_sleep_update;
    datum_index actor_index = datum;
    *(int16_t *)(ACTOR(actor_index) + 0x3fc) = 0;
}

extern "C" void actor_mode_sleep_update(datum_index actor_index)
{
    halo::ai::sleep_mode(actor_index).update();
}

#undef ACTOR

