// trigger_create_projectiles  (Ghidra: trigger_create_projectiles, already named)
// address 0x4c4c40, size 2193 bytes
// name confidence: 0.85 (cea-pdb hint, strings "primary trigger"/"secondary trigger")
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x4c4c40..0x4c54c9 (the draft left out the whole autoaim block and mis-called the
//   reposition, spread, randomise and perpendicular helpers). Stack: (weapon, trigger, role). For each trigger
//   marker (all of them with trigger flag 0x20, else one): start at the marker; a live unit holder projects the
//   shot onto its aiming axis (0x5658f0, giving the inherited speed), a player adds the trigger's first-person
//   offset (+0x84..+0x8c) and resolves its autoaim target (0x4593b0), an actor its aim direction and error
//   (0x40f7e0). Then per projectile (+0x6e, or the charged count with trigger 1's projectile): placement at the
//   origin along the aim, every n-th a tracer (+0x26), randomised by the error (min +0x7c / max +0x80 by the
//   weapon's error / heat), the barrel spread, inherited velocity (the root parent's for projectile flag 0x10),
//   created (0x4f54b0); a player's shot is swept back from the camera (0x4f7b70) and all get the autoaim target.
// blam-cc: stack -> (item_index, trigger_index, role)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "fn_ai.h"
#include "fn_game.h"
#include "fn_math.h"
#include "fn_objects.h"
#include "fn_items.h"

extern data_array *object_data;        // 0x008603b0
extern data_array *actor_data;         // 0x00880360
extern tag_instance *tag_instances;    // 0x0087bc14
extern random_seed random_seed_global; // 0x00719cd0
extern real_vector3d *global_up3d_pointer;   // 0x00696720
extern real_vector3d *global_left3d_pointer; // 0x0069671c
extern char s_primary_trigger_marker[];   // 0x006600a0 "primary trigger"
extern char s_secondary_trigger_marker[]; // 0x0066b1bc "secondary trigger"

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *markers,
    uint32_t max_count); // 0x4f6080, returns the marker count
extern void unit_project_onto_aiming_axis(datum_index unit_index, real *out_speed, uint8_t project_point,
    uint8_t use_unit_aiming_vector, real_point3d *point, real_vector3d *axis); // 0x5658f0, stack, EAX, EBX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0


extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
    datum_index role); // 0x4f53a0, EAX, stack
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out, random_seed *seed,
    real lo, real hi); // 0x4cd1b0, EAX, EBX, EDI, stack


extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out); // 0x568f80, ECX, EDI


extern double fabs(double x);
extern double sqrt(double x);

#define F(p, o) (*(float *)((p) + (o)))
#define W(p, o) (*(int16_t *)((p) + (o)))
#define D(p, o) (*(datum_index *)((p) + (o)))
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

