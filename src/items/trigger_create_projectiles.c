// trigger_create_projectiles  (Ghidra: trigger_create_projectiles, already named)
// address 0x4c4c40, size 2193 bytes
// name confidence: 0.85 (cea-pdb hint, strings "primary trigger"/"secondary trigger")
// rewrite confidence: 0.2 (by far the least complete function in this module: one whole
//   behavioural block is deliberately absent, see below)
// evidence: types/items.h weapon_data/item_data/WeaponTrigger fields used for the clearly
//   identified pieces (magazine/ammo bookkeeping, first_person_offset at WeaponTrigger 0x84,
//   projectile.tag_id at 0xa0, WeaponTriggerFlags bit 5 = projectiles_use_weapon_origin);
//   types/objects.h object_placement_data, object.parent_object.
// The marker walk IS resolved: object_get_node_local_transform fills an array of 0x40
// object_marker records (0x40 * 0x6c == 0x1b00, exactly Ghidra's local_1b00 block), and Ghidra's
// `local_1c10 = local_1c10 + 0x1b` walk reading [0..2] and [-9..-7] is
// markers[i].node_transform.position and .node_transform.forward. See the loop below.
//
// NOT PORTED -- the autoaim / magnetism block. In the #if 0 decompilation this is everything
// from `local_1bdc = (uint *)0x0;` down to the `local_1be4 = FUN_004593b0(...)` assignment. It
// resolves the holder's autoaim target by walking the object data_array by hand (local_1c18's
// index and salt are validated against DAT_008603b0 + 0x20 / +0x22 before the header at +0x34 is
// indexed), follows the target's own 0xca handle when it has one, reads a unit field at 0x218
// and one at 0x1f4, consults actor_data (0x00880360, stride 0x724, the int16 at +0x5f2 against
// 4), and then calls FUN_005658f0 / FUN_0040f7e0 / FUN_004593b0 to produce three outputs this
// function does use: spread_gain (local_1c14), error_bias (local_1c08) and
// projectile_type_index (local_1be4). Porting it needs the units and ai headers, and the three
// callees' argument shapes are not established. All three outputs are therefore initialized to
// their "no target" values below and the projectile aim is the raw marker basis. That is a real
// behavioural gap, not a naming gap: on this code path a weapon with autoaim would fire
// perfectly straight and would not apply its per-target spread. Everything else in the function
// -- the marker walk, the round count, the per-shot placement loop, the spread and
// perpendicular-basis maths, the velocity inheritance from the root parent, and the
// object_new_with_datum_role_control bookkeeping -- is ported.
// UNSURE: FUN_004c54e0 (this module, the barrel spread offset helper) and FUN_004f7b70 argument
// shapes; the WeaponTrigger offsets 0x1b4, 0x6e and 0x26 read through raw casts below.
// register convention: item index, trigger index and the new object's role are all
// Ghidra-recognized parameters.
// blam-cc: stack -> (item_index, trigger_index, role)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed random_seed_global; // 0x00719cd0

// sqrt is a single x87/SSE instruction sequence in the original code (Ghidra's SQRT());
// declared locally instead of via <math.h> because -I types shadows that header name with
// types/math.h.
extern double sqrt(double x);
// These three are read only by the autoaim/magnetism block that this rewrite does NOT port
// (see the file header); they are declared with the names the rest of the repo already uses so
// that whoever finishes that block does not have to re-establish them. types/math.h names the
// first two; src/ai and src/units name the third.
extern real_vector3d *global_up3d_pointer;   // 0x00696720 -> 0x0065c224, (0, 0, 1)
extern real_vector3d *global_left3d_pointer; // 0x0069671c -> 0x0065c218, (0, 1, 0); the fallback
    // basis vector when the up x forward cross product degenerates
