// projectile_collision_test  (Ghidra: FUN_004c0450; renamed per
// out/phase4/projectiles_types_notes.md: "point sweep plus two collision_radius-offset sweeps")
// address 0x4c0450, size 536 bytes
// name confidence: 0.7   rewrite confidence: 0.8 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: types/projectiles.h k_projectile_collision_mask_point (0x1000e9),
//   k_projectile_collision_mask_radius (0x89), collision_result (the caller's 0x50-byte output
//   buffer, here just an opaque out-pointer); types/projectiles.h projectile_data.
//   ignore_object_index (0x234); types/tags.h Projectile.collision_radius (0x1a0);
//   types/math.h global_up3d_pointer (0x00696720), global_left3d_pointer (0x0069671c).
// register convention: object index in EAX (in_EAX); the swept-to point (real_point3d *) in
//   EDI (unaff_EDI); the caller's collision_result output buffer is Ghidra's one recognized
//   stack parameter (param_1).
// blam-cc: EAX -> object_index, EDI -> target, stack -> out_record
// First tries a straight point sweep from the object's own position to `target`. If that finds
// nothing and the projectile has a meaningful collision_radius, it re-tries with two rays offset
// by +-collision_radius along a direction perpendicular to the sweep and to global up (falling
// back to global left if the sweep is parallel to up).
// UNSURE: collision_test_movement_segment and collision_test_movement_segment_between_points are the collision module's swept-test entry points
// (0x00505880 established in src/units/vehicle_create_hover_thruster_effects.c;
// collision_test_movement_segment_between_points's 3-argument opaque form established in src/items/item_update.c and
// src/units/unit_find_placement_position.c). RESOLVED this pass by disassembly, replacing the
// earlier "not wired into an argument" note:
//   - 0x00401a20 is "sweep between two POINTS": it computes delta = *ECX - *EAX into a local and
//     tail-calls collision_test_movement_segment(mask, EAX, &delta, ignore, out). Its two points are register-only.
//   - 0x4c0647 `lea ecx,[esp+0x48]` / 0x4c064b `lea eax,[esp+0x54]` hand it the -collision_radius
//     TARGET point and the -collision_radius ORIGIN point. That is why the original code never
//     computes a delta for the second offset ray: 0x00401a20 does it. The earlier rewrite built
//     a `minus_delta` and dropped it on the floor; it now builds minus_target instead.
//   - The cross-product operands are likewise resolved: 0x4c04e4 `mov ecx,ds:0x696720` pushes
//     global_up3d as the stack operand, 0x4c04f1 `lea ecx,[esp+0x28]` puts the sweep direction
//     in ECX and 0x4c04ed `lea eax,[esp+0x10]` is the output, i.e. exactly the call written
//     below under the vector3d_cross_product signature this repo already uses.
// Return value: AL only (`xor al,al` / `mov al,0x1`), hence uint8_t rather than a full word --
// the caller in projectile_update.c tests it as a byte.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"
#include "fn_math.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_up3d_pointer;   // 0x00696720
extern real_vector3d *global_left3d_pointer; // 0x0069671c

extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0, out = stack_operand x ecx_operand

extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t collision_mask,
                            uint32_t ignore_object_index, void *out_record); // 0x401a20
    // blam-cc: ECX -> target, EAX -> origin, stack -> (collision_mask, ignore_object_index,
    // out_record). Computes delta = *target - *origin and tail-calls collision_test_movement_segment.

