// item_update  (Ghidra: item_update, already named via cea-pdb hint on the shared "ground point" string)
// address 0x4bc5c0, size 2444 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4bc5c0..0x4bcf4b. The draft's tumble tail called the cross product and normaliser with
//   NULL pointers and the rotations without vectors (a crash for every tumbling item: dropped weapons and
//   equipment), and several helpers (marker address, inverse transform, material effects, cluster tests, sound)
//   without their operands. Stack: item. For a free item: upright tags are stood up; a moving item sweeps one tick
//   of velocity (gravity unless tag flag 4), plays its material effect near a local player and its collision
//   sound, settles on a floor (structure or a static object, normal.k > 0.7071, approach < 0.05) or bounces
//   (-1.4 n.v, at most 1.5 off objects), and is relinked in the swept leaf; a resting item falls when its breakable
//   surface or supporting object goes away, else follows the support, its spin decaying. A tumbling item (flag 4)
//   turns its basis (about the "ground point" when at rest in single player). Then the detonation countdown and
//   the held time.
// blam-cc: stack -> item_index

// FIXED 2026-09-28: global_origin3d_pointer here is the global at its address comment, global_zero_vector3d_pointer (the name belonged to another
// global at a different address, so the link bound it there).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "projectiles.h" // collision_result
#include "effects.h"
#include "sound.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;        // 0x008603b0
extern tag_instance *tag_instances;    // 0x0087bc14
extern game_time_globals *game_time;   // 0x006f1d6c
extern game_engine_definition *current_game_engine;
extern int16_t network_game_mode;   // 0x00719720
extern int16_t global_structure_bsp_index; // 0x0069e8d8
extern uint8_t *global_structure_collision_bsp; // 0x00746f98, +0x40 the surfaces (0xc each)
extern real_vector3d *global_origin3d_pointer; // 0x00696714
extern real_vector3d *global_forward3d_pointer;     // 0x00696718
extern real_vector3d *global_up3d_pointer;          // 0x00696720
extern real_vector3d *global_down3d_pointer;        // 0x0069672c
extern float k_physics_gravity;           // 0x0069c52c
extern char s_ground_point_marker[];   // 0x0066b180 "ground point"

extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820
extern uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t flags,
    uint32_t exclude_object_index, collision_result *result); // 0x401a20, EAX, ECX, stack
extern uint8_t any_local_player_within_10_units(const real_point3d *query_point); // 0x453330, EDX
extern void material_effects_play_at_marker(uint32_t material_effects_tag, int16_t material_type, int16_t sub_effect_index,
    uint32_t *location_bundle, uint32_t sound_param, real_point3d *position, real_vector3d *offset); // 0x453490, EAX, stack, EDX, EDI
extern datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale); // 0x543d80
extern void item_align_to_normal_and_point(real_point3d *out_position, uint32_t item_index, real_vector3d *normal,
    real_point3d *point); // 0x4bd5d0, EAX, ECX, stack
extern void object_list_membership_set(uint32_t object_index, char add); // 0x4f7450, ECX, stack
extern real_matrix4x3 *object_get_node_marker_address(uint32_t object_index, int16_t node_index); // 0x4f6000, EAX, stack
extern void matrix4x3_inverse_transform_point(real_matrix4x3 *m, real_point3d *out, real_point3d *point); // 0x4cbf80, ECX, EDX, ESI
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0, EAX, EDX, stack
extern void item_compute_rotation(uint32_t object_index); // 0x4bd500, EAX
extern uint8_t object_collision_test_cluster_group(uint32_t flags, real_point3d *position, uint32_t exclude_object_index); // 0x505490, stack, EDI
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index,
    bsp_leaf_reference *location); // 0x4f5350, ESI, EDI, stack
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t maximum_markers); // 0x4f6080
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern int8_t breakable_surface_is_intact(int16_t bit_index); // 0x4ffda0, AX
extern void item_accelerate(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer); // 0x4bd080, EAX, stack
extern void object_recompute_basis_from_marker_delta(object *obj, object_marker *marker, real_matrix4x3 *output_matrix); // 0x4f62f0, EAX, stack
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source); // 0x4507a0
extern void object_delete(uint32_t object_index); // 0x4f5bd0, EAX

