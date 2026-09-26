// unit_release_thrown_grenade  (Ghidra: already named unit_release_thrown_grenade)
// address 0x56e440, size 988 bytes
// name confidence: 0.5 (functions.md summary matches; corroborated by throw-state writes)
// rewrite confidence: 0.3 -- the two vector3d_cross_product calls, the two trailing UNSURE
//   network helpers and one object_reposition_to_spawn_location call site could not be pinned to exact register
//   arguments; see UNSURE notes.
// evidence: types/units.h unit_data.throwing_grenade_state/_counter/_duration/_projectile
//   (0x28d/0x28e/0x290/0x294), .actor_index (0x1f4), .controlling_player (0x218),
//   .aiming_vector (0x23c); types/tags.h Unit.grenade_velocity (0x2c0, confirmed by summing
//   the Unit tag layout: matches the *0.033333335 [1/30 tick] scale factor exactly);
//   types/objects.h object.network_role (0x004), object.velocity (0x068); callees
//   object_snap_to_parent_marker_and_detach (0x4f6610), object_set_position_and_relink
//   (0x4f5350, ESI=position/EDI=object_index), vector3d_cross_product (0x4052c0,
//   out=stack_operand x ecx_operand), object_type_override_call_0x68 (0x4f4560, ESI=object_index),
//   object_delete, object_is_delete_pending (already so named by Ghidra).
// register convention: object index in EAX (in_EAX / param_1), apply-throw-fraction flag in a
//   second register (param_2, a byte).
//   // blam-cc: EAX -> object_index, second register -> apply_throw_fraction
// UNSURE: the two vector3d_cross_product calls are register-only in the decompile. Read as
//   building an orthonormal (right, true_up) pair around the aim direction using the world-up
//   constant as a reference (right = up x aim, normalized, falling back to up itself when aim
//   is parallel to up; true_up = aim x right, normalized) -- the standard basis-rebuild idiom
//   used elsewhere in this module (e.g. 0x558860).
// UNSURE: globals_tag_data+0x174 (the "player information" block per types/units.h) is indexed
//   at +0x68/+0x6c/+0x70 for the forward/right/up throw-origin offsets; not named in the header.
// UNSURE: actor_compute_grenade_throw_vector (the AI-controlled branch's direction helper) and object_apply_impulse_and_spin (the
//   final impulse applier) are out of this module's range and kept with the argument counts
//   Ghidra shows at their call sites, which is not enough to assert a full prototype.
// UNSURE: the object_reposition_to_spawn_location call site passes two stack-looking values (projectile_index,
//   0xffffffff) that do not obviously match the (object_index, target_position) register
//   convention established for that address in src/objects/object_reposition_to_spawn_location.c
//   (target_position is dereferenced unconditionally there, which -1 cannot be); reproduced
//   literally rather than forced into that signature.
// UNSURE: projectile_send_creation's signature (object serialize into a scratch network buffer, return
//   size) is inferred only from its own call site here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *globals_tag_data;   // 0x00746fa0
extern real_vector3d *global_up3d_pointer; // 0x00696720, indirect pointer to math.h global_up3d
extern int32_t game_connection_role;   // 0x00719720
extern uint8_t object_network_message_scratch[0x7ff8]; // 0x00871de0

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand,
                                    real_vector3d *stack_operand); // 0x4052c0, out = stack_operand x ecx_operand
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);  // 0x4f6610
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX=out, ECX=object_index
extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out); // 0x568f80, UNSURE signature
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index); // 0x4f5350
extern void actor_compute_grenade_throw_vector(real_point3d *target, real_vector3d *out); // 0x410a60, UNSURE signature
extern real random_real_range(real min, real max); // 0x401050
extern void object_apply_impulse_and_spin(uint32_t object_index, real_vector3d *impulse); // 0x4bef80, UNSURE signature
extern uint8_t object_reposition_to_spawn_location(uint32_t object_index, real_point3d *target_position); // 0x4f7b70, UNSURE args, see header
extern void object_delete(uint32_t object_index);            // 0x4f5bd0, UNSURE exact signature
extern uint8_t object_is_delete_pending(uint32_t object_index); // 0x4f5c10
extern void object_type_override_call_0x68(uint32_t object_index); // 0x4f4560
extern int32_t projectile_send_creation(uint32_t object_index, uint8_t *buffer, uint32_t buffer_size); // 0x4c0b10, UNSURE
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, ECX server

