// effect_event_apply  (Ghidra: effect_event_apply, already named)
// address 0x452cf0, size 1156 bytes
// name confidence: 0.7 (already carries this name; out/phase4/effects_functions.md: "Applies a
//   single effect event/part by tag group, spawning the corresponding object, decal, damage,
//   light, particle system, or sound")   rewrite confidence: 0.25 (the 'obje', 'deca' and 'snd!'
//   branches reuse one on-stack scratch region across mutually exclusive cases in ways this pass
//   could only partly resolve; see UNSURE notes throughout)
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
// register convention: this function's own 5 parameters (self, part, marker, up, scale) are
//   Ghidra-recognized stack parameters (__cdecl-shaped). Two ADDITIONAL vectors are read through
//   in_EAX/in_ECX. RESOLVED by the phase-4 integration pass, from the 'obje' branch's own
//   writes: with the scratch struct based where the 'jpt!' branch's damage_data is based,
//   in_ECX lands on object_placement_data.position (+0x18), in_EAX on .forward (+0x34) and
//   param_4 on .up (+0x40). The only caller, object_change_color_evaluate 0x4529d0, builds those
//   three as ONE contiguous 9-float stack block (its local_24 / local_18 / local_c) and passes
//   the address of the first, so the register pair is simply the second and third slot of the
//   same block.
//   // blam-cc: EAX -> forward, ECX -> position, stack -> (self, part, marker, up, scale)
// UNSURE (major, 'obje' branch): object_placement_data_initialize and object_new are each called
//   here with only their stack argument(s) visible; their pointer/register arguments (the
//   placement struct itself, and object_new's role) are reconstructed from the writes that
//   immediately follow, which land exactly on object_placement_data's position/velocity/forward/
//   up fields when the placement struct is assumed to start where damage_data starts in the
//   'jpt!' branch (i.e. this function keeps one scratch object_placement_data/damage_data union
//   on its stack, reused per branch). effect_random_velocity_vector and
//   effect_random_direction_vector are each called with only their seed argument visible; every
//   other argument (direction, bounds, bitsets, self, out) is a guess based on which fields nc
//   the result ends up in.
// UNSURE (major, 'deca' branch): FUN_0044ece0 is decal_spawn_for_response, whose own file
//   establishes a fully register-passed signature (ESI/BL/ECX, no stack arguments at all). This
//   call site's decompile instead shows four stack-shaped arguments, which cannot both be true;
//   most likely Ghidra mis-attributed unrelated stack slots (leftover from the immediately
//   preceding, mostly-elided effect_random_velocity_vector call) as this call's arguments. Kept
//   as a best-effort call with a random radius-modifier roll preserved (the one part of this
//   branch that is unambiguous) and the decal call itself heavily flagged.
// UNSURE ('snd!' branch): sound_start_at_object_marker and sound_start_at_location are outside this batch's address range;
//   their signatures are modeled minimally from this call site alone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern random_seed random_seed_global;               // 0x00719cd0
extern random_seed effect_random_seed;                // 0x00719cd4
extern data_array *object_data;                       // 0x008603b0
extern int32_t light_count_enabled;                   // 0x0068944c, UNSURE name: gates the
                                                       // 'ligh' branch, ">0" required
extern const real_point3d *global_origin3d_pointer;   // 0x00696714, math module

extern real random_range_real(real min, real max); // 0x444af0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX/ESI
extern void damage_data_initialize(damage_data *dd, datum_index damage_effect_tag); // 0x4ed990
extern void damage_apply_area_effect(damage_data *dd); // 0x4edd30
extern datum_index light_new_positioned(datum_index light_tag, int32_t marker_index,
    int16_t marker_sub_index, real_point3d *position, uint32_t param_5, real_vector3d *direction); // 0x4f0c10
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0
extern void object_new(void); // 0x4f5460; UNSURE: real (placement, role) args not visible here,
    // same as object_new.c's own note about its call to object_new_with_datum_role_control
extern void effect_random_velocity_vector(effect *self, random_seed *seed,
    real_vector3d *direction, real_vector3d *out_direction, real_vector3d *out_velocity,
    real min, real max, real angle_max, uint32_t a_bitset, uint8_t b_bitset);
    // 0x451310, this module; self in EAX. Every argument except the seed is elided at both call
    // sites below, so only the seed and where the output lands are evidence here.
extern void effect_random_direction_vector(random_seed *seed, real_point3d *out, real min,
    real max); // 0x451450, this module; UNSURE, see file header
extern datum_index particle_system_new_at_point(uint32_t definition_index, real_point3d *position,
    real_vector3d *velocity, ColorARGB *color, float scale); // 0x453600, this module
extern void decal_spawn_for_response(datum_index response_tag_index, uint8_t deterministic,
    uint32_t *seed_words); // 0x44ece0, this module; UNSURE, see file header
extern void sound_start_at_object_marker(datum_index sound_tag, uint32_t marker_index, real scale,
    uint8_t is_valid_unit); // 0x543ce0, outside this batch; UNSURE signature
extern void sound_start_at_location(void *world_placement, real scale); // 0x543d80, outside this batch;
    // UNSURE signature: only `scale` is a visible stack argument, the placement block is passed
    // in a register

// Resolves the marker_index field's "no first-person bit, or 0xffff sentinel" convention shared
// by the light and sound branches.
static uint32_t effect_event_apply_resolve_marker(effect_location_marker *marker)
{
    if (marker->marker_index == 0xffff) {
        return (uint32_t)0xffffffff;
    }
    return (uint32_t)(marker->marker_index & 0x7fff);
}

