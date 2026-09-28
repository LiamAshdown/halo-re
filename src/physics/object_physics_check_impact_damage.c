// object_physics_check_impact_damage  (Ghidra: FUN_00508b70; renamed)
// address 0x508b70, size 1355 bytes
// name confidence: 0.4   rewrite confidence: 0.85 -- VERIFIED 2026-09-27 (static loop) against objdump
//   0x508b70..0x5090ba end to end (every local, push order and constant: 0.5, 1/64, 0.8, 0.1, 1/15,
//   1/900, -1.0); the only defect left was the test_point call (fixed below). Earlier history: raised from 0.2 by the phase-4 integration
//   pass, which re-derived the whole body line by line against `python tools/pack.py 0x508b70`
//   and corrected eight places where the first rewrite had drifted:
//     - local_aca0/9c/98 (the impulse vector) and local_ac94 (a scratch float) had been fused,
//       so impulse.k carried local_ac94's value and the FUN_00505200 radius argument was passed
//       from the wrong local; both now match the decompile.
//     - local_ac8c's first role is `local_ac84 - 0.015625`, not `local_ac80 - local_ac88*0.5`,
//       and it reaches FUN_00505200 as a float, not a real_vector3d *.
//     - local_ac88/84/80/7c/78 is ONE five-float block: local_ac80/7c/78 are simultaneously
//       unit_get_crouch_height_offset's last three outputs and the contact point. The first rewrite declared two
//       separate variables, breaking the aliasing the `local_ac80 = 2*impulse.i + local_ac80`
//       accumulation depends on.
//     - puVar2[0x28..0x2a] and local_ac1c are both `object.bounding_center` (+0xa0), not
//       `object.position` (+0x5c).
//     - `local_ac10 = local_ac10 - local_ac84` (out_position.z pulled back down by the y margin
//       after physics_point_find_clear_position) had been dropped entirely.
//     - `*(uint *)(iVar11 + 0x324)` is the SELF object's unit_data.driver_unit_index, not the
//       candidate's parent_object; the earlier rewrite damaged the wrong party's owner.
//     - the second damage block reads `*(int *)((int)local_ac88 + 0x58)`, i.e. the same
//       Globals sub-tag the first block used, not `Globals + 0x188 + 0x58`.
//     - that block's material scale is indexed through the CANDIDATE object's tag
//       (`*local_ac8c & 0xffff`), not the self object's, and lands in random_blend (+0x40), not
//       multiplier (+0x44).
//   The remaining 0.35 ceiling is the hidden-argument problem described below, not the flow.
// evidence: out/phase4/physics_functions.md ("Checks whether a physics/antenna object is
//   hitting another object hard enough to hurt it and, if so, applies damage via
//   object_apply_damage"); types/objects.h damage_data (size 0x54, matching the 0x15-dword zero
//   loop here field for field: flags, responsible_player/object, team_index,
//   location_cluster_index, epicentre, origin, direction, random_blend, multiplier, unknown_4c);
//   src/items/weapon_fire_trigger.c's own object_apply_damage(damage_data*, target, node,
//   param_4, material, param_6) extern, reused verbatim here.
// register convention: param_1/param_2 are Ghidra's own recognized parameters. param_1 is read
//   here only as *param_1 (a single object index), but it is forwarded whole to FUN_00505200
//   (object_collision_context_gather_sphere_shapes), whose param_1 is an object_collision_context
//   -- so param_1 really is the 0x10-byte context object_physics_handle_nearby_object_impacts
//   allocates, whose first field is that index. Typed as uint32_t * here because that is the only
//   field this function itself touches.
//   // blam-cc: stack -> self_object_index, candidate_object_index
// UNSURE (major): the whole opening block (unit_get_crouch_height_offset, object_collision_context_test_point, FUN_00505200,
//   physics_shape_test_point) that decides whether to recover a contact point when the self object's
//   collision nodes are not fully resolved -- every intermediate float here (local_ac88..ac78)
//   is read before any visible write, meaning Ghidra lost every store; this rewrite declares the
//   same shape of locals and preserves the arithmetic, but the exact meaning of unit_get_crouch_height_offset's
//   and object_collision_context_test_point's outputs is not established.
// UNSURE (major): the "falling damage" style Globals sub-tag chase (DAT_00746fa0+0x18c, then
//   +0x68) is foreign to this module and kept as raw offsets.
// UNSURE: object_set_position_and_relink is called with a single literal 0 argument in the
//   decompile, which cannot be its real signature (it plainly needs at least an object index and
//   a position); preserved as Ghidra shows it, since the true hidden arguments are unrecoverable
//   here.
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"
#include <string.h>
#include "game.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern Globals *global_globals;       // 0x00746fa0
extern game_time_globals *game_time; // 0x006f1d6c
                                    // game_time; same global src/items and src/devices use)