// Detaches the grenade previously attached to the unit's hand, computes its launch velocity
// (a fixed speed along the aim direction for an AI unit, or a camera-relative toss origin plus
// a charge-scaled blend between a slow "drop" toss and the full throw speed for a player-driven
// unit), applies it as an impulse relative to the grenade's current velocity, marks the throw
// finished (state 3), and -- when running as the server for a unit with no controlling player --
// notifies the network layer of the release.
void unit_release_thrown_grenade(uint32_t object_index, uint8_t apply_throw_fraction)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    datum_index projectile_index;
    real_vector3d velocity;
    object *proj;

    if (unit->throwing_grenade_state != 2) {
        return;
    }

    projectile_index = unit->throwing_grenade_projectile;
    if (projectile_index == k_datum_index_none) {
        unit->throwing_grenade_state = 3;
        return;
    }

    object_snap_to_parent_marker_and_detach(projectile_index);

    if (unit->actor_index == k_datum_index_none) {
        real_vector3d aim = unit->aiming_vector;

        if (unit->controlling_player != k_datum_index_none) {
            uint8_t *player_info = globals_tag_data + 0x174;
            real_vector3d right, true_up;
            real_point3d launch_point;
            float forward_offset = *(float *)(player_info + 0x68);
            float right_offset = *(float *)(player_info + 0x6c);
            float up_offset = *(float *)(player_info + 0x70);

            vector3d_cross_product(&right, &aim, global_up3d_pointer); // right = up x aim
            if (vector3d_normalize_with_length(&right) == 0.0f) {
                right = *global_up3d_pointer;
            }
            vector3d_cross_product(&true_up, &right, &aim); // true_up = aim x right
            vector3d_normalize_with_length(&true_up);

            unit_get_camera_position(object_index, &launch_point);
            launch_point.x += true_up.i * up_offset + right.i * right_offset + aim.i * forward_offset;
            launch_point.y += true_up.j * up_offset + right.j * right_offset + aim.j * forward_offset;
            launch_point.z += true_up.k * up_offset + right.k * right_offset + aim.k * forward_offset;
            object_set_position_and_relink(&launch_point, projectile_index);
        }

        {
            float speed = tag->grenade_velocity * 0.033333335f;
            velocity.i = speed * aim.i;
            velocity.j = speed * aim.j;
            velocity.k = speed * aim.k;
        }

        if (apply_throw_fraction != 0) {
            float fraction = (float)unit->throwing_grenade_counter / (float)unit->throwing_grenade_duration;
            if (fraction < 1.0f) {
                float toss_speed = (float)random_real_range(0.020000001, 0.046666667);
                float remaining = 1.0f - fraction;
                real_vector3d toss = { toss_speed * aim.i, toss_speed * aim.j, toss_speed * aim.k };

                velocity.i = toss.i * remaining + velocity.i * fraction;
                velocity.j = toss.j * remaining + velocity.j * fraction;
                velocity.k = toss.k * remaining + velocity.k * fraction;
            }
        }
    } else {
        // UNSURE: both the EAX out-pointer and the ECX object_index are register-only at this
        // call site and could not be recovered; a scratch point and this unit's own index are
        // the best available placeholders.
        real_point3d target_position;
        object_get_position(&target_position, object_index);
        actor_compute_grenade_throw_vector(&target_position, &velocity);
    }

    proj = ((object_header *)object_data->data)[projectile_index & 0xffff].data;
    velocity.i -= proj->velocity.i;
    velocity.j -= proj->velocity.j;
    velocity.k -= proj->velocity.k;
    object_apply_impulse_and_spin(projectile_index, &velocity);

    unit->throwing_grenade_projectile = k_datum_index_none;
    unit->throwing_grenade_state = 3;

    {
        real_point3d discard;
        unit_get_camera_position(object_index, &discard); // UNSURE: result unused by the original, call kept for its side effect
    }

    if (object_reposition_to_spawn_location(projectile_index, (real_point3d *)0xffffffff) == 0) {
        object_delete(projectile_index);
        return;
    }

    if (obj->network_role == 0 && game_connection_role == 2) {
        if (object_is_delete_pending(projectile_index) == 0) {
            proj = ((object_header *)object_data->data)[projectile_index & 0xffff].data;
            proj->network_role = 0;
            object_type_override_call_0x68(projectile_index);
            {
                int32_t encoded_size = projectile_send_creation(projectile_index, object_network_message_scratch, 0x7ff8);
                if (encoded_size > 0) {
                    network_session_broadcast_to_flagged(network_server_pointer, 1, object_network_message_scratch, 1, 0, 0, 3);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x56e440):

void unit_release_thrown_grenade(uint param_1,char param_2)

{
  float fVar1;
  uint *puVar2;
  uint uVar3;
  float fVar4;
  undefined *puVar5;
  char cVar6;
  undefined4 uVar7;
  int iVar8;
  int iVar9;
  float10 fVar10;
  float fVar11;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar8 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(char *)((int)puVar2 + 0x28d) == '\x02') {
    uVar3 = puVar2[0xa5];
    if (uVar3 == 0xffffffff) {
      *(undefined1 *)((int)puVar2 + 0x28d) = 3;
    }
    else {
      FUN_004f6610(uVar3);
      puVar5 = PTR_DAT_00696720;
      if (puVar2[0x7d] == 0xffffffff) {
        if (puVar2[0x86] != 0xffffffff) {
          iVar9 = *(int *)(DAT_00746fa0 + 0x174);
          local_24 = (float)puVar2[0x8f];
          local_20 = (float)puVar2[0x90];
          local_1c = (float)puVar2[0x91];
          vector3d_cross_product(PTR_DAT_00696720);
          fVar10 = (float10)vector3d_normalize_with_length();
          if ((float10)0.0 == fVar10) {
            local_18 = *(float *)puVar5;
            local_14 = *(float *)(puVar5 + 4);
            local_10 = *(float *)(puVar5 + 8);
          }
          vector3d_cross_product(&local_24);
          vector3d_normalize_with_length();
          unit_get_camera_position();
          fVar1 = *(float *)(iVar9 + 0x68);
          fVar4 = *(float *)(iVar9 + 0x6c);
          fVar11 = *(float *)(iVar9 + 0x70);
          local_30 = local_c * fVar11 + local_18 * fVar4 + local_24 * fVar1 + local_30;
          local_2c = local_8 * fVar11 + local_14 * fVar4 + local_20 * fVar1 + local_2c;
          local_28 = local_4 * fVar11 + local_10 * fVar4 + local_1c * fVar1 + local_28;
          object_set_position_and_relink(0);
        }
        local_28 = *(float *)(iVar8 + 0x2c0) * 0.033333335;
        local_30 = local_28 * (float)puVar2[0x8f];
        local_2c = local_28 * (float)puVar2[0x90];
        local_28 = local_28 * (float)puVar2[0x91];
      }
      else {
        uVar7 = object_get_position();
        FUN_00410a60(uVar7,&local_30);
      }
      if (param_2 != '\0') {
        fVar1 = (float)(int)*(short *)((int)puVar2 + 0x28e) / (float)(int)(short)puVar2[0xa4];
        if (fVar1 < 1.0) {
          fVar11 = random_real_range(0.020000001,0.046666667);
          local_20 = fVar11 * (float)puVar2[0x90];
          local_1c = fVar11 * (float)puVar2[0x91];
          fVar4 = 1.0 - fVar1;
          local_30 = fVar11 * (float)puVar2[0x8f] * fVar4 + local_30 * fVar1;
          local_2c = local_20 * fVar4 + local_2c * fVar1;
          local_28 = local_1c * fVar4 + local_28 * fVar1;
        }
      }
      iVar9 = (uVar3 & 0xffff) * 0xc;
      iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
      local_30 = local_30 - *(float *)(iVar8 + 0x68);
      local_2c = local_2c - *(float *)(iVar8 + 0x6c);
      local_28 = local_28 - *(float *)(iVar8 + 0x70);
      FUN_004bef80();
      puVar2[0xa5] = 0xffffffff;
      *(undefined1 *)((int)puVar2 + 0x28d) = 3;
      unit_get_camera_position();
      cVar6 = FUN_004f7b70(uVar3,0xffffffff);
      if (cVar6 == '\0') {
        object_delete();
        return;
      }
      if ((puVar2[1] == 0) && (DAT_00719720 == 2)) {
        cVar6 = object_is_delete_pending();
        if (cVar6 == '\0') {
          *(undefined4 *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9) + 4) = 0;
          object_type_override_call_0x68();
          iVar8 = FUN_004c0b10(uVar3,&DAT_00871de0,0x7ff8);
          if (0 < iVar8) {
            FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
            return;
          }
        }
      }
    }
  }
  return;
}
#endif