// Dispatches on the EffectPart's referenced tag group (object/decal/damage/light/particle
// system/sound), spawning the corresponding instance at the resolved marker placement.
void effect_event_apply(effect *self, EffectPart *part, effect_location_marker *marker,
    real_vector3d *up, real_vector3d *forward, real_point3d *position, real scale)
{
    uint32_t group = part->type_class;

    if (group < 0x6f626a66u) {
        if (group == 0x6f626a65u) { // "obje"
            // UNSURE: shared scratch object_placement_data, see file header.
            object_placement_data scratch;

            object_placement_data_initialize(&scratch, part->type.tag_id.index, self->creator_object_index);
            scratch.position = *position;         // in_ECX
            scratch.forward = *forward;            // in_EAX
            scratch.up = *up;                      // param_4

            {
                real_vector3d scratch_direction; // the call's other output, unused here
                effect_random_velocity_vector(self, &random_seed_global, 0, &scratch_direction,
                    &scratch.velocity, 0.0f, 0.0f, 0.0f, 0, 0); // UNSURE, see file header
            }
            scratch.velocity.i += self->velocity.i;
            scratch.velocity.j += self->velocity.j;
            scratch.velocity.k += self->velocity.k;

            effect_random_direction_vector(&random_seed_global, (real_point3d *)&scratch.angular_velocity,
                part->angular_velocity_bounds[0], part->angular_velocity_bounds[1]); // UNSURE

            object_new(); // UNSURE: real (placement=&scratch, role) args not visible here
            return;
        }
        if (group == 0x64656361u) { // "deca"
            real radius_modifier = random_range_real(part->radius_modifier_bounds[0],
                part->radius_modifier_bounds[1]);
            uint32_t seed_words[3] = {0, 0xffffffff, 0}; // UNSURE: see file header
            (void)radius_modifier;
            decal_spawn_for_response(part->type.tag_id.index, 0, seed_words); // UNSURE, see file header
            return;
        }
        if (group == 0x6a707421u) { // "jpt!"
            damage_data dd;
            object *unit = object_try_and_get(self->object_index, _object_mask_all); // UNSURE:
                // object index reconstructed as self->object_index, see file header

            damage_data_initialize(&dd, part->type.tag_id.index);
            if (unit != (object *)0) {
                dd.responsible_player = *(uint32_t *)((uint8_t *)unit + 0xc0); // owner_linkage
                dd.responsible_object = self->creator_object_index;
                dd.team_index = *(int16_t *)((uint8_t *)unit + 0xb8); // name_index
            }
            dd.location_leaf_index = self->location.leaf_index;
            dd.random_blend = scale;
            dd.location_cluster_index = self->location.cluster_index;
            dd.epicentre = *position;   // in_ECX, written twice in the original (see below)
            dd.origin = *position;      // in_ECX again
            dd.direction = *forward;    // in_EAX

            damage_apply_area_effect(&dd);
            return;
        }
        if (group == 0x6c696768u && 0 < light_count_enabled) { // "ligh"
            uint32_t marker_index = effect_event_apply_resolve_marker(marker);
            light_new_positioned(part->type.tag_id.index, self->object_index, (int16_t)marker_index,
                &marker->transform.position, scale, 0); // UNSURE: last argument (direction) not
                    // established at this call site
            return;
        }
    } else if (group == 0x7063746cu) { // "pctl"
        ColorARGB color;
        real_vector3d velocity;

        color.red = self->color.red;
        color.green = self->color.green;
        color.blue = self->color.blue;
        color.alpha = 1.0f;

        {
            real_vector3d scratch_direction; // the call's other output, unused here
            effect_random_velocity_vector(self, &effect_random_seed, 0, &scratch_direction,
                &velocity, 0.0f, 0.0f, 0.0f, 0, 0); // UNSURE, see file header
        }
        velocity.i += self->velocity.i;
        velocity.j += self->velocity.j;
        velocity.k += self->velocity.k;

        particle_system_new_at_point(part->type.tag_id.index, &marker->transform.position,
            &velocity, &color, scale); // UNSURE: position/scale reconstructed, see file header
    } else if (group == 0x736e6421u) { // "snd!"
        if (self->object_index != k_datum_index_none) {
            uint32_t marker_index = effect_event_apply_resolve_marker(marker);
            uint8_t is_valid_unit = 0;
            object *unit = object_try_and_get(self->object_index, _object_mask_unit);

            if (unit != (object *)0) {
                // UNSURE: datum_get's handle/array are not visible at this call site.
                void *record = datum_get(k_datum_index_none, (data_array *)0);
                if (record != (void *)0 && *(int16_t *)((uint8_t *)record + 2) != -1) {
                    is_valid_unit = 1;
                }
            }

            sound_start_at_object_marker(part->type.tag_id.index, marker_index, scale, is_valid_unit);
        } else {
            // A free standing effect: the sound is placed in the world instead of on an object.
            // The original builds a five field block on its own stack (Ghidra local_a8 down to
            // local_80) and passes only the scale visibly, so the block is the elided register
            // argument.
            struct {
                real_point3d position;      // 0x00 in_ECX
                real_vector3d forward;      // 0x0c in_EAX
                real_vector3d velocity;     // 0x18 global_origin3d, i.e. stationary
                int16_t leaf_index;         // 0x24 effect.location
                int16_t cluster_index;      // 0x26
            } sound_placement;

            sound_placement.position = *position;
            sound_placement.forward = *forward;
            sound_placement.velocity = *(const real_vector3d *)global_origin3d_pointer;
            sound_placement.leaf_index = self->location.leaf_index;
            sound_placement.cluster_index = self->location.cluster_index;

            sound_start_at_location(&sound_placement, scale); // UNSURE: the block is register passed, see
                // file header
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