extern data_array *actor_data;               // 0x00880360, stride 0x724 (ai module); the
    // unported block reads the int16 at actor + 0x5f2 and compares it against 4

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern int16_t object_get_node_local_transform(datum_index object_index, char *marker_name,
    void *out_transforms, int32_t max_count); // 0x4f6080
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern int32_t FUN_0040f7e0(real_vector3d *v); // 0x40f7e0, outside this module, UNSURE signature
extern int32_t FUN_004593b0(real_point3d *origin, real_vector3d *forward); // 0x4593b0, outside this module, UNSURE signature
extern void FUN_005658f0(datum_index target_index, real *out_gain, uint32_t flags1, uint32_t flags2); // 0x5658f0, outside this module, UNSURE signature
extern real_vector3d *vector3d_randomize_direction(real angle, real_vector3d *out); // 0x4cd1b0, UNSURE argument order at this call site
extern void *vector3d_build_perpendicular(void); // 0x4cd670, UNSURE: this call site shows no visible arguments
extern void weapon_trigger_barrel_spread_offset(real_vector3d *v, real_vector3d *axis, uint16_t barrel_index,
    int16_t distribution_function, real distribution_angle, uint32_t flags); // 0x4c54e0
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
    datum_index role); // 0x4f53a0
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern void unit_get_camera_position(datum_index unit_index, real_point3d *out); // 0x568f80
extern void FUN_004f7b70(datum_index new_object_index, datum_index camera_unit_index); // 0x4f7b70, outside this module, UNSURE signature