extern double fabs(double x);
extern double sqrt(double x);

#define F(p, o) (*(float *)((p) + (o)))

// the item falls: accelerate by one tick of gravity along global down
static void item_start_falling(uint32_t item_index)
{
    real_vector3d fall;

    fall.i = k_physics_gravity * global_down3d_pointer->i;
    fall.j = k_physics_gravity * global_down3d_pointer->j;
    fall.k = k_physics_gravity * global_down3d_pointer->k;
    item_accelerate(item_index, &fall, 0);
}

uint8_t item_update(uint32_t item_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[item_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;   // [esp+0x28]
    real_vector3d *forward = &((item_object *)obj)->base.forward;
    real_vector3d *up = &((item_object *)obj)->base.up;

    if ((((item_object *)obj)->base.flags & 0x800) && ((item_object *)obj)->base.parent_object == k_datum_index_none) {
        // 0x4bc621: items that must stay upright are stood back up
        if ((((Item *)tag)->item_flags & 1) && !(fabs(((item_object *)obj)->base.up.k - 1.0f) < 9.999999747378752e-05)) {
            real_vector3d side;

            *up = *global_up3d_pointer;
            vector3d_cross_product(&side, forward, up);
            vector3d_cross_product(forward, up, &side);
            if (vector3d_normalize_with_length(forward) == 0.0f) {
                *forward = *global_forward3d_pointer;
            }
        }

        if (!(((item_object *)obj)->base.flags & 0x20)) {
            // 0x4bc6c4: moving
            real_vector3d velocity = ((item_object *)obj)->base.velocity;   // [esp+0x1c]
            real_point3d target;                                      // [esp+0xc]
            collision_result hit;                                     // [esp+0x30]

            if (!(((Item *)tag)->item_flags & 4)) {
                velocity.k -= k_physics_gravity;
            }
            target.x = ((item_object *)obj)->base.position.x + velocity.i;
            target.y = ((item_object *)obj)->base.position.y + velocity.j;
            target.z = ((item_object *)obj)->base.position.z + velocity.k;
            if (collision_test_movement_segment_between_points(&((item_object *)obj)->base.position, &target, 0x1ff3e9,
                                                               ((item_object *)obj)->item.ignore_object_index, &hit)) {
                real speed_factor;                                    // [esp+0x18]
                int16_t hit_type = *(int16_t *)&hit;

                target.x += hit.plane.normal.i * 0.05f;
                target.y += hit.plane.normal.j * 0.05f;
                target.z += hit.plane.normal.k * 0.05f;
                speed_factor = (real)sqrt(velocity.j * velocity.j + velocity.i * velocity.i + velocity.k * velocity.k) * 10.0f;
                if (!(speed_factor >= 0.0f)) {
                    speed_factor = 0.0f;
                } else if (!(speed_factor <= 1.0f)) {
                    speed_factor = 1.0f;
                }
                if (*(datum_index *)&((Item *)tag)->material_effects.tag_id != k_datum_index_none && any_local_player_within_10_units(&hit.point)) {
                    material_effects_play_at_marker(*(datum_index *)&((Item *)tag)->material_effects.tag_id, 8, *(int16_t *)&hit.material_type,
                                                    (uint32_t *)&hit.leaf, *(uint32_t *)&speed_factor, &hit.point,
                                                    &hit.plane.normal);
                }
                if (*(datum_index *)&((Item *)tag)->collision_sound.tag_id != k_datum_index_none) {
                    sound_placement placement;                        // [esp+0xa0]

                    *(real_point3d *)&placement.position = target;
                    *(real_vector3d *)&placement.forward = hit.plane.normal;
                    *(real_vector3d *)&placement.velocity = *global_origin3d_pointer;
                    placement.leaf_index = ((item_object *)obj)->base.location_leaf_index;
                    *(int32_t *)&placement.cluster_index = *(int32_t *)&((item_object *)obj)->base.location_cluster_index;
                    sound_start_at_location(*(datum_index *)&((Item *)tag)->collision_sound.tag_id, &placement, speed_factor);
                }
                if ((hit_type == 2 ||
                     (hit_type == 3 &&
                      ((1u << (((uint8_t *)object_data->data)[(hit.object_index & 0xffff) * 0xc + 3] & 0x1f)) & 0x3c0))) &&
                    hit.plane.normal.k > 0.7071f &&
                    -(hit.plane.normal.j * velocity.j + hit.plane.normal.i * velocity.i +
                      hit.plane.normal.k * velocity.k) < 0.05f) {
                    // 0x4bc935: settle on a floor
                    real spin;

                    target = hit.point;
                    item_align_to_normal_and_point(&target, item_index, &hit.plane.normal, &hit.point);
                    spin = ((item_object *)obj)->base.angular_velocity.j * hit.plane.normal.j + ((item_object *)obj)->base.angular_velocity.k * hit.plane.normal.k +
                           ((item_object *)obj)->base.angular_velocity.i * hit.plane.normal.i;
                    velocity.i = 0.0f;
                    velocity.j = 0.0f;
                    velocity.k = 0.0f;
                    ((item_object *)obj)->base.angular_velocity.i = hit.plane.normal.i * spin;
                    ((item_object *)obj)->base.angular_velocity.j = hit.plane.normal.j * spin;
                    ((item_object *)obj)->base.angular_velocity.k = spin * hit.plane.normal.k;
                    if (current_game_engine == 0 && (datum_index)((item_object *)obj)->base.owner_linkage == k_datum_index_none) {
                        object_list_membership_set(item_index, 1);
                    }
                    ((item_object *)obj)->base.flags |= 0x20;
                    if (hit_type != 2) {
                        ((item_object *)obj)->item.flags |= 0x10;
                        ((item_object *)obj)->item.resting_object_index = hit.object_index;
                        matrix4x3_inverse_transform_point(object_get_node_marker_address(hit.object_index, 0),
                                                          &((item_object *)obj)->item.contact_point, &hit.point);
                    } else {
                        ((item_object *)obj)->item.flags |= 8;
                        ((item_object *)obj)->item.resting_surface_index = *(int16_t *)((uint8_t *)&hit + 0x44);
                        ((item_object *)obj)->item.resting_bsp_index = global_structure_bsp_index;
                    }
                    ((item_object *)obj)->item.rotation_axis = hit.plane.normal;
                    item_compute_rotation(item_index);
                    ((item_object *)obj)->item.ignore_object_index = k_datum_index_none;
                } else {
                    // 0x4bca89: bounce off (at most 1.5 off an object)
                    real impulse = hit.plane.normal.i * velocity.i * -1.4f - hit.plane.normal.j * velocity.j * 1.4f -
                                   hit.plane.normal.k * velocity.k * 1.4f;

                    if (hit_type != 2 && !(1.5f > impulse)) {
                        impulse = 1.5f;
                    }
                    velocity.i += hit.plane.normal.i * impulse;
                    velocity.j += hit.plane.normal.j * impulse;
                    velocity.k += hit.plane.normal.k * impulse;
                    target = hit.point;
                    if (object_collision_test_cluster_group(0x1ff3e9, &target, item_index)) {
                        target.x = hit.plane.normal.i * 0.05f + hit.point.x;
                        target.y = hit.plane.normal.j * 0.05f + hit.point.y;
                        target.z = hit.plane.normal.k * 0.05f + hit.point.z;
                    }
                    object_collision_test_cluster_group(0x1ff3e9, &target, item_index);
                }
            }
            // 0x4bcb78
            ((item_object *)obj)->base.velocity = velocity;
            object_set_position_and_relink(&target, item_index, &hit.leaf);
        } else if (!(((Item *)tag)->item_flags & 4)) {
            // 0x4bcbb7: resting; fall when the support goes away
            object_marker marker;                                     // [esp+0x30]
            uint32_t flags = ((item_object *)obj)->item.flags;

            object_get_node_local_transform(item_index, s_ground_point_marker, &marker, 1);
            if ((flags & 8) && ((item_object *)obj)->item.resting_surface_index != -1 && ((item_object *)obj)->item.resting_bsp_index == global_structure_bsp_index) {
                uint8_t *surface = *(uint8_t **)(global_structure_collision_bsp + 0x40) + ((item_object *)obj)->item.resting_surface_index * 0xc;

                if ((surface[8] & 8) && !breakable_surface_is_intact((int16_t)surface[9])) {
                    ((item_object *)obj)->item.flags = flags & ~8u;
                    ((item_object *)obj)->item.resting_surface_index = -1;
                    item_start_falling(item_index);
                }
            } else if (flags & 0x10) {
                datum_index support = ((item_object *)obj)->item.resting_object_index;

                if (object_try_and_get(support, 0xffffffff) != 0) {
                    real_point3d contact;

                    matrix4x3_transform_point(&contact, &((item_object *)obj)->item.contact_point,
                                              object_get_node_marker_address(support, 0));
                    item_align_to_normal_and_point(0, item_index, &((item_object *)obj)->item.rotation_axis, &contact);
                } else {
                    ((item_object *)obj)->item.flags = flags & ~0x10u;
                    item_start_falling(item_index);
                }
            }
            ((item_object *)obj)->base.angular_velocity.i *= 0.9f;
            ((item_object *)obj)->base.angular_velocity.j *= 0.9f;
            ((item_object *)obj)->base.angular_velocity.k *= 0.9f;
            item_compute_rotation(item_index);
        }

        // 0x4bcd4c: tumble
        if (((item_object *)obj)->item.flags & 4) {
            real_vector3d *axis = &((item_object *)obj)->item.rotation_axis;
            real sin_angle = ((item_object *)obj)->item.rotation_sine;
            real cos_angle = ((item_object *)obj)->item.rotation_cosine;
            object_marker marker;                                     // [esp+0xd8]
            real_vector3d side;                                       // [esp+0x1c]

            if (network_game_mode == 0 && (((item_object *)obj)->base.flags & 0x20) &&
                (int16_t)object_get_node_local_transform(item_index, s_ground_point_marker, &marker, 1)) {
                // resting: turn the ground point's frame and move the item so the point stays put
                real_matrix4x3 frame = marker.node_transform;         // [esp+0xa0]

                vector3d_rotate_about_axis(&frame.forward, axis, sin_angle, cos_angle);
                vector3d_rotate_about_axis(&frame.up, axis, sin_angle, cos_angle);
                vector3d_cross_product(&frame.left, &frame.forward, &frame.up);
                vector3d_cross_product(&frame.forward, &frame.up, &frame.left);
                vector3d_normalize_with_length(&frame.forward);
                vector3d_normalize_with_length(&frame.left);
                vector3d_normalize_with_length(&frame.up);
                object_recompute_basis_from_marker_delta((object *)obj, &marker, &frame);
            } else {
                vector3d_rotate_about_axis(forward, axis, sin_angle, cos_angle);
                vector3d_rotate_about_axis(up, axis, sin_angle, cos_angle);
            }
            vector3d_normalize_with_length(up);
            vector3d_cross_product(&side, forward, up);
            vector3d_cross_product(forward, up, &side);
            vector3d_normalize_with_length(forward);
        }
    }

    // 0x4bceea: the detonation countdown, and the held time
    if (((item_object *)obj)->item.detonation_countdown > 0) {
        ((item_object *)obj)->item.detonation_countdown -= 1;
        if (((item_object *)obj)->item.detonation_countdown == 0) {
            effect_new_on_object(item_index, *(datum_index *)&((Item *)tag)->detonation_effect.tag_id, item_index, -1, 0.0f, 0.0f, 0, 0);
            object_delete(item_index);
        }
    }
    if (((item_object *)obj)->item.flags & 1) {
        ((item_object *)obj)->item.held_game_time = game_time->game_time;
    }
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
