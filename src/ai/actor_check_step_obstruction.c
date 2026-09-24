// actor_check_step_obstruction  (Ghidra: actor_check_step_obstruction, renamed)
// address 0x417bb0, size 661 bytes
// name confidence: 0.25  rewrite confidence: 0.25 (best-effort; several callee signatures
// guessed from the visible stack args alone, no disassembly cross-check on this file)
// evidence: only reachable when actor.flying is clear; projects a step point along a
// caller-supplied 2D direction from body_position, runs a trace (path_find_test_segment_unobstructed) against it,
// and, when either the trace fails or a positive step_up is requested, runs up to two more
// point-clearance checks (collision_bsp_query_segment_init) around the actor's aim_origin/body_position midpoint
// and a step further along the global "down" axis. Returns whether the step is obstructed;
// optionally reports through *out_flag whether the second (point-clearance) path is what
// produced the result.
// register convention: actor_index in EAX (Ghidra's in_EAX); the 2D direction vector is EDI
// (Ghidra's unaff_EDI, never set up in this function's own body -- inherited unmodified from
// whatever the caller left there); step_distance, step_up, out_flag and a fourth pointer
// parameter are genuine stack parameters.
// blam-cc: EAX -> actor_index, EDI -> direction, stack -> step_distance, stack -> step_up,
//   stack -> out_flag, stack -> extra_param
// UNSURE: path_find_test_segment_unobstructed and collision_bsp_query_segment_init are trace/point-clearance utilities outside this
// module, not rewritten here; their parameter types are guesses from Ghidra's own untyped
// call sites (undefined4/undefined1), not independently confirmed.
// UNSURE: PTR_DAT_0069672c (used as a 3-float direction, by the 0x696714/0x696718/0x69671c/
// 0x696720 pattern this module already reads as "zero"/"forward"/"right"/"up") would be
// "down" by that same pattern; not confirmed here.
// reconciled: R54 0x502060 prototype -> physics signature collision_bsp_query_segment_init(flags EAX = 3, result ECX, bsp, 0, 0, origin, delta, FLT_MAX); both calls now pass the flags and a result buffer

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include "physics.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern const real_vector3d *global_down3d_pointer; // 0x0069672c, UNSURE: see file header

extern void actor_update_target_lead_position(void); // 0x429570, not this module, UNSURE: no visible args at this call site
extern uint8_t path_find_test_segment_unobstructed(uint8_t ignores_glass, int32_t param2, real_point3d *point, uint32_t ignore,
                             uint32_t param5, uint8_t param6, void *param7); // 0x43de90, not this module, UNSURE signature
extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
                                                ModelCollisionGeometryBSP *bsp,
                                                int16_t breakable_surface_count,
                                                uint32_t *breakable_surfaces, real_point3d *origin,
                                                real_vector3d *delta, float max_fraction);
    // 0x502060, src/physics/collision_bsp_query_segment_init.c; flags in EAX, result in ECX

extern int32_t global_structure_collision_bsp; // 0x00746f98, UNSURE: a global handle collision_bsp_query_segment_init reads for its first call and reuses for the second

// blam-cc: EAX -> actor_index, EDI -> direction, stack -> step_distance, stack -> step_up,
//   stack -> out_flag, stack -> extra_param
uint8_t actor_check_step_obstruction(datum_index actor_index, real_vector2d *direction, float step_distance, float step_up,
                                      uint8_t *out_flag, void *extra_param)
{
    actor *self;
    Actor *definition;
    uint8_t obstructed = 0;
    uint8_t used_point_check = 0;
    real_point3d step_point;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;

    if (self->flying == 0) {
        uint8_t trace_ok;

        step_point.x = step_distance * direction->i + self->body_position.x;
        step_point.y = step_distance * direction->j + self->body_position.y;
        actor_update_target_lead_position();
        trace_ok = path_find_test_segment_unobstructed(self->ignores_glass, (uint32_t)self->unknown_164, &step_point, (uint32_t)-1,
                                 *(uint32_t *)&definition->pathfinding_radius, 0, extra_param);
        if (!trace_ok) {
            obstructed = 1;
            {
                // Ghidra: *(float*)(param_4 + 0xc) -- extra_param is a caller-owned record
                // whose .z-like field sits at +0xc (not this function's own step_point, which
                // is only ever written for .x/.y above).
                float dz = *(float *)((uint8_t *)extra_param + 0xc) - self->body_position.z;
                if (dz <= step_distance * 0.5f && (step_up != 0.0f || step_distance * -0.5f <= dz)) {
                    goto done;
                }
            }
        }
        obstructed = 0;
        if (0.0f < step_up) {
            real_point3d mid;
            real_vector3d scaled_dir;
            uint8_t clear1;
            collision_bsp_segment_result probe; // the stack buffer ECX points at

            mid.x = (self->aim_origin.x + self->body_position.x) * 0.5f;
            mid.y = (self->aim_origin.y + self->body_position.y) * 0.5f;
            mid.z = (self->aim_origin.z + self->body_position.z) * 0.5f;
            scaled_dir.i = step_distance * direction->i;
            scaled_dir.j = step_distance * direction->j;
            scaled_dir.k = 0.0f;

            clear1 = collision_bsp_query_segment_init(3, &probe, (ModelCollisionGeometryBSP *)global_structure_collision_bsp, 0, 0, &mid, &scaled_dir, 3.4028235e+38f); // 0x417d71
            if (!clear1) {
                obstructed = 1;
                used_point_check = 1;
                if (step_up < 3.4028235e+38f) {
                    real_point3d far_point;
                    real_vector3d down_step;
                    uint8_t clear2;

                    far_point.x = scaled_dir.i + mid.x;
                    far_point.y = scaled_dir.j + mid.y;
                    far_point.z = scaled_dir.k + mid.z;
                    down_step.i = step_up * global_down3d_pointer->i;
                    down_step.j = step_up * global_down3d_pointer->j;
                    down_step.k = step_up * global_down3d_pointer->k;

                    // FIXED: both point-clearance calls take global_structure_collision_bsp. Ghidra caches it in
                    // uVar3 before the first branch (`uVar3 = global_structure_collision_bsp;`) and hands that same
                    // uVar3 to the second call; the first rewrite substituted DAT_00746f9c.
                    clear2 = collision_bsp_query_segment_init(3, &probe, (ModelCollisionGeometryBSP *)global_structure_collision_bsp, 0, 0, &far_point, &down_step, 3.4028235e+38f); // 0x417e12
                    if (clear2) {
                        obstructed = 0;
                    }
                }
            }
        }
    }
done:
    if (out_flag != 0) {
        *out_flag = used_point_check;
    }
    return obstructed;
}

