// item_update  (Ghidra: item_update, already named via cea-pdb hint on the shared "ground
// point" string, and functions.md's own summary)
// address 0x4bc5c0, size 2444 bytes
// name confidence: 0.75   rewrite confidence: 0.60 (raised by the phase-4 verification pass, which
//   re-derived this function against objdump disassembly rather than the decompilation; see the
//   notes in the body) (by far the largest function in this batch)
// evidence: types/items.h documents almost every field this function touches, field by field,
//   in its item_data section (flags bits 0x01/0x04/0x08/0x10/0x20, detonation_countdown,
//   resting_surface_index/resting_bsp_index, ignore_object_index, held_game_time,
//   resting_object_index/contact_point, rotation_axis/rotation_sine/rotation_cosine) -- this
//   rewrite follows that mapping directly; types/objects.h object (flags 0x010, position 0x05c,
//   velocity 0x068, forward 0x074, up 0x080, angular_velocity 0x08c, parent_object 0x11c),
//   object_type_mask (_object_mask_scenery | _object_mask_device = 0x3c0); types/tags.h Item
//   (item_flags 0x17c bit 0 = always_maintains_z_up, bit 2 = unaffected_by_gravity;
//   material_effects.tag_id at 0x254, collision_sound.tag_id at 0x264).
// register convention: already a plain __cdecl `int item_update(uint item_index)` in Ghidra's
//   own output; no unrecognized registers at the top level.
// UNSURE (extensive, function-wide, matching the same tradeoff src/objects/object_apply_damage.c
//   documents for its own largest/most opaque function): Ghidra elides essentially every
//   argument to FUN_00401a20 (the swept collision test -- this codebase already treats it as
//   opaque in src/units/unit_find_placement_position.c: "an unresolved collision/physics-module
//   ... preserved as opaque calls with the exact arguments Ghidra shows"), to
//   object_recompute_basis_from_marker_delta and vector3d_rotate_about_axis in the rotation-tail
//   block, and to any_local_player_within_10_units/material_effects_play_at_marker/object_collision_test_cluster_group/sound_start_at_location/FUN_00ffda0 (sound/effect
//   and material-lookup helpers entirely outside this module's address range). Rather than
//   reconstructing a plausible-but-unverified register binding for each, this rewrite keeps
//   Ghidra's own local-variable shapes (raw offset structs with a comment, not invented field
//   names) for the collision-result record and the rotation-tail block, and preserves every
//   opaque call with only the arguments Ghidra actually shows, per this task's priority of
//   literal control-flow/arithmetic preservation over readability when the two are in tension.
//   The object.flags bit this function gates on (0x800) is named _object_needs_cluster_update_bit
//   in types/objects.h from unrelated evidence; its use as the top-level "should this item even
//   update" gate here does not obviously match that name, and is preserved literally rather than
//   guessed at.
// reconciled: R04 0x006f1d20 uint8_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)
// reconciled: R04 follow-up: game.h is now included, so the local extern void *game_time_globals (0x006f1d6c) became game.h game_time_globals *game_time

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "projectiles.h" // collision_result

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_up3d_pointer; // 0x00696720
extern real_vector3d *global_forward3d_pointer; // 0x00696718, UNSURE name: same shape as
    // global_up3d_pointer, used here as the "always upright" forward fallback
extern real_point3d *global_origin3d_pointer; // 0x00696714 -> 0x0065c230, (0,0,0); named in
    // types/math.h. Copied verbatim into the sound_start_at_location bundle's third vector slot, i.e. that
    // slot is simply zeroed.
extern real_vector3d *global_reference_vector_0069672c; // 0x0069672c, UNSURE name/role: scaled
    // by a per-tick gravity-ish constant when a resting surface goes away underneath the item
