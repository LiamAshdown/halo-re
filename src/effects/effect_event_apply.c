// effect_event_apply  (Ghidra: effect_event_apply, already named)
// address 0x452cf0, size 1156 bytes
// name confidence: 0.7 (already carries this name; out/phase4/effects_functions.md: "Applies a
//   single effect event/part by tag group, spawning the corresponding object, decal, damage,
//   light, particle system, or sound")   rewrite confidence: 0.85
// evidence: types/tags.h EffectPart (location 0x04, type_class 0x14 -- the fourcc dispatch key,
//   type.tag_id 0x24, radius_modifier_bounds 0x54, angular_velocity_bounds 0x4c); types/objects.h
//   object_placement_data (owner_linkage 0x08=role source, position 0x18, velocity 0x28, forward
//   0x34, up 0x40) and damage_data (all fields, matching src/objects/damage_data_initialize.c and
//   damage_apply_area_effect.c byte for byte: location_leaf_index 0x14, location_cluster_index
//   0x18, epicentre 0x1c, origin 0x28, direction 0x34, random_blend 0x40); types/effects.h effect
//   (location 0x10, velocity 0x24, color 0x18, object_index 0x3c, creator_object_index 0x40);
//   src/objects/object_try_and_get.c (the "visible literal is the type mask, object index is an
//   elided ECX" idiom, confirmed against src/objects/object_delete_by_pooled_node_id.c's own
//   objdump-verified case); src/objects/light_new_positioned.c; src/effects/
//   particle_system_new_at_point.c; src/effects/decal_spawn_for_response.c (whose own,
//   already-established signature is entirely register-passed -- see the UNSURE note on the
//   'deca' branch below, where THIS call site's decompile disagrees with that signature).
// REWRITTEN from objdump 0x452cf0..0x453173 (the draft guessed the sound, decal, object and
//   particle calls; the sound branch passed the tag as the object).
// blam-cc: EAX -> forward, ECX -> position, stack -> self, part, marker, up, scale (object_change_color_evaluate
//   0x452c77 passes one block {up, forward, position}: stack, EAX, ECX)
// Dispatch on the part tag group (+0x14), tag at +0x24:
//   ligh (with 0x0068944c > 0): light_new_positioned(tag, self object +0x3c, marker index, marker
//     position +0x30, scale; EBX marker forward +0x0c). A marker index 0xffff is -1, else & 0x7fff.
//   jpt!: damage_data_initialize(tag); with a creator (self +0x40, object_try_and_get -1) its +0xc0 goes
//     to +0x08, the creator to +0x0c and its +0xb8 word to +0x10; scale +0x40, self location +0x10/+0x14
//     to +0x14/+0x18, position to +0x1c and +0x28, forward to +0x34; damage_apply_area_effect.
//   deca: effect_random_velocity_vector (EAX self, effect seed 0x00719cd4, forward, part +0x40 +0x44
//     +0x48 +0x60 +0x64) gives a direction; radius = random_range_real(+0x54, +0x58);
//     decal_spawn_for_response(ESI tag, BL 0, ECX position, direction, radius, -1).
//   obje: object_placement_data_initialize(tag, creator); position +0x18, forward +0x34, up +0x40;
//     velocity +0x28 from effect_random_velocity_vector (global seed 0x00719cd0) plus the effect velocity
//     (+0x24); angular velocity +0x4c from effect_random_direction_vector (global seed, +0x4c, +0x50);
//     object_new (ECX placement).
//   pctl: color {1, self +0x18..+0x20}; velocity from effect_random_velocity_vector (effect seed) plus
//     the effect velocity; particle_system_new_at_point(tag, position, ECX velocity, EAX color, scale).
//   snd!: attached (self object != -1): the flag is 1 when the creator is a unit (mask 3) whose player
//     (+0x218, datum_get on player_data) is local (+0x02 != -1); sound_start_at_object_marker(ESI object,
//     ECX marker position, EAX marker forward, tag, marker index, scale, flag). Free: a sound_placement
//     {position, forward, *0x00696714, self location +0x10/+0x14} for sound_start_at_location (EDX tag,
//     EAX placement, scale).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "sound.h"
#include "units.h"
#include "game.h"
#include "fn_sound.h"
#include "fn_objects.h"

extern random_seed random_seed_global;               // 0x00719cd0
extern random_seed effect_random_seed;                // 0x00719cd4
extern data_array *player_data;                       // 0x0087a480
extern int16_t light_count_enabled;                   // 0x0068944c, the ligh gate (word, > 0)
extern const real_vector3d *global_origin3d_pointer;  // 0x00696714