// blam-cc: stack -> (item_index, trigger_index, role)
void trigger_create_projectiles(uint32_t item_index, int16_t trigger_index, uint32_t role)
{
    uint8_t *item = OBJECT_DATA(item_index);                          // [esp+0x2c]
    uint8_t *weapon_tag = TAG_DATA(*(datum_index *)item);             // [esp+0x8c]
    uint8_t *trigger = *(uint8_t **)(weapon_tag + 0x500) + trigger_index * 0x114; // ebp, the WeaponTrigger
    uint8_t *state = item + 0x260 + trigger_index * 0x28;             // [esp+0x90]
    datum_index holder = k_datum_index_none;                          // [esp+0x20]
    uint32_t marker_object = item_index;
    static object_marker markers[0x40];                               // [esp+0x138]
    int16_t marker_count;
    int16_t m;

    if (D(item, 0x11c) != k_datum_index_none && object_try_and_get(D(item, 0x11c), 3) != 0) {
        holder = D(item, 0x11c);
    }
    if ((((struct item_object *)item)->base.flags & 1) && D(item, 0x11c) != k_datum_index_none) {
        marker_object = D(item, 0x11c);
    }
    marker_count = (int16_t)object_get_node_local_transform(marker_object,
        trigger_index == 0 ? s_primary_trigger_marker : s_secondary_trigger_marker, markers, 0x40);
    if (marker_count == 0) {
        marker_count = 1;
    }
    if (!(*(uint32_t *)trigger & 0x20)) {
        marker_count = 1;
    }

    for (m = 0; m < marker_count; m++) {
        real_point3d origin = markers[m].node_transform.position;      // [esp+0x14]
        real_vector3d forward = markers[m].node_transform.forward;     // [esp+0x38]
        real speed = 0.0f;                                             // [esp+0x24]
        float error = 0.0f;                                            // [esp+0x30]
        uint8_t *holder_object = 0;                                    // [esp+0x5c]
        datum_index target = k_datum_index_none;                       // [esp+0x54]
        datum_index projectile_tag;
        int16_t count;                                                 // [esp+0x34]
        datum_index owner = k_datum_index_none;                        // [esp+0x4c]
        int16_t shot;

        // the holder, if it is a live unit
        if (holder != k_datum_index_none) {
            int16_t index = (int16_t)holder;
            int16_t salt = (int16_t)(holder >> 16);

            if (index >= 0 && index < *(int16_t *)((uint8_t *)object_data + 0x20)) {
                uint8_t *header = (uint8_t *)object_data->data + *(int16_t *)((uint8_t *)object_data + 0x22) * index;

                if (*(int16_t *)header != 0 && (salt == 0 || *(int16_t *)header == salt) &&
                    ((1u << (header[3] & 0x1f)) & 3)) {
                    holder_object = *(uint8_t **)(header + 0x8);
                }
            }
        }

        // 0x4c4e05: autoaim for a player, aim direction for an actor
        if (!(*(uint32_t *)trigger & 0x800) && holder_object != 0 && !(holder_object[0x106] & 4)) {
            uint8_t *holder_tag = TAG_DATA(*(datum_index *)holder_object);
            datum_index player = D(holder_object, 0x218);
            datum_index actor = D(holder_object, 0x1f4);
            uint8_t use_aiming_vector;
            uint8_t project_point = 1;

            if (D(holder_object, 0x328) != k_datum_index_none) {
                uint8_t *controller = OBJECT_DATA(D(holder_object, 0x328));

                player = D(controller, 0x218);
                actor = D(controller, 0x1f4);
            }
            use_aiming_vector = (uint8_t)((*(uint32_t *)(holder_tag + 0x17c) >> 3) & 1);
            if (actor != k_datum_index_none &&
                W((uint8_t *)actor_data->data + (actor & 0xffff) * 0x724, 0x5f2) == 4) {
                project_point = 0;
            }
            if (D(holder_object, 0x328) != k_datum_index_none) {
                project_point = 0;
            }
            unit_project_onto_aiming_axis(holder, &speed, use_aiming_vector, project_point, &origin, &forward);
            if (player != k_datum_index_none) {
                real_vector3d left;     // [esp+0x64]
                real_vector3d up;       // [esp+0x94]
                real x = F(trigger, 0x84);
                real y = F(trigger, 0x88);
                real z = F(trigger, 0x8c);

                vector3d_cross_product(&left, &forward, global_up3d_pointer);
                if (vector3d_normalize_with_length(&left) == 0.0f) {
                    left = *global_left3d_pointer;
                }
                vector3d_cross_product(&up, &left, &forward);
                vector3d_normalize_with_length(&up);
                // the first-person offset: forward x, left y, up z
                origin.x = origin.x + forward.i * x + left.i * y + up.i * z;
                origin.y = origin.y + forward.j * x + left.j * y + up.j * z;
                origin.z = origin.z + forward.k * x + left.k * y + up.k * z;
                target = camera_observer_update(player, &origin, &forward);
            } else if (actor != k_datum_index_none) {
                target = actor_compute_grenade_aim_direction(actor, &origin, &forward, &error);
            }
        }
        if (*(uint32_t *)trigger & 0x20) {
            origin = markers[m].node_transform.position;
        }

        // 0x4c5052: how many projectiles, of which kind
        if (trigger_index == 0 && W(item, 0x25c) > 0) {
            int16_t charge = W(item, 0x25c);

            if (W(weapon_tag, 0x32c) == 4) {
                charge++;
            }
            projectile_tag = D(*(uint8_t **)(weapon_tag + 0x500), 0x1b4);
            count = (int16_t)((uint16_t)W(trigger, 0x6e) * charge);
            W(item, 0x25c) = 0;
        } else {
            count = W(trigger, 0x6e);
            projectile_tag = D(trigger, 0xa0);
        }
        if (projectile_tag == k_datum_index_none) {
            continue;
        }
        {
            datum_index parent = D(OBJECT_DATA(item_index), 0x11c);
            object *parent_object = parent != k_datum_index_none ? object_try_and_get(parent, 3) : 0;

            if (parent_object != 0) {
                owner = parent;
                if (D((uint8_t *)parent_object, 0x328) != k_datum_index_none) {
                    owner = D((uint8_t *)parent_object, 0x328);
                }
            }
        }

        for (shot = 0; shot < count; shot++) {
            object_placement_data placement;       // [esp+0xa0]
            uint8_t tracer = 0;                     // [esp+0x13]
            uint8_t from_player;
            datum_index projectile;
            uint8_t *projectile_definition;

            object_placement_data_initialize(&placement, D(trigger, 0xa0), owner);
            placement.position = origin;
            placement.forward = forward;
            // every n-th round (trigger +0x26) is a tracer
            if (F(state, 0x10) == 0.0f) {
                tracer = 1;
                W(state, 0xe) = 0;
            } else {
                int16_t n = W(state, 0xe);

                W(state, 0xe) = n + 1;
                if (!(n < W(trigger, 0x26))) {
                    tracer = 1;
                    W(state, 0xe) = 0;
                }
            }
            if (error == 0.0f) {
                real e = (*(uint32_t *)trigger & 0x200) ? F(item, 0x234) : F(state, 0x1c);

                error = (1.0f - e) * F(trigger, 0x7c) + e * F(trigger, 0x80);
            }
            if (!(*(uint32_t *)trigger & 0x400) || !(item[0x230] & 0x40)) {
                vector3d_randomize_direction((real_point3d *)&placement.forward, &placement.forward, &random_seed_global,
                                             F(trigger, 0x78), error);
            }
            {
                static real_vector3d first_direction;   // [esp+0x7c]

                if (shot == 0) {
                    first_direction = placement.forward;
                }
                if (*(uint32_t *)trigger & 0x1000) {
                    placement.forward = first_direction;
                }
            }
            vector3d_build_perpendicular(&placement.up, &placement.forward);
            {
                real length = (real)sqrt(placement.up.i * placement.up.i + placement.up.j * placement.up.j +
                                         placement.up.k * placement.up.k);

                if (!(fabs(length) < 9.999999747378752e-05)) {
                    real inverse = 1.0f / length;

                    placement.up.i *= inverse;
                    placement.up.j *= inverse;
                    placement.up.k *= inverse;
                }
            }
            weapon_trigger_barrel_spread_offset(&placement.forward, &placement.up, (uint16_t)shot, W(trigger, 0x6c),
                                                F(trigger, 0x70), (uint32_t)count);
            projectile_definition = TAG_DATA(projectile_tag);
            if (projectile_definition != 0 && (*(uint32_t *)(projectile_definition + 0x17c) & 0x10) &&
                holder != k_datum_index_none) {
                // inherit the root parent's velocity
                uint8_t *root = OBJECT_DATA(holder);

                while (D(root, 0x11c) != k_datum_index_none) {
                    root = OBJECT_DATA(D(root, 0x11c));
                }
                placement.velocity = *(real_vector3d *)&((struct object *)root)->velocity.i;
            } else {
                placement.velocity.i = placement.forward.i * speed;
                placement.velocity.j = placement.forward.j * speed;
                placement.velocity.k = placement.forward.k * speed;
            }
            from_player = (uint8_t)(holder_object != 0 && D(holder_object, 0x218) != k_datum_index_none);
            if (from_player) {
                placement.flags |= 2;
            }
            projectile = object_new_with_datum_role_control(&placement, role);
            if (projectile == k_datum_index_none) {
                continue;
            }
            if (from_player) {
                real_point3d camera;    // [esp+0x12c]

                unit_get_camera_position(holder, &camera);
                object_reposition_to_spawn_location(projectile, &camera, holder);
            }
            if (target != k_datum_index_none) {
                D(OBJECT_DATA(projectile), 0x238) = target;
            }
            if (!tracer) {
                *(uint32_t *)(OBJECT_DATA(projectile) + 0x22c) &= ~2u;
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