extern real gravity_per_tick_0069c52c; // 0x0069c52c, UNSURE name: 0.0035651792, subtracted from
    // vertical velocity once per tick while the Item tag does not have unaffected_by_gravity
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern int16_t network_game_mode; // 0x00719720, UNSURE name: 0 local/authoritative
extern int16_t global_structure_bsp_index; // 0x0069e8d8
extern uint8_t global_structure_collision_bsp[]; // 0x00746f98, see item_accelerate.c
extern game_time_globals *game_time; // 0x006f1d6c, game.h; +0x0c game_time is the tick

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_list_membership_set(uint32_t object_index, char add); // 0x4f7450, objects module
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index); // 0x4f5350, UNSURE convention, see item_accelerate.c
extern void object_delete(uint32_t object_index); // 0x4f5bd0
extern void item_accelerate(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer); // 0x4bd080, this batch
extern void item_compute_rotation(uint32_t object_index); // 0x4bd500, this batch, EAX -> object_index
extern void item_align_to_normal_and_point(real_point3d *out_position, uint32_t item_index,
    real_vector3d *normal, real_point3d *point); // 0x4bd5d0, this batch
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale); // 0x4507a0, opaque, see src/objects/object_dispatch_effect_notify.c
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle,
    real cos_angle); // 0x4cd820, the four-argument form src/math/vector3d_rotate_about_axis.c
    // establishes. UNSURE: Ghidra recovered only the two stack arguments at both call sites
    // below -- v in EAX and axis in ECX are hidden -- so the two pointers are passed as 0.
extern void object_recompute_basis_from_marker_delta(void *marker, void *output_matrix); // 0x4f62f0,
    // UNSURE args: Ghidra recovered only 2 of the real 3 (obj in EAX is hidden here); real
    // signature: src/items/item_align_to_normal_and_point.c
extern void object_get_node_marker_address(uint32_t param_1); // 0x4f6000, UNSURE signature
extern void matrix4x3_inverse_transform_point(void); // 0x4cbf80, UNSURE signature, 0 args visible
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *in, real_matrix4x3 *m); // 0x4cbde0
extern uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t flags,
    uint32_t exclude_object_index, collision_result *result); // 0x401a20, src/physics; blam-cc: EAX origin, ECX target, stack rest
    // 0x401a20, UNSURE, opaque collision/physics-module routine (the world sweep); see
    // src/units/unit_find_placement_position.c. The first argument is the literal collision
    // mask 0x1ff3e9 -- objdump shows `push 0x1ff3e9` at 0x4bc714, the same literal
    // object_collision_test_cluster_group is called with twice below -- NOT the global_structure_collision_bsp pointer.
extern uint8_t any_local_player_within_10_units(const real_point3d *query_point); // 0x453330, src/game; blam-cc: EDX query_point
extern void material_effects_play_at_marker(uint32_t a1, uint32_t a2, void *a3, void *a4); // 0x453490, opaque, out of range
extern char breakable_surface_is_intact(void); // 0x4ffda0, AX -> surface index (R79)
extern char object_collision_test_cluster_group(uint32_t mask, uint32_t item_index); // 0x505490, opaque, out of range
extern double fabs(double x); // ABS is a single x87 FABS instruction
extern double sqrt(double x); // a single x87 FSQRT instruction
extern void sound_start_at_location(void *bundle, real *speed_factor); // 0x543d80, opaque, out of range;
    // UNSURE args: Ghidra recovered only the speed pointer (2nd here) at this call site, the
    // bundle built right before the call (position/speed/normal/forward/location) is passed
    // through a hidden register