#if 0
Original Ghidra decompilation (0x417bb0):

undefined1 FUN_00417bb0(float param_1,float param_2,undefined1 *param_3,int param_4)

{
  int iVar1;
  float fVar2;
  undefined4 uVar3;
  char cVar4;
  uint in_EAX;
  int iVar5;
  float *unaff_EDI;
  undefined1 local_452;
  undefined1 local_451;
  float local_450;
  float local_44c;
  float local_448;
  float local_444;
  float local_440;
  float local_43c;
  float local_438;
  float local_434;
  float local_430;
  float local_42c;
  float local_428;
  float local_424;
  float local_420;
  float local_41c;

  iVar5 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar1 = *(int *)((*(uint *)(iVar5 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_452 = 0;
  local_451 = 0;
  if (*(char *)(iVar5 + 0x99) == '\0') {
    local_438 = param_1 * *unaff_EDI + *(float *)(iVar5 + 300);
    local_434 = param_1 * unaff_EDI[1] + *(float *)(iVar5 + 0x130);
    actor_update_target_lead_position();
    cVar4 = FUN_0043de90(*(undefined1 *)(iVar5 + 0x376),*(undefined4 *)(iVar5 + 0x164),&local_438,
                         0xffffffff,*(undefined4 *)(iVar1 + 0x8c),0,param_4);
    uVar3 = DAT_00746f98;
    if (cVar4 == '\0') {
      local_452 = 1;
      fVar2 = *(float *)(param_4 + 0xc) - *(float *)(iVar5 + 0x134);
      if ((fVar2 <= param_1 * 0.5) && ((param_2 != 0.0 || (param_1 * -0.5 <= fVar2))))
      goto LAB_00417e26;
    }
    local_452 = 0;
    if (0.0 < param_2) {
      local_450 = (*(float *)(iVar5 + 0x120) + *(float *)(iVar5 + 300)) * 0.5;
      local_44c = (*(float *)(iVar5 + 0x124) + *(float *)(iVar5 + 0x130)) * 0.5;
      local_43c = 0.0;
      local_448 = (*(float *)(iVar5 + 0x128) + *(float *)(iVar5 + 0x134)) * 0.5;
      local_444 = param_1 * *unaff_EDI;
      local_440 = param_1 * unaff_EDI[1];
      cVar4 = FUN_00502060(DAT_00746f98,0,0,&local_450,&local_444,0x7f7fffff);
      if (cVar4 == '\0') {
        local_452 = 1;
        local_451 = 1;
        if (param_2 < 3.4028235e+38) {
          local_424 = local_444 + local_450;
          local_420 = local_440 + local_44c;
          local_41c = local_448 + local_43c;
          local_430 = param_2 * *(float *)PTR_DAT_0069672c;
          local_42c = param_2 * *(float *)(PTR_DAT_0069672c + 4);
          local_428 = param_2 * *(float *)(PTR_DAT_0069672c + 8);
          cVar4 = FUN_00502060(uVar3,0,0,&local_424,&local_430,0x7f7fffff);
          if (cVar4 == '\0') {
            local_452 = 0;
          }
        }
      }
    }
  }
LAB_00417e26:
  fVar2 = *(float *)(param_4 + 0xc) - *(float *)(iVar5 + 0x134);
  if (param_3 != (undefined1 *)0x0) {
    *param_3 = local_451;
  }
  return local_452;
}
#endif