extern float k_impact_damage_scale_table[]; // 0x0069c54c, indexed by material type per
                                             // types/physics.h's k_physics_impact_damage_scale
                                             // UNSURE note

extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
    float *pill_radius_out); // 0x55a2e0, EAX, ECX, stack, EBX
extern uint32_t object_collision_context_test_point(object_collision_context *context,
    real_point3d *point); // 0x504e90, EBX context, stack point
extern uint8_t object_collision_context_gather_sphere_shapes(void *context,
    real_point3d *origin, float radius_scale, float margin, float thickness,
    physics_model *model); // 0x505200, this module; param_1 is this function's own param_1, the
                           // 0x10-byte object_collision_context its caller built
extern uint8_t physics_shape_test_point(physics_model *model, real_point3d *point,
    physics_model_contact *out_contact); // 0x504260, this module (higher half)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, math module
    // (src/math/vector3d_normalize_with_length.c); normalizes in place, returns the old length
extern void unit_apply_impulse(uint32_t object_index, real_vector3d *impulse); // 0x559fa0, EAX, EDI
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index,
    bsp_leaf_reference *location); // 0x4f5350, ESI, EDI, stack (location may be 0)
                                                                // args, see file header
extern void object_apply_damage(damage_data *dd, uint32_t target_object_index, int16_t node_index,
    int16_t region_index, int16_t material_index, uint32_t plane); // 0x4ee5e0, objects module
extern uint8_t physics_point_find_clear_position(uint32_t flags, real_point3d *current_position,
    float sample_radius, float x_margin, float y_margin, uint32_t exclude_object_index,
    real_point3d *out_position); // 0x507170, this module
extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// Tests whether self_object_index and candidate_object_index are colliding hard enough for
// impact damage: recovers a contact point via FUN_00505200/physics_shape_test_point when self's own
// collision-node resolution (object_collision_context_test_point) is incomplete, computes a relative-impact velocity,
// nudges the candidate via object_set_position_and_relink, and applies damage to whichever side
// (or both) the two "falling damage"-style thresholds indicate.
uint8_t object_physics_check_impact_damage(uint32_t *self_object_index, uint32_t candidate_object_index)
{
    uint8_t hit_recorded = 0;          // local_ac8d
    float sample[5];                   // local_ac88/84/80/7c/78; sample[2..4] are one real_point3d
    real_point3d *contact_point = (real_point3d *)&sample[2];
    real_vector3d impulse;             // local_aca0/9c/98
    real_point3d recovered_position;   // local_ac18/14/10
    damage_data dd;                    // local_ac74 .. local_ac28
    object *self_obj;
    object *candidate_obj;
    real_point3d *self_center;         // local_ac1c, captured before iVar11 is reassigned
    float relative_speed;              // local_ac94 in its second role
    float clamped_speed;

    // FIXED (objdump 0x508b84..0x508b99): EAX = &sample[2] (position), ECX = the candidate (ESI), stack = &sample[0]
    //   (pill height), EBX = &sample[1] (pill radius). The draft passed only the buffer.
    unit_get_crouch_height_offset(contact_point, candidate_object_index, &sample[0], &sample[1]);

    // FIXED 2026-09-27 (static loop): 0x508b9e..0x508bad is `mov ebx,[arg1] / push &contact / call 0x504e90` --
    // EBX = the caller's collision context, the point on the stack. The draft passed only the point, so the callee
    // used the point as the context and read a garbage point.
    if (!object_collision_context_test_point((object_collision_context *)self_object_index, contact_point)) {
        physics_model model;
        physics_model_contact contact;
        float sphere_radius;           // local_ac94 in its first role
        float thickness;               // local_ac8c in its first role

        impulse.i = contact_point->x;
        impulse.j = contact_point->y;
        impulse.k = contact_point->z + sample[0] * 0.5f;
        model.sphere_count = 0;
        model.pill_count = 0;
        model.shape_count = 0;
        sphere_radius = sample[0] * 0.5f + sample[1];
        thickness = sample[1] - 0.015625f;
        if (thickness <= 0.015625f) {
            thickness = 0.015625f;
        }

        object_collision_context_gather_sphere_shapes(self_object_index,
            (real_point3d *)&impulse, sphere_radius, sample[0], thickness, &model);
        if (!physics_shape_test_point(&model, contact_point, &contact)) {
            return hit_recorded;
        }
    }

    self_obj = ((object_header *)object_data->data)[*self_object_index & 0xffff].data;
    candidate_obj = ((object_header *)object_data->data)[candidate_object_index & 0xffff].data;
    self_center = &self_obj->bounding_center;

    relative_speed = (float)sqrt((double)(self_obj->velocity.k * self_obj->velocity.k +
        self_obj->velocity.j * self_obj->velocity.j + self_obj->velocity.i * self_obj->velocity.i));

    // both ends are the bounding centre (object +0xa0), not the position at +0x5c
    impulse.i = candidate_obj->bounding_center.x - self_center->x;
    impulse.j = candidate_obj->bounding_center.y - self_center->y;
    impulse.k = candidate_obj->bounding_center.z - self_center->z;

    vector3d_normalize_with_length(&impulse); // UNSURE: operand is a register argument
    impulse.k += 0.8f;
    vector3d_normalize_with_length(&impulse); // UNSURE: operand is a register argument

    clamped_speed = (relative_speed <= 0.1f) ? 0.1f : relative_speed;
    impulse.i = (impulse.i * clamped_speed + self_obj->velocity.i) * 0.5f;
    impulse.j = (impulse.j * clamped_speed + self_obj->velocity.j) * 0.5f;
    impulse.k = (clamped_speed * impulse.k + self_obj->velocity.k) * 0.5f;

    unit_apply_impulse(candidate_object_index, &impulse); // FIXED: EAX = ESI (the candidate), EDI = &impulse (0x508d28)

    // sample[2..4] IS contact_point; the original aliases local_ac80/7c/78 the same way
    contact_point->x = impulse.i + impulse.i + contact_point->x;
    contact_point->y = impulse.j + impulse.j + contact_point->y;
    contact_point->z = impulse.k + impulse.k + contact_point->z;

    hit_recorded = 0;
    {
        // UNSURE: current_position (hidden ESI) assumed to be contact_point, see file header
        uint8_t recovered = physics_point_find_clear_position(0x20c3a0, contact_point,
            sample[1] + sample[1], sample[0], sample[1], candidate_object_index,
            &recovered_position);
        if (recovered) {
            recovered_position.z = recovered_position.z - sample[1];
            // 0x508de2: ESI = the recovered position, EDI = the candidate, no location
            object_set_position_and_relink(&recovered_position, candidate_object_index, 0);

            // candidate +0x32c / +0x330 are unit_data.last_parent_object_index and
            // last_seat_change_tick (types/units.h): a unit that just dismounted from this very
            // object within the last 90 ticks is never hurt by it.
            if (*self_object_index == *(uint32_t *)((uint8_t *)candidate_obj + 0x32c) &&
                game_time->game_time <=
                    (int32_t)(*(uint32_t *)((uint8_t *)candidate_obj + 0x330) + 0x5a)) {
                return 1;
            }
            if (relative_speed <= 0.06666667f) {
                float dx = candidate_obj->velocity.i - self_obj->velocity.i;
                float dy = candidate_obj->velocity.j - self_obj->velocity.j;
                float dz = candidate_obj->velocity.k - self_obj->velocity.k;
                if (dx * dx + dy * dy + dz * dz <= 0.0011111111f) {
                    return 1;
                }
            }
        }
    }

    {
        // local_ac88 and local_ac94 are both reused here as the sub-tag pointer and its field
        uint8_t *collision_damage_tag = *(uint8_t **)((uint8_t *)global_globals + 0x18c);
        int32_t impact_damage_tag_id = *(int32_t *)(collision_damage_tag + 0x68);
        int32_t breakable_damage_tag_id;

        if (impact_damage_tag_id != -1) {
            // self +0x324 is unit_data.driver_unit_index (types/units.h): the damage is credited
            // to whoever is driving the object that did the hitting.
            uint32_t driver = *(uint32_t *)((uint8_t *)self_obj + 0x324);
            uint32_t responsible = *self_object_index;
            object *responsible_obj = self_obj;

            if (driver != 0xffffffff) {
                responsible_obj = ((object_header *)object_data->data)[driver & 0xffff].data;
                responsible = driver;
            }

            memset(&dd, 0, sizeof(dd));
            dd.flags |= 1;
            dd.material_type = -1;
            dd.location_cluster_index = -1;
            dd.multiplier = 1.0f;
            dd.random_blend = 1.0f;
            dd.responsible_player = responsible_obj->owner_linkage;
            dd.responsible_object = responsible;
            if (responsible_obj->creator_object != 0xffffffff) {
                dd.responsible_object = responsible_obj->creator_object;
            }
            dd.team_index = responsible_obj->owner_team; // matches local_ac64 = *(short *)
                                                          // (responsible_obj + 0xb8); see header
            dd.epicentre = candidate_obj->bounding_center;
            dd.origin = *self_center;   // still the ORIGINAL self object, not responsible_obj
            dd.direction = impulse;
            dd.damage_effect_tag = impact_damage_tag_id;

            vector3d_normalize_with_length(&dd.direction); // UNSURE: operand is a register
            object_apply_damage(&dd, candidate_object_index, -1, -1, -1, 0);
        }

        breakable_damage_tag_id = *(int32_t *)(collision_damage_tag + 0x58);
        if (breakable_damage_tag_id != -1) {
            // the material scale is indexed by the CANDIDATE object's own tag, +0x298
            void *candidate_tag = tag_instances[candidate_obj->definition_tag & 0xffff].data;

            memset(&dd, 0, sizeof(dd));
            dd.epicentre = candidate_obj->bounding_center;
            dd.team_index = -1;
            dd.responsible_player = -1;
            dd.responsible_object = -1;
            dd.location_cluster_index = -1;
            dd.material_type = -1;
            dd.multiplier = 1.0f;
            dd.random_blend = k_impact_damage_scale_table[
                *(int16_t *)((uint8_t *)candidate_tag + 0x298)];
            dd.direction.i = impulse.i * -1.0f;
            dd.direction.j = impulse.j * -1.0f;
            dd.direction.k = impulse.k * -1.0f;
            dd.damage_effect_tag = breakable_damage_tag_id;

            object_apply_damage(&dd, *self_object_index, -1, -1, -1, 0);
        }
    }

    return 1;
}