// Per-tick physics update for a dropped/loose item. Airborne items are integrated forward,
// swept against the world, and either bounce, come fully to rest, or slide; resting items are
// re-validated against the surface/object they are resting on and either woken back up
// (through item_accelerate) or left to decay their spin. A rotation-valid item then has its
// forward/up basis rotated by its stored axis/sine/cosine. Finally the detonation countdown is
// ticked (deleting the object at zero) and, while held, held_game_time is refreshed.
int item_update(uint32_t item_index)
{
    object *obj = ((object_header *)object_data->data)[item_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);
    Item *tag = (Item *)tag_instances[obj->definition_tag & 0xffff].data;

    if ((obj->flags & 0x800) != 0 && obj->parent_object == (datum_index)0xffffffff) {
        // "always maintains z-up": snap the up vector back to world-up if it has drifted.
        if ((tag->item_flags & 0x01) != 0 && 0.0001f <= (real)fabs((double)(obj->up.k - 1.0f))) {
            real_vector3d cross1;
            obj->up = *global_up3d_pointer;
            vector3d_cross_product(&cross1, &obj->up, &obj->forward); // UNSURE operand order
            vector3d_cross_product(&obj->forward, &cross1, &obj->up); // UNSURE operand order
            if (vector3d_normalize_with_length(&obj->forward) == 0.0f) {
                obj->forward = *global_forward3d_pointer;
            }
        }

        if ((obj->flags & _object_at_rest_bit) == 0) {
            // ---------------- airborne: integrate, sweep, land or bounce ----------------
            real_vector3d velocity = obj->velocity;
            real vertical = velocity.k;
            real_point3d predicted;
            uint8_t hit_record[0x40]; // UNSURE shape, see file header (FUN_00401a20's output)

            if ((tag->item_flags & 0x04) == 0) { // NOT unaffected_by_gravity
                vertical -= gravity_per_tick_0069c52c;
            }
            predicted.x = obj->position.x + velocity.i;
            predicted.y = obj->position.y + velocity.j;
            predicted.z = obj->position.z + vertical;

            // 0x4bc700..0x4bc72f: EAX = &obj->position, ECX = &predicted
            if (collision_test_movement_segment_between_points(&obj->position, &predicted, 0x1ff3e9,
                    item->ignore_object_index, (collision_result *)hit_record) != 0) {
                // Offsets into the sweep result record, all read out of the disassembly (the
                // record base is `lea ecx,[esp+0x30]` at 0x4bc706, and every [esp+N] below is
                // that base + N - 0x30 once the intervening pushes are accounted for):
                //   +0x00 int16   surface type: 2 = structure BSP surface, 3 = another object
                //   +0x0c  12     UNSURE: an opaque 12-byte "location" sub-record. Nothing in
                //                 this function writes or reads its fields; it is handed
                //                 straight to material_effects_play_at_marker (0x4bc7ee) and to
                //                 object_set_position_and_relink (0x4bcb87). Most likely the
                //                 leaf/cluster pair those two need, but that is not proven.
                //   +0x18 point3d the contact point
                //   +0x24 vector3d the surface normal
                //   +0x34 uint32  UNSURE: passed to material_effects_play_at_marker as its second argument
                //   +0x38 uint32  the object the item hit, when the type is 3
                //   +0x44 int16   the BSP surface index, when the type is 2
                int16_t hit_type = *(int16_t *)(hit_record + 0x00);
                void *hit_location = (void *)(hit_record + 0x0c);
                real_point3d *hit_point = (real_point3d *)(hit_record + 0x18);
                real_vector3d *hit_normal = (real_vector3d *)(hit_record + 0x24);
                uint32_t material_effect_arg = *(uint32_t *)(hit_record + 0x34);
                uint32_t hit_object_index = *(uint32_t *)(hit_record + 0x38);
                int16_t hit_surface_index = *(int16_t *)(hit_record + 0x44);
                real speed_factor;

                predicted.x = hit_normal->i * 0.05f + predicted.x;
                predicted.y = hit_normal->j * 0.05f + predicted.y;
                predicted.z = hit_normal->k * 0.05f + predicted.z;

                speed_factor = (real)sqrt((double)(vertical * vertical + velocity.i * velocity.i +
                    velocity.j * velocity.j)) * 10.0f;
                // Written exactly as the original: the >= 0 test comes first, so a NaN
                // speed falls to the else and is clamped to 0 rather than left as NaN.
                if (0.0f <= speed_factor) {
                    if (1.0f < speed_factor) {
                        speed_factor = 1.0f;
                    }
                } else {
                    speed_factor = 0.0f;
                }

                if (*(int32_t *)&tag->material_effects.tag_id != -1 && any_local_player_within_10_units(hit_point) != 0) { // 0x4bc7d8: EDX = record + 0x18
                    material_effects_play_at_marker(8, material_effect_arg, hit_location, &speed_factor);
                }
                if (*(int32_t *)&tag->collision_sound.tag_id != -1) {
                    // The bundle is built at 0x4bc81e..0x4bc8a2 into [esp+0xa0..0xd0]; the
                    // field offsets below are that block's own, and the speed is NOT one of
                    // them -- it is pushed as the separate stack argument.
                    struct {
                        real_point3d position;       // 0x00 the clearance-corrected position
                        real_vector3d normal;        // 0x0c the surface normal
                        real_point3d reference;      // 0x18 global_origin3d, i.e. always (0,0,0)
                        datum_index location_leaf;   // 0x24 object.location_leaf_index
                        uint32_t pad_28;             // 0x28 left at zero
                        int16_t location_cluster;    // 0x2c object.location_cluster_index
                    } sound_args;
                    sound_args.position = predicted;
                    sound_args.normal = *hit_normal;
                    sound_args.reference = *global_origin3d_pointer;
                    sound_args.pad_28 = 0;
                    sound_args.location_leaf = obj->location_leaf_index;
                    sound_args.location_cluster = obj->location_cluster_index;
                    sound_start_at_location(&sound_args, &speed_factor);
                }

                if ((hit_type != 2 &&
                     (hit_type != 3 ||
                      (((1 << (((object_header *)object_data->data)[hit_object_index & 0xffff].type & 0x1f))
                        & (_object_mask_scenery | _object_mask_device)) == 0))) ||
                    hit_normal->k <= 0.7071f ||
                    0.05f <= -(hit_normal->k * vertical + hit_normal->i * velocity.i + hit_normal->j * velocity.j)) {
                    // Glancing hit: bounce.
                    real bounce = (hit_normal->i * velocity.i * -1.4f - hit_normal->j * velocity.j * 1.4f) -
                        hit_normal->k * vertical * 1.4f;
                    if (hit_type != 2 && 1.5f <= bounce) { // 0x4bcac3 fcomp: 1.5 <= bounce
                        bounce = 1.5f;
                    }
                    velocity.i += hit_normal->i * bounce;
                    velocity.j += hit_normal->j * bounce;
                    vertical += hit_normal->k * bounce;
                    predicted = *hit_point;
                    if (object_collision_test_cluster_group(0x1ff3e9, item_index) != 0) {
                        predicted.x = hit_normal->i * 0.05f + hit_point->x;
                        predicted.y = hit_normal->j * 0.05f + hit_point->y;
                        predicted.z = hit_normal->k * 0.05f + hit_point->z;
                    }
                    object_collision_test_cluster_group(0x1ff3e9, item_index);
                } else {
                    // Solid hit: come to rest on the surface.
                    real spin_dot;
                    predicted = *hit_point;
                    // out_position is &predicted: 0x4bc957 is `lea eax,[esp+0x14]`, which after
                    // the two pushes at 0x4bc94d/0x4bc956 is the same slot the position is
                    // seeded into one instruction earlier and the same slot
                    // object_set_position_and_relink reads at the end of the branch. So this
                    // call refines `predicted` in place; it is not a discarded output.
                    item_align_to_normal_and_point(&predicted, item_index, hit_normal, hit_point);
                    spin_dot = hit_normal->i * obj->angular_velocity.i +
                        hit_normal->k * obj->angular_velocity.k + hit_normal->j * obj->angular_velocity.j;
                    obj->angular_velocity.i = hit_normal->i * spin_dot;
                    obj->angular_velocity.j = hit_normal->j * spin_dot;
                    obj->angular_velocity.k = spin_dot * hit_normal->k;
                    velocity.i = 0.0f;
                    velocity.j = 0.0f;
                    vertical = 0.0f;

                    if (current_game_engine == 0 && obj->owner_linkage == (uint32_t)0xffffffff) {
                        object_list_membership_set(item_index, 1);
                    }
                    obj->flags |= _object_at_rest_bit;
                    if (hit_type == 2) {
                        item->flags |= _item_at_rest_on_structure_bit;
                        item->resting_surface_index = hit_surface_index;
                        item->resting_bsp_index = global_structure_bsp_index;
                    } else {
                        item->flags |= _item_at_rest_on_object_bit;
                        item->resting_object_index = hit_object_index;
                        // 0x4bca0b..0x4bca29: object_get_node_marker_address(0) returns the
                        // hit object's marker matrix in EAX, then matrix4x3_inverse_transform_point
                        // runs with ECX = that matrix, EDX = &item->contact_point and
                        // ESI = hit_point, i.e. it stores the contact point in the hit object's
                        // local space. UNSURE: the exact parameter order of both callees.
                        object_get_node_marker_address(0);
                        matrix4x3_inverse_transform_point(); // -> item->contact_point, from hit_point
                    }
                    item->rotation_axis = *hit_normal;
                    item_compute_rotation(item_index);
                    item->ignore_object_index = (datum_index)0xffffffff;
                }
            }

            obj->velocity.i = velocity.i;
            obj->velocity.j = velocity.j;
            obj->velocity.k = vertical;
            // UNSURE: 0x4bcb8e pushes hit_location (the record's +0x0c block) as a third
            // argument here, on top of ESI = &predicted and EDI = item_index. The extern below
            // keeps the two-argument shape the rest of the codebase uses for this function, so
            // that third argument is not expressible; it is the same block material_effects_play_at_marker gets.
            object_set_position_and_relink(&predicted, item_index);
        } else if ((tag->item_flags & 0x04) == 0) {
            // ---------------- resting: re-validate the surface/object ----------------
            object_marker marker;
            uint32_t flags = item->flags;
            object_get_node_local_transform(item_index, "ground point", &marker, 1);

            if ((flags & _item_at_rest_on_structure_bit) == 0 ||
                item->resting_surface_index == -1 ||
                item->resting_bsp_index != global_structure_bsp_index) {
                if ((flags & _item_at_rest_on_object_bit) != 0) {
                    if (object_try_and_get(item->resting_object_index, 0xffffffff) == 0) {
                        real_vector3d fall = {
                            gravity_per_tick_0069c52c * global_reference_vector_0069672c->i,
                            gravity_per_tick_0069c52c * global_reference_vector_0069672c->j,
                            gravity_per_tick_0069c52c * global_reference_vector_0069672c->k
                        };
                        item->flags = flags & ~(uint32_t)_item_at_rest_on_object_bit;
                        item_accelerate(item_index, &fall, 0);
                    } else {
                        real_point3d world_contact;
                        object_get_node_marker_address(0); // UNSURE args, see file header
                        matrix4x3_transform_point(&world_contact, &item->contact_point, 0); // UNSURE args
                        item_align_to_normal_and_point(0, item_index,
                            &item->rotation_axis, &world_contact);
                    }
                }
            } else if ((*(uint8_t *)(global_structure_collision_bsp + 0x40 +
                        (uint32_t)(uint16_t)item->resting_surface_index * 0x0c + 8) & 8) != 0 &&
                       breakable_surface_is_intact() == 0) {
                real_vector3d fall = {
                    gravity_per_tick_0069c52c * global_reference_vector_0069672c->i,
                    gravity_per_tick_0069c52c * global_reference_vector_0069672c->j,
                    gravity_per_tick_0069c52c * global_reference_vector_0069672c->k
                };
                item->flags = flags & ~(uint32_t)_item_at_rest_on_structure_bit;
                item->resting_surface_index = -1;
                item_accelerate(item_index, &fall, 0);
            }

            obj->angular_velocity.i *= 0.9f;
            obj->angular_velocity.j *= 0.9f;
            obj->angular_velocity.k *= 0.9f;
            item_compute_rotation(item_index);
        }

        if ((item->flags & _item_rotation_valid_bit) != 0) {
            // ---------------- rotation-valid tail ----------------
            // UNSURE (whole block): Ghidra elides essentially every argument here; the shape
            // (rotate a copy of the object's basis by rotation_axis/sine/cosine, optionally
            // re-deriving it from the "ground point" marker first) is preserved, the exact
            // buffers are not. See file header.
            if (network_game_mode == 0 && (obj->flags & _object_at_rest_bit) != 0) {
                object_marker marker;
                if (object_get_node_local_transform(item_index, "ground point", &marker, 1) != 0) {
                    vector3d_rotate_about_axis(0, 0, item->rotation_sine, item->rotation_cosine); // UNSURE: v/axis
                    vector3d_rotate_about_axis(0, 0, item->rotation_sine, item->rotation_cosine); // UNSURE: v/axis
                    vector3d_cross_product(0, 0, 0); // UNSURE, see file header
                    vector3d_cross_product(0, 0, 0); // UNSURE, see file header
                    vector3d_normalize_with_length(0);
                    vector3d_normalize_with_length(0);
                    vector3d_normalize_with_length(0);
                    object_recompute_basis_from_marker_delta(&marker, 0);
                }
            } else {
                vector3d_rotate_about_axis(0, 0, item->rotation_sine, item->rotation_cosine); // UNSURE: v/axis
                vector3d_rotate_about_axis(0, 0, item->rotation_sine, item->rotation_cosine); // UNSURE: v/axis
            }
            vector3d_normalize_with_length(0);
            vector3d_cross_product(&obj->up, 0, 0); // UNSURE, see file header
            vector3d_cross_product(0, 0, 0); // UNSURE, see file header
            vector3d_normalize_with_length(0);
        }
    }

    if (item->detonation_countdown > 0) {
        item->detonation_countdown -= 1;
        if (item->detonation_countdown == 0) {
            effect_new_on_object(item_index, 0xffffffff, 0, 0, 0, 0);
            object_delete(item_index);
        }
    }

    if ((item->flags & _item_in_inventory_bit) != 0) {
        item->held_game_time = game_time->game_time; // +0x0c
    }

    // The original is `return CONCAT31((int3)(uVar7 >> 8), 1)`: only AL is meaningful, and
    // the upper three bytes are whatever the last computation left in EAX. Every caller tests
    // the byte, so this returns a plain 1.
    return 1;
}