extern real random_range_real(real min, real max); // 0x444af0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX handle, ESI array

extern void damage_apply_area_effect(damage_data *dd); // 0x4edd30 (the caller also pushes an unread -1)

extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0

extern void effect_random_velocity_vector(effect *self, random_seed *seed,
    real_vector3d *direction, real_vector3d *out_direction, real_vector3d *out_velocity,
    real min, real max, real angle_max, uint32_t a_bitset, uint8_t b_bitset); // 0x451310, EAX self
extern void effect_random_direction_vector(random_seed *seed, real_point3d *out, real min, real max,
    effect *self, uint32_t a_bitset, uint32_t b_bitset); // 0x451450, stack, EBX self, ESI a, EDI b
extern datum_index particle_system_new_at_point(uint32_t definition_index, real_point3d *position,
    real_vector3d *velocity, ColorARGB *color, float scale); // 0x453600
extern void decal_spawn_for_response(datum_index response_tag_index, uint8_t deterministic, real_point3d *origin,
    real_vector3d *direction, real radius, int32_t marker_index); // 0x44ece0, ESI, BL, ECX, stack
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward,
    datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint); // 0x543ce0, ESI, ECX, EAX, stack


static int32_t effect_event_apply_marker_index(effect_location_marker *marker)
{
    if (marker->marker_index == 0xffff) {
        return -1;
    }
    return marker->marker_index & 0x7fff;
}

#define PART_FIELD(type, offset) (*(type *)((uint8_t *)part + (offset)))
#define SELF_FIELD(type, offset) (*(type *)((uint8_t *)self + (offset)))

