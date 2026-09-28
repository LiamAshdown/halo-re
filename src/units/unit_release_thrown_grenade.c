// unit_release_thrown_grenade  (Ghidra: FUN_0056e440)
// address 0x56e440, size 988 bytes
// name confidence: 0.5 (functions.md summary matches; corroborated by throw-state writes)
// rewrite confidence: 0.9
// REWRITTEN from objdump 0x56e440..0x56e81b. Stack: (unit, early). While the unit is releasing (+0x28d == 2) its
//   grenade (+0x294) is detached and given a velocity: an actor's from its throw solution (0x410a60 with the
//   grenade's position), a player's along the aim from the camera plus the globals' grenade offsets (forward
//   +0x68, right +0x6c, up +0x70, relinked there), anything else along the aim; all at the unit's grenade speed
//   (+0x2c0 per second). An early release (arg) blends toward a weak random lob by the throw progress
//   (+0x28e / +0x290). Then the impulse is applied (0x4bef80), the unit forgets the grenade (state 3), the grenade
//   is swept back from the camera (deleted if that fails) and a client-authoritative grenade is sent.
// blam-cc: stack -> unit_index, early

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *globals_tag_data;   // 0x00746fa0, +0x174 the player information block
extern real_vector3d *global_up3d_pointer; // 0x00696720
extern int16_t game_connection_role;   // 0x00719720
extern uint8_t object_network_message_scratch[0x7ff8]; // 0x00871de0
extern void *network_server_pointer; // 0x0071c2d4

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);  // 0x4f6610
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX
extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out); // 0x568f80, ECX, EDI
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index,
    bsp_leaf_reference *location); // 0x4f5350, ESI, EDI, stack
extern uint32_t actor_compute_grenade_throw_vector(datum_index actor_index, real_point3d *grenade_position,
    real_vector3d *out_vector); // 0x410a60, EBX, stack
extern real random_real_range(real min, real max); // 0x401050
extern void object_apply_impulse_and_spin(uint32_t object_index, real_vector3d *delta_velocity); // 0x4bef80, EAX, EDX
extern uint8_t object_reposition_to_spawn_location(uint32_t object_index, real_point3d *target_position,
    uint32_t ignore_object_index); // 0x4f7b70, stack, ECX
extern void object_delete(uint32_t object_index);            // 0x4f5bd0, EAX
extern uint8_t object_is_delete_pending(uint32_t object_index); // 0x4f5c10, EAX
extern void object_type_override_call_0x68(uint32_t object_index); // 0x4f4560, ESI
extern int32_t projectile_send_creation(uint32_t projectile_index); // 0x4c0b10 (pushes the scratch buffer too)
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, char force, int32_t unused); // 0x4e1a80, EAX, ECX, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

void unit_release_thrown_grenade(uint32_t object_index, uint8_t early)
{
    uint8_t *unit = OBJECT_DATA(object_index);                                  // ebp
    uint8_t *unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data; // ebx
    real_vector3d *aim = (real_vector3d *)(unit + 0x23c);
    datum_index grenade;                        // [esp+0x18]
    real_vector3d velocity;                     // [esp+0x1c]

    if (unit[0x28d] != 2) {
        return;
    }
    grenade = *(datum_index *)(unit + 0x294);
    if (grenade == k_datum_index_none) {
        unit[0x28d] = 3;
        return;
    }
    object_snap_to_parent_marker_and_detach(grenade);
    if (*(datum_index *)(unit + 0x1f4) != k_datum_index_none) {
        real_point3d position;                  // [esp+0x40]

        object_get_position(&position, *(datum_index *)(unit + 0x294));
        actor_compute_grenade_throw_vector(*(datum_index *)(unit + 0x1f4), &position, &velocity);
    } else {
        if (*(datum_index *)(unit + 0x218) != k_datum_index_none) {
            // 0x56e4de: a player throws from the camera, offset by the globals' grenade offsets
            uint8_t *info = *(uint8_t **)(globals_tag_data + 0x174);
            real_vector3d forward = *aim;       // [esp+0x28]
            real_vector3d right;                // [esp+0x34]
            real_vector3d up;                   // [esp+0x40]
            real_point3d launch;                // [esp+0x1c]
            real forward_offset = *(float *)(info + 0x68);
            real right_offset = *(float *)(info + 0x6c);
            real up_offset = *(float *)(info + 0x70);

            vector3d_cross_product(&right, &forward, global_up3d_pointer);
            if (vector3d_normalize_with_length(&right) == 0.0f) {
                right = *global_up3d_pointer;
            }
            vector3d_cross_product(&up, &right, &forward);
            vector3d_normalize_with_length(&up);
            unit_get_camera_position(object_index, &launch);
            launch.x = launch.x + forward.i * forward_offset + right.i * right_offset + up.i * up_offset;
            launch.y = launch.y + forward.j * forward_offset + right.j * right_offset + up.j * up_offset;
            launch.z = launch.z + forward.k * forward_offset + right.k * right_offset + up.k * up_offset;
            object_set_position_and_relink(&launch, grenade, 0);
        }
        {
            real speed = *(float *)(unit_tag + 0x2c0) * 0.033333335f;

            velocity.i = speed * aim->i;
            velocity.j = speed * aim->j;
            velocity.k = speed * aim->k;
        }
    }

    // 0x56e644: an early release lobs weaker
    if (early) {
        real progress = (real)*(int16_t *)(unit + 0x28e) / (real)*(int16_t *)(unit + 0x290);

        if (progress < 1.0f) {
            real lob = random_real_range(0.02f, 0.046666667f);
            real rest = 1.0f - progress;

            velocity.i = lob * aim->i * rest + velocity.i * progress;
            velocity.j = lob * aim->j * rest + velocity.j * progress;
            velocity.k = lob * aim->k * rest + velocity.k * progress;
        }
    }

    // 0x56e711
    {
        uint8_t *object = OBJECT_DATA(grenade);
        real_vector3d delta;
        real_point3d camera;

        delta.i = velocity.i - *(float *)(object + 0x68);
        delta.j = velocity.j - *(float *)(object + 0x6c);
        delta.k = velocity.k - *(float *)(object + 0x70);
        object_apply_impulse_and_spin(grenade, &delta);
        *(datum_index *)(unit + 0x294) = k_datum_index_none;
        unit[0x28d] = 3;
        unit_get_camera_position(object_index, &camera);
        if (!object_reposition_to_spawn_location(grenade, &camera, k_datum_index_none)) {
            object_delete(grenade);
            return;
        }
    }
    if (*(int32_t *)(unit + 0x4) == 0 && game_connection_role == 2 && !object_is_delete_pending(grenade)) {
        *(int32_t *)(OBJECT_DATA(grenade) + 0x4) = 0;
        object_type_override_call_0x68(grenade);
        int32_t bits = projectile_send_creation(grenade);

        if (bits > 0) {
            network_session_broadcast_to_flagged(bits, network_server_pointer, 1, object_network_message_scratch, 1, 0, 0, 3);
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
