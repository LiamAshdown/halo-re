/**
 * @file include/halo/objects/object_damage.hpp
 * Object system API: object damage.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Damage, shield and vitality handling for one object.
 */
class ObjectDamage {
public:
    explicit ObjectDamage(uint32_t handle) : handle(handle) {}

    /**
     * Initialises the object's vitality maxima and stun thresholds, optionally from override values.
     *
     * Original register convention: uint32_t object_index in EAX (in_EAX); float *override_max_body_vitality.
     *
     * @address 0x004ed440
     */
    void initialize_shield_stun_thresholds(float *override_max_body_vitality, float *override_max_shield_vitality);

    /**
     * Applies shield and body regeneration, damage timers and the resulting state changes for the tick.
     *
     * Original register convention: stack=object_index.
     *
     * @address 0x004ed510
     */
    void update_vitality_and_regeneration();

    /**
     * Restores the object's body vitality to its maximum; returns whether anything changed.
     *
     * Original register convention: uint32_t object_index in EAX (in_EAX). Returns a bool (1 when the.
     *
     * @address 0x004ed9d0
     */
    uint8_t restore_full_body_vitality();

    /**
     * Sets the object's health frozen flag and updates dependent state.
     *
     * Original register convention: uint32_t object_index in EAX (in_EAX).
     *
     * @address 0x004eda20
     */
    void set_health_frozen_flag();

    /**
     * Sets the shield depleted flag and starts the recharge delay.
     *
     * Original register convention: uint32_t object_index in EDI (unaff_EDI).
     *
     * @address 0x004edb10
     */
    void set_shield_depleted_flag();

    /**
     * Starts shield recharging; returns whether it started.
     *
     * Original register convention: uint32_t object_index in EAX (in_EAX). Returns a bool (1 when recharge.
     *
     * @address 0x004edba0
     */
    uint8_t shield_recharge_start();

    /**
     * Applies a damage event to a target after testing line of sight from the epicentre.
     *
     * Original register convention: stack -> damage, object_index, recurse_siblings.
     *
     * @address 0x004eddb0
     */
    void apply_line_of_sight(damage_data *dd, int8_t continue_flag);

    /**
     * Applies a damage event to a target object: resolves the hit region and material, splits the damage between
     * shield and body and triggers the follow-up notifications.
     *
     * Original register convention: stack=(dd, target_object_index, node_index, region_index, material_index, plane).
     *
     * @address 0x004ee5e0
     */
    void apply_damage(damage_data *dd, int16_t hit_node_index, int16_t hit_region_index, int16_t hit_material_index,
    uint32_t hit_plane);

    /**
     * Applies body (non-shield) damage to a target object region and reports the notification flags and damage done.
     *
     * Original register convention: stack=(target_index, region_index, node_index, plane, geometry, material,
     * effect_block, dd, notify_flags, body_damage_out, material_multiplier_out, damage, is_local).
     *
     * @address 0x004ef2a0
     */
    void apply_body_damage(int32_t region_index, int32_t node_index, void *plane, ModelCollisionGeometry *geometry,
    ModelCollisionGeometryMaterial *material, DamageEffect *effect_block, damage_data *dd, uint32_t *notify_flags, float *body_damage_out,
    float *material_multiplier_out, float damage, uint8_t is_local);

    /**
     * Applies shield damage to a target object and reports the shield damage done and the damage left over.
     *
     * Original register convention: EBX -> record, stack=(target_index, geometry, material, effect_block,
     * notify_flags, shield_damage_out, remaining_damage, is_local, apply_state).
     *
     * @address 0x004ef820
     */
    void apply_shield_damage(ModelCollisionGeometry *geometry, ModelCollisionGeometryMaterial *material, DamageEffect *effect_block, uint32_t *notify_flags,
    float *shield_damage_out, float *remaining_damage, uint8_t is_local, uint8_t apply_state,
    object_shield_impulse_result *record);