void effect_event_apply(effect *self, EffectPart *part, effect_location_marker *marker,
    real_vector3d *up, real_vector3d *forward, real_point3d *position, real scale)
{
    uint32_t group = part->type_class;
    datum_index tag = PART_FIELD(datum_index, 0x24);
    real_point3d *marker_position = (real_point3d *)((uint8_t *)marker + 0x30);
    real_vector3d *marker_forward = (real_vector3d *)((uint8_t *)marker + 0x0c);

    if (group == 0x6c696768u) { // ligh
        if (light_count_enabled > 0) {
            light_new_positioned(tag, (int32_t)SELF_FIELD(datum_index, 0x3c),
                (int16_t)effect_event_apply_marker_index(marker), marker_position, *(uint32_t *)&scale,
                marker_forward);
        }
    } else if (group == 0x6a707421u) { // jpt!
        damage_data dd;
        uint8_t *raw = (uint8_t *)&dd;
        uint8_t *creator = (uint8_t *)object_try_and_get(SELF_FIELD(datum_index, 0x40), 0xffffffff);

        damage_data_initialize(&dd, tag);
        if (creator != 0) {
            *(uint32_t *)(raw + 0x08) = ((struct object *)creator)->owner_linkage;
            *(datum_index *)(raw + 0x0c) = SELF_FIELD(datum_index, 0x40);
            *(int16_t *)(raw + 0x10) = ((struct object *)creator)->owner_team;
        }
        *(real *)(raw + 0x40) = scale;
        *(uint32_t *)(raw + 0x14) = SELF_FIELD(uint32_t, 0x10);
        *(uint32_t *)(raw + 0x18) = SELF_FIELD(uint32_t, 0x14);
        *(real_point3d *)(raw + 0x28) = *position;
        *(real_point3d *)(raw + 0x1c) = *position;
        *(real_vector3d *)(raw + 0x34) = *forward;
        damage_apply_area_effect(&dd);
    } else if (group == 0x64656361u) { // deca
        real_vector3d out_direction;
        real_vector3d direction;
        real radius;

        effect_random_velocity_vector(self, &effect_random_seed, forward, &out_direction, &direction,
            PART_FIELD(real, 0x40), PART_FIELD(real, 0x44), PART_FIELD(real, 0x48), PART_FIELD(uint32_t, 0x60),
            PART_FIELD(uint8_t, 0x64));
        radius = random_range_real(PART_FIELD(real, 0x54), PART_FIELD(real, 0x58));
        decal_spawn_for_response(tag, 0, position, &direction, radius, -1);
    } else if (group == 0x6f626a65u) { // obje
        object_placement_data placement;
        uint8_t *raw = (uint8_t *)&placement;
        real_vector3d out_direction;
        real_vector3d *velocity = (real_vector3d *)(raw + 0x28);

        object_placement_data_initialize(&placement, tag, SELF_FIELD(datum_index, 0x40));
        *(real_point3d *)(raw + 0x18) = *position;
        *(real_vector3d *)(raw + 0x34) = *forward;
        *(real_vector3d *)(raw + 0x40) = *up;
        effect_random_velocity_vector(self, &random_seed_global, forward, &out_direction, velocity,
            PART_FIELD(real, 0x40), PART_FIELD(real, 0x44), PART_FIELD(real, 0x48), PART_FIELD(uint32_t, 0x60),
            PART_FIELD(uint8_t, 0x64));
        velocity->i = velocity->i + SELF_FIELD(real, 0x24);
        velocity->j = velocity->j + SELF_FIELD(real, 0x28);
        velocity->k = velocity->k + SELF_FIELD(real, 0x2c);
        effect_random_direction_vector(&random_seed_global, (real_point3d *)(raw + 0x4c), PART_FIELD(real, 0x4c),
            PART_FIELD(real, 0x50), self, PART_FIELD(uint32_t, 0x60), PART_FIELD(uint32_t, 0x64)); // 0x452f5e: EBX, ESI, EDI
        object_new(&placement);
    } else if (group == 0x7063746cu) { // pctl
        ColorARGB color;
        real_vector3d out_direction;
        real_vector3d velocity;

        color.alpha = 1.0f;
        color.red = SELF_FIELD(real, 0x18);
        color.green = SELF_FIELD(real, 0x1c);
        color.blue = SELF_FIELD(real, 0x20);
        effect_random_velocity_vector(self, &effect_random_seed, forward, &out_direction, &velocity,
            PART_FIELD(real, 0x40), PART_FIELD(real, 0x44), PART_FIELD(real, 0x48), PART_FIELD(uint32_t, 0x60),
            PART_FIELD(uint8_t, 0x64));
        velocity.i = velocity.i + SELF_FIELD(real, 0x24);
        velocity.j = velocity.j + SELF_FIELD(real, 0x28);
        velocity.k = velocity.k + SELF_FIELD(real, 0x2c);
        particle_system_new_at_point(tag, position, &velocity, &color, scale);
    } else if (group == 0x736e6421u) { // snd!
        datum_index object_index = SELF_FIELD(datum_index, 0x3c);

        if (object_index != k_datum_index_none) {
            uint8_t first_person = 0;
            uint8_t *creator = (uint8_t *)object_try_and_get(SELF_FIELD(datum_index, 0x40), 3);

            if (creator != 0) {
                uint8_t *owner = (uint8_t *)datum_get(*(datum_index *)(creator + 0x218), player_data);

                if (owner != 0 && ((struct player *)owner)->local_player_index != -1) {
                    first_person = 1;
                }
            }
            sound_start_at_object_marker(object_index, (Point3D *)marker_position, (Vector3D *)marker_forward, tag,
                (int16_t)effect_event_apply_marker_index(marker), scale, first_person);
        } else {
            sound_placement placement;

            placement.position = *(Point3D *)position;
            placement.forward = *(Vector3D *)forward;
            placement.velocity = *(const Vector3D *)global_origin3d_pointer;
            *(uint32_t *)&placement.leaf_index = SELF_FIELD(uint32_t, 0x10);
            *(uint32_t *)&placement.cluster_index = SELF_FIELD(uint32_t, 0x14);
            sound_start_at_location(tag, &placement, scale);
        }
    }
}

#if 0
Original Ghidra decompilation (0x452cf0):

void effect_event_apply(int param_1,int param_2,int param_3,undefined4 *param_4,undefined4 param_5)