#if 0
Original Ghidra decompilation (0x508b70):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined1 FUN_00508b70(uint *param_1,uint param_2)

{
  float *pfVar1;
  uint *puVar2;
  uint uVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  char cVar7;
  int iVar8;
  int iVar9;
  uint uVar10;
  int iVar11;
  int *piVar12;
  float local_aca0;
  float local_ac9c;
  float local_ac98;
  float local_ac94;
  undefined1 local_ac8d;
  uint *local_ac8c;
  float local_ac88;
  float local_ac84;
  float local_ac80;
  float local_ac7c;
  float local_ac78;
  int local_ac74 [4];
  undefined2 local_ac64;
  undefined2 local_ac5c;
  float local_ac58;
  uint local_ac54;
  uint local_ac50;
  float local_ac4c;
  float local_ac48;
  float local_ac44;
  float local_ac40;
  float local_ac3c;
  float local_ac38;
  undefined4 local_ac34;
  undefined4 local_ac30;
  undefined2 local_ac28;
  float *local_ac1c;
  undefined1 local_ac18 [8];
  float local_ac10;
  undefined4 local_ac0c;
  undefined2 local_ac08;
  undefined4 uStack_4;

  uStack_4 = 0x508b7a;
  local_ac8d = 0;
  FUN_0055a2e0(&local_ac88);
  cVar7 = FUN_00504e90(&local_ac80);
  if (cVar7 == '\0') {
    local_aca0 = local_ac80;
    local_ac98 = local_ac78 + local_ac88 * 0.5;
    local_ac0c = 0;
    local_ac08 = 0;
    local_ac9c = local_ac7c;
    local_ac94 = local_ac88 * 0.5 + local_ac84;
    local_ac8c = (uint *)(local_ac84 - 0.015625);
    if ((float)local_ac8c <= 0.015625) {
      local_ac8c = (uint *)0x3c800000;
    }
    FUN_00505200(param_1,&local_aca0,local_ac94,local_ac88,local_ac8c,&local_ac0c);
    cVar7 = FUN_00504260(&local_ac0c,&local_ac80,local_ac74);
    if (cVar7 == '\0') {
      return local_ac8d;
    }
  }
  iVar11 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*param_1 & 0xffff) * 0xc);
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
  pfVar1 = (float *)(puVar2 + 0x28);
  local_ac1c = (float *)(iVar11 + 0xa0);
  local_ac94 = SQRT(*(float *)(iVar11 + 0x70) * *(float *)(iVar11 + 0x70) +
                    *(float *)(iVar11 + 0x6c) * *(float *)(iVar11 + 0x6c) +
                    *(float *)(iVar11 + 0x68) * *(float *)(iVar11 + 0x68));
  local_aca0 = *pfVar1 - *local_ac1c;
  local_ac9c = (float)puVar2[0x29] - *(float *)(iVar11 + 0xa4);
  local_ac98 = (float)puVar2[0x2a] - *(float *)(iVar11 + 0xa8);
  local_ac8c = puVar2;
  vector3d_normalize_with_length();
  local_ac98 = local_ac98 + 0.8;
  vector3d_normalize_with_length();
  fVar4 = local_ac94;
  if (local_ac94 <= 0.1) {
    fVar4 = 0.1;
  }
  local_aca0 = (local_aca0 * fVar4 + *(float *)(iVar11 + 0x68)) * 0.5;
  local_ac9c = (local_ac9c * fVar4 + *(float *)(iVar11 + 0x6c)) * 0.5;
  local_ac98 = (fVar4 * local_ac98 + *(float *)(iVar11 + 0x70)) * 0.5;
  FUN_00559fa0();
  local_ac80 = local_aca0 + local_aca0 + local_ac80;
  local_ac7c = local_ac9c + local_ac9c + local_ac7c;
  local_ac78 = local_ac98 + local_ac98 + local_ac78;
  cVar7 = FUN_00507170(0x20c3a0,local_ac84 + local_ac84,local_ac88,local_ac84,param_2,local_ac18);
  if (cVar7 != '\0') {
    local_ac10 = local_ac10 - local_ac84;
    object_set_position_and_relink(0);
    if ((*param_1 == local_ac8c[0xcb]) &&
       (*(int *)(DAT_006f1d6c + 0xc) <= (int)(local_ac8c[0xcc] + 0x5a))) {
      return 1;
    }
    if ((local_ac94 <= 0.06666667) &&
       (fVar4 = (float)local_ac8c[0x1a] - *(float *)(iVar11 + 0x68),
       fVar6 = (float)local_ac8c[0x1b] - *(float *)(iVar11 + 0x6c),
       fVar5 = (float)local_ac8c[0x1c] - *(float *)(iVar11 + 0x70),
       fVar4 * fVar4 + fVar6 * fVar6 + fVar5 * fVar5 <= 0.0011111111)) {
      return 1;
    }
  }
  local_ac88 = *(float *)(DAT_00746fa0 + 0x18c);
  local_ac94 = *(float *)((int)local_ac88 + 0x68);
  if (local_ac94 != -NAN) {
    uVar3 = *(uint *)(iVar11 + 0x324);
    uVar10 = *param_1;
    if (uVar3 != 0xffffffff) {
      iVar11 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
      uVar10 = uVar3;
    }
    piVar12 = local_ac74;
    for (iVar8 = 0x15; iVar8 != 0; iVar8 = iVar8 + -1) {
      *piVar12 = 0;
      piVar12 = piVar12 + 1;
    }
    local_ac74[1] = local_ac74[1] | 1;
    local_ac28 = 0xffff;
    local_ac5c = 0xffff;
    local_ac30 = 0x3f800000;
    local_ac34 = 0x3f800000;
    local_ac74[2] = *(undefined4 *)(iVar11 + 0xc0);
    local_ac74[3] = uVar10;
    if (*(uint *)(iVar11 + 0xc4) != 0xffffffff) {
      local_ac74[3] = *(uint *)(iVar11 + 0xc4);
    }
    local_ac64 = *(undefined2 *)(iVar11 + 0xb8);
    local_ac58 = *pfVar1;
    local_ac54 = puVar2[0x29];
    local_ac50 = puVar2[0x2a];
    local_ac4c = *local_ac1c;
    local_ac48 = local_ac1c[1];
    local_ac44 = local_ac1c[2];
    local_ac40 = local_aca0;
    local_ac3c = local_ac9c;
    local_ac38 = local_ac98;
    local_ac74[0] = (int)local_ac94;
    vector3d_normalize_with_length();
    object_apply_damage(local_ac74,param_2,0xffffffff,0xffffffff,0xffffffff,0);
  }
  iVar11 = *(int *)((int)local_ac88 + 0x58);
  if (iVar11 != -1) {
    iVar8 = *(int *)((*local_ac8c & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    piVar12 = local_ac74;
    for (iVar9 = 0x15; iVar9 != 0; iVar9 = iVar9 + -1) {
      *piVar12 = 0;
      piVar12 = piVar12 + 1;
    }
    local_ac58 = *pfVar1;
    local_ac54 = puVar2[0x29];
    local_ac28 = 0xffff;
    local_ac74[2] = 0xffffffff;
    local_ac74[3] = 0xffffffff;
    local_ac64 = 0xffff;
    local_ac5c = 0xffff;
    local_ac30 = 0x3f800000;
    local_ac34 = *(undefined4 *)(&DAT_0069c54c + *(short *)(iVar8 + 0x298) * 4);
    local_ac50 = puVar2[0x2a];
    local_ac40 = local_aca0 * -1.0;
    local_ac3c = local_ac9c * -1.0;
    local_ac38 = local_ac98 * -1.0;
    local_ac74[0] = iVar11;
    object_apply_damage(local_ac74,*param_1,0xffffffff,0xffffffff,0xffffffff,0);
  }
  return 1;
}
#endif