#if 0
Original Ghidra decompilation (0x4bc5c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl item_update(uint item_index)

{
  uint *puVar1;
  float fVar2;
  undefined *puVar3;
  char cVar4;
  short sVar5;
  undefined4 uVar6;
  uint uVar7;
  int iVar8;
  float *pfVar9;
  float *pfVar10;
  bool bVar11;
  float10 fVar12;
  float local_144;
  float local_140;
  float local_13c;
  uint *local_138;
  float local_134;
  float local_130;
  float local_12c;
  int local_128;
  int local_124;
  short local_120 [6];
  undefined1 local_114 [12];
  float local_108;
  float local_104;
  float local_100;
  float local_fc;
  float local_f8;
  float local_f4;
  undefined4 local_ec;
  uint local_e8;
  undefined2 local_dc;
  float local_b0 [4];
  float local_a0;
  float local_9c;
  undefined4 local_98;
  undefined4 local_94;
  undefined4 local_90;
  uint local_8c;
  uint local_88;
  undefined1 local_78 [56];
  float local_40 [15];

  puVar3 = PTR_DAT_00696720;
  local_124 = (item_index & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_124);
  local_128 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((puVar1[4] & 0x800) == 0) || (puVar1[0x47] != 0xffffffff)) goto LAB_004bceea;
  if (((*(byte *)(local_128 + 0x17c) & 1) != 0) && (0.0001 <= ABS((float)puVar1[0x22] - 1.0))) {
    puVar1[0x20] = *(uint *)PTR_DAT_00696720;
    puVar1[0x21] = *(uint *)(puVar3 + 4);
    puVar1[0x22] = *(uint *)(puVar3 + 8);
    vector3d_cross_product(puVar1 + 0x20);
    vector3d_cross_product(&local_144);
    fVar12 = (float10)vector3d_normalize_with_length();
    puVar3 = PTR_DAT_00696718;
    if ((float10)0.0 == fVar12) {
      puVar1[0x1d] = *(uint *)PTR_DAT_00696718;
      puVar1[0x1e] = *(uint *)(puVar3 + 4);
      puVar1[0x1f] = *(uint *)(puVar3 + 8);
    }
  }
  iVar8 = local_128;
  if ((puVar1[4] & 0x20) == 0) {
    local_134 = (float)puVar1[0x1a];
    local_130 = (float)puVar1[0x1b];
    local_12c = (float)puVar1[0x1c];
    if ((*(byte *)(local_128 + 0x17c) & 4) == 0) {
      local_12c = local_12c - _DAT_0069c52c;
    }
    local_144 = local_134 + (float)puVar1[0x17];
    local_140 = local_130 + (float)puVar1[0x18];
    local_13c = local_12c + (float)puVar1[0x19];
    cVar4 = FUN_00401a20(0x1ff3e9,puVar1[0x80],local_120);
    if (cVar4 != '\0') {
      local_144 = local_fc * 0.05 + local_144;
      local_140 = local_f8 * 0.05 + local_140;
      local_13c = local_f4 * 0.05 + local_13c;
      local_138 = (uint *)(SQRT(local_12c * local_12c +
                                local_134 * local_134 + local_130 * local_130) * 10.0);
      if (0.0 <= (float)local_138) {
        if (1.0 < (float)local_138) {
          local_138 = (uint *)0x3f800000;
        }
      }
      else {
        local_138 = (uint *)0x0;
      }
      if ((*(int *)(iVar8 + 0x254) != -1) && (cVar4 = FUN_00453330(), cVar4 != '\0')) {
        FUN_00453490(8,local_ec,local_114,local_138);
      }
      if (*(int *)(local_128 + 0x264) != -1) {
        local_b0[0] = local_144;
        local_b0[1] = local_140;
        local_b0[2] = local_13c;
        local_b0[3] = local_fc;
        local_a0 = local_f8;
        local_9c = local_f4;
        local_98 = *(undefined4 *)PTR_DAT_00696714;
        local_94 = *(undefined4 *)(PTR_DAT_00696714 + 4);
        local_90 = *(undefined4 *)(PTR_DAT_00696714 + 8);
        local_8c = puVar1[0x26];
        local_88 = puVar1[0x27];
        FUN_00543d80(local_138);
      }
      if ((((local_120[0] != 2) &&
           ((local_120[0] != 3 ||
            ((1 << (*(byte *)(*(int *)(DAT_008603b0 + 0x34) + 3 + (local_e8 & 0xffff) * 0xc) & 0x1f)
             & 0x3c0U) == 0)))) || (local_f4 <= 0.7071)) ||
         (0.05 <= -(local_f4 * local_12c + local_fc * local_134 + local_f8 * local_130))) {
        fVar2 = (local_fc * local_134 * -1.4 - local_f8 * local_130 * 1.4) -
                local_f4 * local_12c * 1.4;
        if ((local_120[0] != 2) && (1.5 <= fVar2)) {
          fVar2 = 1.5;
        }
        local_134 = local_fc * fVar2 + local_134;
        local_144 = local_108;
        local_140 = local_104;
        local_13c = local_100;
        local_130 = local_f8 * fVar2 + local_130;
        local_12c = fVar2 * local_f4 + local_12c;
        cVar4 = FUN_00505490(0x1ff3e9,item_index);
        if (cVar4 != '\0') {
          local_144 = local_fc * 0.05 + local_108;
          local_140 = local_f8 * 0.05 + local_104;
          local_13c = local_f4 * 0.05 + local_100;
        }
        FUN_00505490(0x1ff3e9,item_index);
      }
      else {
        local_144 = local_108;
        local_140 = local_104;
        local_13c = local_100;
        item_align_to_normal_and_point(&local_fc,&local_108);
        bVar11 = DAT_006f1d20 == 0;
        local_12c = 0.0;
        local_130 = 0.0;
        local_134 = 0.0;
        fVar2 = local_fc * (float)puVar1[0x23] +
                local_f4 * (float)puVar1[0x25] + local_f8 * (float)puVar1[0x24];
        puVar1[0x23] = (uint)(local_fc * fVar2);
        puVar1[0x24] = (uint)(local_f8 * fVar2);
        puVar1[0x25] = (uint)(fVar2 * local_f4);
        if ((bVar11) && (puVar1[0x30] == 0xffffffff)) {
          FUN_004f7450(1);
        }
        puVar1[4] = puVar1[4] | 0x20;
        sVar5 = DAT_0069e8d8;
        if (local_120[0] == 2) {
          puVar1[0x7d] = puVar1[0x7d] | 8;
          *(undefined2 *)((int)puVar1 + 0x1fa) = local_dc;
          *(short *)(puVar1 + 0x7f) = sVar5;
        }
        else {
          puVar1[0x7d] = puVar1[0x7d] | 0x10;
          puVar1[0x82] = local_e8;
          object_get_node_marker_address(0);
          matrix4x3_inverse_transform_point();
        }
        puVar1[0x86] = (uint)local_fc;
        puVar1[0x87] = (uint)local_f8;
        puVar1[0x88] = (uint)local_f4;
        item_compute_ground_alignment_rotation();
        puVar1[0x80] = 0xffffffff;
      }
    }
    puVar1[0x1a] = (uint)local_134;
    puVar1[0x1b] = (uint)local_130;
    puVar1[0x1c] = (uint)local_12c;
    object_set_position_and_relink(local_114);
  }
  else if ((*(byte *)(local_128 + 0x17c) & 4) == 0) {
    object_get_node_local_transform(item_index,"ground point",local_120,1);
    local_138 = (uint *)puVar1[0x7d];
    if (((((uint)local_138 & 8) == 0) || (*(short *)((int)puVar1 + 0x1fa) == -1)) ||
       ((short)puVar1[0x7f] != DAT_0069e8d8)) {
      if (((uint)local_138 & 0x10) != 0) {
        iVar8 = object_try_and_get(0xffffffff);
        if (iVar8 == 0) {
          local_144 = _DAT_0069c52c * *(float *)PTR_DAT_0069672c;
          local_140 = _DAT_0069c52c * *(float *)(PTR_DAT_0069672c + 4);
          local_13c = _DAT_0069c52c * *(float *)(PTR_DAT_0069672c + 8);
          uVar7 = (uint)local_138 & 0xffffffef;
          goto LAB_004bccf4;
        }
        local_138 = puVar1 + 0x83;
        uVar6 = object_get_node_marker_address(0);
        uVar6 = matrix4x3_transform_point(uVar6);
        item_align_to_normal_and_point(puVar1 + 0x86,uVar6);
      }
    }
    else if (((*(byte *)(*(int *)(DAT_00746f98 + 0x40) + 8 + *(short *)((int)puVar1 + 0x1fa) * 0xc)
              & 8) != 0) && (cVar4 = FUN_004ffda0(), cVar4 == '\0')) {
      local_144 = _DAT_0069c52c * *(float *)PTR_DAT_0069672c;
      local_140 = _DAT_0069c52c * *(float *)(PTR_DAT_0069672c + 4);
      local_13c = _DAT_0069c52c * *(float *)(PTR_DAT_0069672c + 8);
      uVar7 = (uint)local_138 & 0xfffffff7;
      *(undefined2 *)((int)puVar1 + 0x1fa) = 0xffff;
LAB_004bccf4:
      puVar1[0x7d] = uVar7;
      item_accelerate(&local_144,0);
    }
    puVar1[0x23] = (uint)((float)puVar1[0x23] * 0.9);
    puVar1[0x24] = (uint)((float)puVar1[0x24] * 0.9);
    puVar1[0x25] = (uint)((float)puVar1[0x25] * 0.9);
    item_compute_ground_alignment_rotation();
  }
  if ((puVar1[0x7d] & 4) != 0) {
    if (((DAT_00719720 == 0) && ((puVar1[4] & 0x20) != 0)) &&
       (sVar5 = object_get_node_local_transform(item_index,"ground point",local_78,1), sVar5 != 0))
    {
      uVar7 = puVar1[0x8a];
      pfVar9 = local_40;
      pfVar10 = local_b0;
      for (iVar8 = 0xd; iVar8 != 0; iVar8 = iVar8 + -1) {
        *pfVar10 = *pfVar9;
        pfVar9 = pfVar9 + 1;
        pfVar10 = pfVar10 + 1;
      }
      vector3d_rotate_about_axis(puVar1[0x89],uVar7);
      uVar6 = vector3d_rotate_about_axis(puVar1[0x89],puVar1[0x8a]);
      vector3d_cross_product(uVar6);
      vector3d_cross_product(&local_a0);
      vector3d_normalize_with_length();
      vector3d_normalize_with_length();
      vector3d_normalize_with_length();
      object_recompute_basis_from_marker_delta(local_78,local_b0);
    }
    else {
      vector3d_rotate_about_axis(puVar1[0x89],puVar1[0x8a]);
      vector3d_rotate_about_axis(puVar1[0x89],puVar1[0x8a]);
    }
    vector3d_normalize_with_length();
    vector3d_cross_product(puVar1 + 0x20);
    vector3d_cross_product(&local_134);
    vector3d_normalize_with_length();
  }
LAB_004bceea:
  uVar7 = (uint)(ushort)puVar1[0x7e];
  if (0 < (short)(ushort)puVar1[0x7e]) {
    uVar7 = uVar7 - 1;
    *(short *)(puVar1 + 0x7e) = (short)uVar7;
    if ((short)uVar7 == 0) {
      FUN_004507a0(item_index,0xffffffff,0,0,0,0);
      uVar7 = object_delete();
    }
  }
  if ((puVar1[0x7d] & 1) != 0) {
    uVar7 = *(uint *)(DAT_006f1d6c + 0xc);
    puVar1[0x81] = uVar7;
  }
  return CONCAT31((int3)(uVar7 >> 8),1);
}
#endif