    /**
     * Sends the HUD and network notifications for a damage result and applies the resulting impulse to the target.
     *
     * Original register convention: stack=(target_index, dd, notify_flags, shield_damage, body_damage, unused_6,
     * region_index, is_local).
     *
     * @address 0x004efcf0
     */
    void notify_and_impulse(damage_data *dd, uint32_t notify_flags, float shield_damage, float body_damage,
    uint32_t unused_6, int32_t region_index, uint32_t is_local);

    /**
     * Destroys a model region of an object.
     *
     * Original register convention: uint32_t object_index in EAX (in_EAX); int32_t region_index on the stack.
     *
     * @address 0x004f02d0
     */
    void destroy_region(int32_t region_index);

private:
    uint32_t handle;
};

/**
 * Damage effects, networked damage messages and the breakable surface table.
 */
class DamageSystem {
public:
    /**
     * Raises a multiplayer sound event at a throttled rate.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004ee370
     */
    static void throttled_multiplayer_sound_event();

    /**
     * Decodes a networked shield charge message, applies it and notifies the object.
     *
     * Original register convention: a pointer-to-pointer parameter in EAX (in_EAX).
     *
     * @address 0x004ee4d0
     */
    static void apply_shield_charge_and_notify(void **message);

    /**
     * Queues a networked pickup-denied event for a player.
     *
     * Original register convention: ECX -> key, EDI -> source, stack -> param_1.
     *
     * @address 0x004efbf0
     */
    static void queue_pickup_denied_event(void *param_1, int32_t key, uint32_t *source);

    /**
     * Decodes a networked linked-impulse message and applies it to the target object.
     *
     * Original register convention: a pointer-to-pointer parameter in EAX (in_EAX).
     *
     * @address 0x004efc80
     */
    static void apply_linked_impulse(void **message);

    /**
     * Forwards an effect notification (register arguments) to the object's type definition.
     *
     * Original register convention: EAX -> forwarded_eax, ECX -> forwarded_ecx (both pass straight through to.
     *
     * @address 0x004efff0
     */
    static void dispatch_effect_notify(uint32_t forwarded_eax, uint32_t forwarded_ecx);

    /**
     * Spawns the effect attached to a damage effect tag at an impact location, passing the surface normal and
     * incident vectors and the object that was hit.
     *
     * Original register convention: EAX=normal, ECX=incident, EBX=impact_position, EDI=object_index,
     * stack=(effect_tag, node_index).
     *
     * @address 0x004f0010
     */
    static void effect_new_at_location(datum_index effect_tag, int16_t node_index, real_vector3d *normal,
    real_vector3d *incident, real_point3d *impact_position, uint32_t object_index);

    /**
     * Dispatches a damage effect at a node of an object.
     *
     * Original register convention: none visible; whatever registers effect_new_on_object_with_node_table needs pass
     * through.
     *
     * @address 0x004f0250
     */
    static void effect_dispatch(int32_t push_value, int32_t node_object);

    /**
     * Marks every breakable surface of the structure BSP as intact again.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004ffd40
     */
    static void breakable_surfaces_reset();

    /**
     * Returns whether the breakable surface with the given bit index in the current structure BSP is still intact.
     *
     * @address 0x004ffda0
     */
    static int8_t breakable_surface_is_intact(int16_t bit_index);
};

/**
 * Description of one damage event: effect tag, responsible party, epicentre and multipliers.
 */
class DamageDataView {
public:
    explicit DamageDataView(damage_data *self) : self(self) {}

    /**
     * Initialises a damage_data record for the given damage effect tag with default flags, multipliers and unset
     * responsible party.
     *
     * Original register convention: damage_data *dd in EDX (in_EDX); datum_index damage_effect_tag on the.
     *
     * @address 0x004ed990
     */
    void initialize(datum_index damage_effect_tag);

    /**
     * Applies a damage event with an area of effect to the objects within its radius, collecting up to 64 candidates.
     *
     * Original register convention: damage_data *dd on the stack (param_1).
     *
     * @address 0x004edd30
     */
    void apply_area_effect();

private:
    damage_data *self;
};

}  // namespace halo::objects