uint8_t projectile_collision_test(uint32_t object_index, real_point3d *target, void *out_record) // blam-cc: EAX -> object_index, EDI -> target, stack -> out_record
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Projectile *tag = (Projectile *)tag_instances[(uint16_t)obj->definition_tag].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    real_vector3d sweep_delta;
    uint8_t hit;

    sweep_delta.i = target->x - obj->position.x;
    sweep_delta.j = target->y - obj->position.y;
    sweep_delta.k = target->z - obj->position.z;

    hit = collision_test_movement_segment(k_projectile_collision_mask_point, &obj->position, &sweep_delta,
                        (uint32_t)proj->ignore_object_index, out_record);
    if (hit != 0) {
        return 1;
    }
    if (tag->collision_radius < 0.0001f) {
        return 0;
    }

    {
        real_vector3d direction, perpendicular;
        real_point3d plus_origin, minus_origin, minus_target;
        real_vector3d plus_delta;
        real radius;

        direction.i = target->x - obj->position.x;
        direction.j = target->y - obj->position.y;
        direction.k = target->z - obj->position.z;

        vector3d_cross_product(&perpendicular, &direction, global_up3d_pointer); // operands confirmed by disassembly, see file header
        if (vector3d_normalize_with_length(&perpendicular) == 0.0f) {
            perpendicular = *global_left3d_pointer;
        }

        radius = tag->collision_radius;
        plus_origin.x = perpendicular.i * radius + obj->position.x;
        plus_origin.y = perpendicular.j * radius + obj->position.y;
        plus_origin.z = perpendicular.k * radius + obj->position.z;
        plus_delta.i = (perpendicular.i * radius + target->x) - plus_origin.x;
        plus_delta.j = (perpendicular.j * radius + target->y) - plus_origin.y;
        plus_delta.k = (radius * perpendicular.k + target->z) - plus_origin.z;

        radius = -tag->collision_radius;
        minus_origin.x = perpendicular.i * radius + obj->position.x;
        minus_origin.y = perpendicular.j * radius + obj->position.y;
        minus_origin.z = perpendicular.k * radius + obj->position.z;
        // the second offset ray is passed as two POINTS, not as origin+delta; collision_test_movement_segment_between_points
        // subtracts them itself (see file header)
        minus_target.x = perpendicular.i * radius + target->x;
        minus_target.y = perpendicular.j * radius + target->y;
        minus_target.z = perpendicular.k * radius + target->z;

        hit = collision_test_movement_segment(k_projectile_collision_mask_radius, &plus_origin, &plus_delta,
                            (uint32_t)proj->ignore_object_index, out_record);
        if (hit == 0) {
            hit = collision_test_movement_segment_between_points(&minus_origin, &minus_target, k_projectile_collision_mask_radius, // 0x4c0647: ECX = &minus_target, EAX = &minus_origin
                                   (uint32_t)proj->ignore_object_index, out_record);
            if (hit == 0) {
                return 0;
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4c0450):

undefined4 FUN_004c0450(undefined4 param_1)

{
  float *pfVar1;
  float fVar2;
  uint *puVar3;
  int iVar4;
  float fVar5;
  char cVar6;
  uint in_EAX;
  float *unaff_EDI;
  float10 fVar7;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
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

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  local_3c = *unaff_EDI - (float)puVar3[0x17];
  pfVar1 = (float *)(puVar3 + 0x17);
  local_38 = unaff_EDI[1] - (float)puVar3[0x18];
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_34 = unaff_EDI[2] - (float)puVar3[0x19];
  cVar6 = FUN_00505880(0x1000e9,pfVar1,&local_3c,puVar3[0x8d],param_1);
  if (cVar6 == '\0') {
    if (*(float *)(iVar4 + 0x1a0) < 0.0001) {
      return 0;
    }
    local_30 = *unaff_EDI - *pfVar1;
    local_2c = unaff_EDI[1] - (float)puVar3[0x18];
    local_28 = unaff_EDI[2] - (float)puVar3[0x19];
    vector3d_cross_product(PTR_DAT_00696720);
    fVar7 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar7) {
      local_48 = *(float *)PTR_DAT_0069671c;
      local_44 = *(float *)(PTR_DAT_0069671c + 4);
      local_40 = *(float *)(PTR_DAT_0069671c + 8);
    }
    fVar2 = *(float *)(iVar4 + 0x1a0);
    local_3c = local_48 * fVar2 + *pfVar1;
    local_38 = local_44 * fVar2 + (float)puVar3[0x18];
    local_34 = local_40 * fVar2 + (float)puVar3[0x19];
    fVar2 = *(float *)(iVar4 + 0x1a0);
    local_30 = local_48 * fVar2 + *unaff_EDI;
    local_2c = local_44 * fVar2 + unaff_EDI[1];
    fVar5 = -*(float *)(iVar4 + 0x1a0);
    local_c = local_48 * fVar5 + *pfVar1;
    local_8 = local_44 * fVar5 + (float)puVar3[0x18];
    local_4 = local_40 * fVar5 + (float)puVar3[0x19];
    local_18 = local_48 * fVar5 + *unaff_EDI;
    local_14 = local_44 * fVar5 + unaff_EDI[1];
    local_10 = local_40 * fVar5 + unaff_EDI[2];
    local_24 = local_30 - local_3c;
    local_20 = local_2c - local_38;
    local_1c = (fVar2 * local_40 + unaff_EDI[2]) - local_34;
    cVar6 = FUN_00505880(0x89,&local_3c,&local_24,puVar3[0x8d],param_1);
    if ((cVar6 == '\0') && (cVar6 = FUN_00401a20(0x89,puVar3[0x8d],param_1), cVar6 == '\0')) {
      return 0;
    }
  }
  return 1;
}
#endif