{
  undefined4 *in_EAX;
  uint uVar1;
  int iVar2;
  undefined4 *in_ECX;
  float fVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  float local_b8;
  float local_b4;
  float local_b0;
  uint local_ac;
  undefined4 local_a8;
  undefined4 local_a4;
  undefined4 local_a0;
  undefined4 local_9c;
  undefined4 local_98;
  undefined4 local_94;
  undefined4 local_90;
  undefined4 local_8c;
  undefined4 local_88;
  undefined4 local_84;
  float local_80;
  float local_7c;
  float local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  undefined1 local_5c [64];
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;

  uVar1 = *(uint *)(param_2 + 0x14);
  if (uVar1 < 0x6f626a66) {
    if (uVar1 == 0x6f626a65) {
      object_placement_data_initialize
                (*(undefined4 *)(param_2 + 0x24),*(undefined4 *)(param_1 + 0x40));
      local_90 = *in_ECX;
      local_8c = in_ECX[1];
      local_88 = in_ECX[2];
      local_74 = *in_EAX;
      local_70 = in_EAX[1];
      local_6c = in_EAX[2];
      local_68 = *param_4;
      local_64 = param_4[1];
      local_60 = param_4[2];
      FUN_00451310(&random_seed_global);
      local_80 = local_80 + *(float *)(param_1 + 0x24);
      local_7c = local_7c + *(float *)(param_1 + 0x28);
      local_78 = local_78 + *(float *)(param_1 + 0x2c);
      FUN_00451450(&random_seed_global,local_5c,*(undefined4 *)(param_2 + 0x4c),
                   *(undefined4 *)(param_2 + 0x50));
      object_new();
      return;
    }
    if (uVar1 == 0x64656361) {
      FUN_00451310(&DAT_00719cd4);
      uVar5 = 0;
      uVar4 = 0xffffffff;
      fVar3 = random_range_real(*(float *)(param_2 + 0x54),*(float *)(param_2 + 0x58));
      FUN_0044ece0(&local_b8,fVar3,uVar4,uVar5);
      return;
    }
    if (uVar1 == 0x6a707421) {
      iVar2 = object_try_and_get(0xffffffff);
      damage_data_initialize(*(undefined4 *)(param_2 + 0x24));
      if (iVar2 != 0) {
        local_a0 = *(undefined4 *)(iVar2 + 0xc0);
        local_9c = *(undefined4 *)(param_1 + 0x40);
        local_98 = CONCAT22(local_98._2_2_,*(undefined2 *)(iVar2 + 0xb8));
      }
      local_94 = *(undefined4 *)(param_1 + 0x10);
      local_68 = param_5;
      local_90 = *(undefined4 *)(param_1 + 0x14);
      local_80 = (float)*in_ECX;
      local_7c = (float)in_ECX[1];
      local_78 = (float)in_ECX[2];
      local_8c = *in_ECX;
      local_74 = *in_EAX;
      local_88 = in_ECX[1];
      local_84 = in_ECX[2];
      local_70 = in_EAX[1];
      local_6c = in_EAX[2];
      damage_apply_area_effect(&local_a8,0xffffffff);
      return;
    }
    if ((uVar1 == 0x6c696768) && (0 < DAT_0068944c)) {
      if (*(ushort *)(param_3 + 2) == 0xffff) {
        uVar1 = 0xffffffff;
      }
      else {
        uVar1 = *(ushort *)(param_3 + 2) & 0x7fff;
      }
      light_new_positioned
                (*(undefined4 *)(param_2 + 0x24),*(undefined4 *)(param_1 + 0x3c),uVar1,
                 param_3 + 0x30,param_5);
      return;
    }
  }
  else if (uVar1 == 0x7063746c) {
    local_18 = *(undefined4 *)(param_1 + 0x18);
    local_14 = *(undefined4 *)(param_1 + 0x1c);
    local_10 = *(undefined4 *)(param_1 + 0x20);
    local_1c = 0x3f800000;
    FUN_00451310(&DAT_00719cd4);
    local_b8 = local_b8 + *(float *)(param_1 + 0x24);
    local_b4 = local_b4 + *(float *)(param_1 + 0x28);
    local_b0 = local_b0 + *(float *)(param_1 + 0x2c);
    FUN_00453600(*(undefined4 *)(param_2 + 0x24));
  }
  else if (uVar1 == 0x736e6421) {
    if (*(int *)(param_1 + 0x3c) != -1) {
      local_ac = local_ac & 0xffffff00;
      iVar2 = object_try_and_get(3);
      if (iVar2 != 0) {
        iVar2 = datum_get();
        if ((iVar2 != 0) && (*(short *)(iVar2 + 2) != -1)) {
          local_ac = CONCAT31(local_ac._1_3_,1);
        }
      }
      if (*(ushort *)(param_3 + 2) == 0xffff) {
        uVar1 = 0xffffffff;
      }
      else {
        uVar1 = *(ushort *)(param_3 + 2) & 0x7fff;
      }
      FUN_00543ce0(*(undefined4 *)(param_2 + 0x24),uVar1,param_5,local_ac);
      return;
    }
    local_a8 = *in_ECX;
    local_a4 = in_ECX[1];
    local_a0 = in_ECX[2];
    local_9c = *in_EAX;
    local_98 = in_EAX[1];
    local_94 = in_EAX[2];
    local_90 = *(undefined4 *)PTR_DAT_00696714;
    local_8c = *(undefined4 *)(PTR_DAT_00696714 + 4);
    local_88 = *(undefined4 *)(PTR_DAT_00696714 + 8);
    local_84 = *(undefined4 *)(param_1 + 0x10);
    local_80 = *(float *)(param_1 + 0x14);
    FUN_00543d80(param_5);
    return;
  }
  return;
}
#endif