// Computes the firing origin/spread for a weapon's trigger and spawns the resulting projectile
// object(s), one per marker matching the trigger's tag-defined attachment marker. See the file
// header for the substantial UNSURE caveats on the autoaim/marker-transform machinery.
void trigger_create_projectiles(datum_index item_index, int16_t trigger_index, uint32_t role)
{
    object *item_obj;
    weapon_data *wd;
    item_data *id;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;
    weapon_trigger_state *trigger;
    datum_index holder_index;
    datum_index effective_item_index;
    char *marker_names[2];
    // The output buffer is 0x40 object_marker records: 0x40 * 0x6c is exactly 0x1b00, which is
    // exactly the span of Ghidra's local_1b00 frame block, and Ghidra's own marker walk
    // (`local_1c10 = local_1c10 + 0x1b`, i.e. +0x6c bytes, reading [0..2] and [-9..-7]) lands on
    // object_marker.node_transform.position (marker + 0x60) and .forward (marker + 0x3c) for
    // every record. Ghidra's local_1aa0 is simply local_1b00 + 0x60, i.e. markers[0]'s position,
    // which is why its indices go negative.
    object_marker markers[0x40];
    int16_t marker_count;
    int16_t marker_index;
    object_marker *marker;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)item_obj + k_item_data_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
    trigger = &wd->triggers[trigger_index];

    holder_index = (datum_index)0xffffffff;
    if (item_obj->parent_object != (datum_index)0xffffffff &&
        object_try_and_get(item_obj->parent_object, _object_mask_unit) != 0) {
        holder_index = item_obj->parent_object;
    }

    marker_names[0] = "primary trigger";
    marker_names[1] = "secondary trigger";

    // If the item itself has no collision and is attached, markers are read off the holder.
    effective_item_index = item_index;
    if ((item_obj->flags & _object_no_collision_bit) != 0 && item_obj->parent_object != (datum_index)0xffffffff) {
        effective_item_index = item_obj->parent_object;
    }

    marker_count = object_get_node_local_transform(effective_item_index, marker_names[trigger_index],
        markers, 0x40);
    if (marker_count == 0) {
        marker_count = 1;
    }
    if ((tag_trigger->flags & 0x20) == 0) { // !projectiles_use_weapon_origin
        marker_count = 1;
    }

    // The original guards the marker walk with `if (0 < (short)uVar6)` and counts down a
    // separate copy, so a negative count runs the body zero times. A `(uint16_t)` widening here
    // would instead run it 0xffff times.
    for (marker_index = 0; marker_index < marker_count; marker_index++) {
        real_point3d origin;
        real_vector3d forward, up, left;
        datum_index autoaim_target; // UNSURE: local_1bdc's real type (object* vs a wider record)
        int32_t autoaim_available;
        real spread_gain; // local_1c14
        real error_bias;  // local_1c08
        int32_t projectile_type_index; // local_1be4
        int16_t projectile_tag_index; // UNSURE
        int16_t round_count; // local_1c04
        datum_index projectile_tag; // uVar17 in the two branches
        datum_index attachment_role;
        int16_t shot;

        marker = &markers[marker_index];
        origin = marker->node_transform.position;
        forward = marker->node_transform.forward;

        autoaim_target = (datum_index)0xffffffff; // UNSURE: local_1bdc resolution (autoaim /
            // magnetism target lookup through holder_index) is not ported; every downstream use
            // of it below is therefore forced down its "no target" path.
        autoaim_available = 0;
        spread_gain = 0.0f;
        error_bias = 0.0f;
        projectile_type_index = -1;
        (void)autoaim_available;

        if ((tag_trigger->flags & 0x20) != 0) {
            origin = marker->node_transform.position;
        }

        if (trigger_index == 0 && wd->alternate_shots_loaded > 0) {
            round_count = (int16_t)(*(int16_t *)((uint8_t *)tag_trigger + 0x6e) *
                (weapon_tag->secondary_trigger_mode == 4 ? wd->alternate_shots_loaded + 1 : wd->alternate_shots_loaded));
            wd->alternate_shots_loaded = 0;
            projectile_tag = *(datum_index *)((uint8_t *)weapon_tag->triggers.pointer + 0x1b4); // UNSURE offset
        } else {
            projectile_tag = *(datum_index *)&tag_trigger->projectile.tag_id;
            round_count = *(int16_t *)((uint8_t *)tag_trigger + 0x6e);
        }

        if (projectile_tag != (datum_index)0xffffffff) {
            attachment_role = (datum_index)0xffffffff;
            if (item_obj->parent_object != (datum_index)0xffffffff) {
                object *parent = object_try_and_get(item_obj->parent_object, _object_mask_unit);
                if (parent != 0) {
                    attachment_role = item_obj->parent_object;
                    if (parent->unknown_0c4 != (uint32_t)0xffffffff) { // UNSURE: object 0x328 vs 0x0c4
                        attachment_role = (datum_index)parent->unknown_0c4;
                    }
                }
            }

            for (shot = 0; shot < round_count; shot++) {
                object_placement_data placement;
                real spread[3];
                real spread_axis[3];
                real spawn_velocity[3];
                int32_t report_projectile_flags;
                int32_t is_first_shot_this_tick;
                int32_t hit_something;
                datum_index new_index;

                is_first_shot_this_tick = 0;
                object_placement_data_initialize(&placement, projectile_tag, attachment_role);
                spread[0] = origin.x; spread_axis[0] = forward.i;
                spread[1] = origin.z; spread_axis[1] = up.i; // UNSURE: mirrors local_1b78/local_1b60
                spread[2] = origin.y; spread_axis[2] = forward.k;

                if (trigger->firing_effect_rounds != 0 ||
                    (*(int16_t *)((uint8_t *)tag_trigger + 0x26) <= *(int16_t *)((uint8_t *)&item_obj[0] + 0xe))) {
                    is_first_shot_this_tick = 1;
                }

                if (error_bias == 0.0f) {
                    real fraction = ((tag_trigger->flags & 0x200) == 0) ? *(real *)((uint8_t *)&item_obj[0] + 0x1c) : wd->primary_trigger; // UNSURE offsets
                    error_bias = fraction * *(real *)((uint8_t *)tag_trigger + 0x80) +
                                 (1.0f - fraction) * *(real *)((uint8_t *)tag_trigger + 0x7c); // UNSURE: error_angle[0..1]
                }

                if ((tag_trigger->flags & 0x400) == 0 || (wd->control_flags & 0x40) == 0) {
                    vector3d_randomize_direction(error_bias, &forward); // UNSURE argument order
                }

                {
                    real *perp = (real *)vector3d_build_perpendicular(); // UNSURE: no visible args
                    real length = (real)sqrt((double)(perp[2] * perp[2] + perp[1] * perp[1] + perp[0] * perp[0]));
                    if (length >= 0.0001f) {
                        real inv = 1.0f / length;
                        perp[0] *= inv; perp[1] *= inv; perp[2] *= inv;
                    }
                    weapon_trigger_barrel_spread_offset((real_vector3d *)&forward, (real_vector3d *)perp,
                        (uint16_t)shot, tag_trigger->distribution_function, tag_trigger->distribution_angle,
                        (uint32_t)round_count);
                }

                if (holder_index == (datum_index)0xffffffff ||
                    (((Unit *)tag_instances[(uint16_t)holder_index].data)->unit_flags & 0x10) == 0) { // UNSURE: object 0x17c bit 4
                    spawn_velocity[0] = forward.i * spread_gain;
                    spawn_velocity[1] = forward.j * spread_gain;
                    spawn_velocity[2] = forward.k * spread_gain;
                } else {
                    object *root = item_obj;
                    while (root->parent_object != (datum_index)0xffffffff) {
                        root = ((object_header *)object_data->data)[(uint16_t)root->parent_object].data;
                    }
                    spawn_velocity[0] = root->velocity.i;
                    spawn_velocity[1] = root->velocity.j;
                    spawn_velocity[2] = root->velocity.k;
                }

                hit_something = (autoaim_target != (datum_index)0xffffffff); // UNSURE placeholder,
                    // real predicate is `local_1bdc == 0 || local_1bdc[0x86] == -1`
                report_projectile_flags = hit_something ? 2 : 0;

                new_index = object_new_with_datum_role_control(&placement, role);
                if (new_index != (datum_index)0xffffffff) {
                    if (report_projectile_flags & 2) {
                        real_point3d camera_position;
                        unit_get_camera_position(holder_index, &camera_position);
                        FUN_004f7b70(new_index, holder_index);
                    }
                    {
                        object *new_obj = ((object_header *)object_data->data)[(uint16_t)new_index].data;
                        if (projectile_type_index != -1) {
                            *(int32_t *)((uint8_t *)new_obj + 0x238) = projectile_type_index;
                        }
                        if (!is_first_shot_this_tick) {
                            new_obj->flags = new_obj->flags & ~(uint32_t)2;
                        }
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4c4c40):

void trigger_create_projectiles(uint param_1,short param_2,undefined4 param_3)

{
  uint *puVar1;
  float fVar2;
  uint uVar3;
  bool bVar4;
  bool bVar5;
  ushort uVar6;
  int iVar7;
  short *psVar8;
  float *pfVar9;
  uint uVar10;
  short sVar11;
  short sVar12;
  int iVar13;
  uint *puVar14;
  int iVar15;
  short *psVar16;
  uint uVar17;
  float10 fVar18;
  float local_1c24;
  float local_1c20;
  float local_1c1c;
  uint local_1c18;
  float local_1c14;
  float *local_1c10;
  uint *local_1c0c;
  float local_1c08;
  uint local_1c04;
  float local_1c00;
  float local_1bfc;
  float local_1bf8;
  uint local_1bf4;
  int local_1bf0;
  int local_1bec;
  float local_1be8;
  int local_1be4;
  float local_1be0;
  uint *local_1bdc;
  uint local_1bd8;
  float local_1bd4;
  float local_1bd0;
  float local_1bcc;
  char *local_1bc8 [2];
  uint local_1bc0;
  float local_1bbc;
  float local_1bb8;
  float local_1bb4;
  int local_1bb0;
  int local_1bac;
  uint *local_1ba8;
  float local_1ba4;
  float local_1ba0;
  float local_1b9c;
  undefined1 local_1b98 [4];
  uint local_1b94;
  float local_1b80;
  float local_1b7c;
  float local_1b78;
  float local_1b70;
  float local_1b6c;
  float local_1b68;
  float local_1b64;
  float local_1b60;
  float local_1b5c;
  undefined1 local_1b58 [88];
  undefined1 local_1b00 [96];
  float local_1aa0 [1703];
  undefined4 uStack_4;

  uStack_4 = 0x4c4c4a;
  iVar15 = (int)param_2;
  iVar13 = (param_1 & 0xffff) * 0xc;
  local_1c0c = *(uint **)(iVar13 + 8 + *(int *)(DAT_008603b0 + 0x34));
  local_1ba8 = local_1c0c + iVar15 * 10 + 0x98;
  local_1bac = *(int *)((*local_1c0c & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar17 = local_1c0c[0x47];
  puVar14 = (uint *)(iVar15 * 0x114 + *(int *)(local_1bac + 0x500));
  local_1bb0 = iVar13;
  local_1c18 = 0xffffffff;
  if ((uVar17 != 0xffffffff) && (iVar7 = object_try_and_get(3), iVar7 != 0)) {
    local_1c18 = uVar17;
  }
  iVar13 = *(int *)(iVar13 + 8 + *(int *)(DAT_008603b0 + 0x34));
  local_1bc8[0] = "primary trigger";
  local_1bc8[1] = "secondary trigger";
  if (((*(byte *)(iVar13 + 0x10) & 1) != 0) &&
     (uVar17 = *(uint *)(iVar13 + 0x11c), uVar17 != 0xffffffff)) {
    param_1 = uVar17;
  }
  uVar6 = object_get_node_local_transform(param_1,local_1bc8[iVar15],local_1b00,0x40);
  if (uVar6 == 0) {
    uVar6 = 1;
  }
  if ((*puVar14 & 0x20) == 0) {
    uVar6 = 1;
  }
  if (0 < (short)uVar6) {
    local_1bd8 = (uint)uVar6;
    local_1c10 = local_1aa0;
    do {
      local_1c24 = *local_1c10;
      local_1c20 = local_1c10[1];
      local_1c1c = local_1c10[2];
      local_1c00 = local_1c10[-9];
      local_1bfc = local_1c10[-8];
      local_1bf8 = local_1c10[-7];
      psVar16 = (short *)0x0;
      local_1c14 = 0.0;
      local_1c08 = 0.0;
      if (((local_1c18 != 0xffffffff) && (sVar11 = (short)local_1c18, -1 < sVar11)) &&
         (sVar11 < *(short *)(DAT_008603b0 + 0x20))) {
        psVar8 = (short *)((int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar11 +
                          *(int *)(DAT_008603b0 + 0x34));
        sVar11 = *psVar8;
        if ((sVar11 != 0) &&
           ((sVar12 = (short)(local_1c18 >> 0x10), sVar12 == 0 || (sVar11 == sVar12)))) {
          psVar16 = psVar8;
        }
      }
      local_1bdc = (uint *)0x0;
      if ((psVar16 != (short *)0x0) && ((1 << (*(byte *)((int)psVar16 + 3) & 0x1f) & 3U) != 0)) {
        local_1bdc = *(uint **)(psVar16 + 4);
      }
      local_1be4 = -1;
      if ((((*puVar14 & 0x800) == 0) && (local_1bdc != (uint *)0x0)) &&
         ((*(byte *)((int)local_1bdc + 0x106) & 4) == 0)) {
        uVar17 = local_1bdc[0x86];
        uVar10 = local_1bdc[0x7d];
        uVar3 = local_1bdc[0xca];
        if (uVar3 != 0xffffffff) {
          iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
          uVar17 = *(uint *)(iVar13 + 0x218);
          uVar10 = *(uint *)(iVar13 + 500);
        }
        local_1bc0 = CONCAT31(local_1bc0._1_3_,
                              (char)(*(uint *)(*(int *)((*local_1bdc & 0xffff) * 0x20 + 0x14 +
                                                       DAT_0087bc14) + 0x17c) >> 3)) & 0xffffff01;
        local_1bf4 = CONCAT31(local_1bf4._1_3_,1);
        if ((uVar10 != 0xffffffff) &&
           (*(short *)((uVar10 & 0xffff) * 0x724 + 0x5f2 + *(int *)(DAT_00880360 + 0x34)) == 4)) {
          local_1bf4 = (uint)local_1bf4._1_3_ << 8;
        }
        if (uVar3 != 0xffffffff) {
          local_1bf4 = local_1bf4 & 0xffffff00;
        }
        FUN_005658f0(local_1c18,&local_1c14,local_1bc0,local_1bf4);
        if (uVar17 == 0xffffffff) {
          if (uVar10 != 0xffffffff) {
            local_1be4 = FUN_0040f7e0(&local_1c08);
          }
        }
        else {
          vector3d_cross_product(PTR_DAT_00696720);
          fVar18 = (float10)vector3d_normalize_with_length();
          if ((float10)0.0 == fVar18) {
            local_1bd4 = *(float *)PTR_DAT_0069671c;
            local_1bd0 = *(float *)(PTR_DAT_0069671c + 4);
            local_1bcc = *(float *)(PTR_DAT_0069671c + 8);
          }
          vector3d_cross_product(&local_1c00);
          vector3d_normalize_with_length();
          fVar2 = (float)puVar14[0x21];
          local_1be8 = (float)puVar14[0x22];
          local_1be0 = (float)puVar14[0x23];
          local_1c24 = local_1ba4 * local_1be0 +
                       local_1bd4 * local_1be8 + local_1c00 * fVar2 + local_1c24;
          local_1c20 = local_1ba0 * local_1be0 +
                       local_1bd0 * local_1be8 + local_1bfc * fVar2 + local_1c20;
          local_1c1c = local_1b9c * local_1be0 +
                       local_1bcc * local_1be8 + local_1bf8 * fVar2 + local_1c1c;
          local_1be4 = FUN_004593b0(&local_1c24,&local_1c00);
        }
      }
      if ((*puVar14 & 0x20) != 0) {
        local_1c24 = *local_1c10;
        local_1c20 = local_1c10[1];
        local_1c1c = local_1c10[2];
      }
      if ((param_2 == 0) && (sVar11 = (short)local_1c0c[0x97], 0 < sVar11)) {
        uVar17 = *(uint *)(*(int *)(local_1bac + 0x500) + 0x1b4);
        if (*(short *)(local_1bac + 0x32c) == 4) {
          sVar11 = sVar11 + 1;
        }
        local_1c04 = (uint)(ushort)(*(short *)((int)puVar14 + 0x6e) * sVar11);
        *(undefined2 *)(local_1c0c + 0x97) = 0;
      }
      else {
        uVar17 = puVar14[0x28];
        local_1c04 = CONCAT22(local_1c04._2_2_,*(undefined2 *)((int)puVar14 + 0x6e));
      }
      if (uVar17 != 0xffffffff) {
        iVar13 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_1bb0) + 0x11c);
        local_1bec = -1;
        if (((iVar13 != -1) && (iVar15 = object_try_and_get(3), iVar15 != 0)) &&
           (local_1bec = iVar13, *(int *)(iVar15 + 0x328) != -1)) {
          local_1bec = *(int *)(iVar15 + 0x328);
        }
        local_1bf0 = 0;
        if (0 < (short)local_1c04) {
          local_1bc8[0] = (char *)((uVar17 & 0xffff) * 0x20 + 0x14);
          do {
            bVar5 = false;
            object_placement_data_initialize(puVar14[0x28],local_1bec);
            local_1b80 = local_1c24;
            local_1b64 = local_1c00;
            local_1b78 = local_1c1c;
            local_1b5c = local_1bf8;
            local_1b7c = local_1c20;
            local_1b60 = local_1bfc;
            if (((float)local_1ba8[4] == 0.0) ||
               (sVar11 = *(short *)((int)puVar14 + 0x26), sVar12 = *(short *)((int)local_1ba8 + 0xe)
               , *(short *)((int)local_1ba8 + 0xe) = sVar12 + 1, sVar11 <= sVar12)) {
              bVar5 = true;
              *(undefined2 *)((int)local_1ba8 + 0xe) = 0;
            }
            if (local_1c08 == 0.0) {
              if ((*puVar14 & 0x200) == 0) {
                fVar2 = (float)local_1ba8[7];
              }
              else {
                fVar2 = (float)local_1c0c[0x8d];
              }
              local_1c08 = fVar2 * (float)puVar14[0x20] + (1.0 - fVar2) * (float)puVar14[0x1f];
            }
            if (((*puVar14 & 0x400) == 0) || ((local_1c0c[0x8c] & 0x40) == 0)) {
              vector3d_randomize_direction(puVar14[0x1e],local_1c08);
            }
            if ((short)local_1bf0 == 0) {
              local_1bbc = local_1b64;
              local_1bb8 = local_1b60;
              local_1bb4 = local_1b5c;
            }
            if ((*puVar14 & 0x1000) != 0) {
              local_1b64 = local_1bbc;
              local_1b60 = local_1bb8;
              local_1b5c = local_1bb4;
            }
            pfVar9 = (float *)vector3d_build_perpendicular();
            fVar2 = SQRT(pfVar9[2] * pfVar9[2] + pfVar9[1] * pfVar9[1] + *pfVar9 * *pfVar9);
            if (0.0001 <= ABS(fVar2)) {
              fVar2 = 1.0 / fVar2;
              *pfVar9 = fVar2 * *pfVar9;
              pfVar9[1] = fVar2 * pfVar9[1];
              pfVar9[2] = fVar2 * pfVar9[2];
            }
            FUN_004c54e0(&local_1b64,local_1b58,(short)puVar14[0x1b],puVar14[0x1c],local_1c04);
            if ((*(int *)(local_1bc8[0] + DAT_0087bc14) == 0) ||
               ((*(byte *)(*(int *)(local_1bc8[0] + DAT_0087bc14) + 0x17c) & 0x10) == 0)) {
              local_1b70 = local_1b64 * local_1c14;
              local_1b6c = local_1b60 * local_1c14;
              local_1b68 = local_1b5c * local_1c14;
            }
            else {
              iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_1c18 & 0xffff) * 0xc);
              uVar17 = *(uint *)(iVar13 + 0x11c);
              while (uVar17 != 0xffffffff) {
                iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar17 & 0xffff) * 0xc);
                uVar17 = *(uint *)(iVar13 + 0x11c);
              }
              local_1b70 = *(float *)(iVar13 + 0x68);
              local_1b6c = *(float *)(iVar13 + 0x6c);
              local_1b68 = *(float *)(iVar13 + 0x70);
            }
            if ((local_1bdc == (uint *)0x0) || (local_1bdc[0x86] == 0xffffffff)) {
              bVar4 = false;
            }
            else {
              local_1b94 = local_1b94 | 2;
              bVar4 = true;
            }
            uVar10 = object_new_with_datum_role_control(local_1b98,param_3);
            uVar17 = local_1c18;
            if (uVar10 != 0xffffffff) {
              if (bVar4) {
                unit_get_camera_position();
                FUN_004f7b70(uVar10,uVar17);
              }
              iVar13 = DAT_008603b0;
              if (local_1be4 != -1) {
                *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar10 & 0xffff) * 0xc) +
                        0x238) = local_1be4;
              }
              if (!bVar5) {
                puVar1 = (uint *)(*(int *)(*(int *)(iVar13 + 0x34) + 8 + (uVar10 & 0xffff) * 0xc) +
                                 0x22c);
                *puVar1 = *puVar1 & 0xfffffffd;
              }
            }
            local_1bf0 = local_1bf0 + 1;
          } while ((short)local_1bf0 < (short)local_1c04);
        }
      }
      local_1c10 = local_1c10 + 0x1b;
      local_1bd8 = local_1bd8 - 1;
    } while (local_1bd8 != 0);
  }
  return;
}
#endif
