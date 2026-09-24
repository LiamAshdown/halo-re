// object_render_state_refresh  (Ghidra: FUN_0050f270; renamed per out/phase4/render_types_notes.md's
// misattributed-functions table: "The object render state cache, not a shadow cache. Probable
// names: object_get_cached_render_state, object_render_state_refresh,
// object_get_cached_render_lighting")
// address 0x50f270, size 683 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/render.h cached_object_render_state's own doc: "object_render_state_refresh
//   0x50f270 resamples desired_lighting, steps lighting toward it at 0.03 per call (0.0015 for
//   the shadow vector) or copies it whole (0x1d dwords), and stamps the counters", and its own
//   parameter order confirmed by object_get_cached_render_state.c's call site
//   (cache_index, object_index, level_of_detail_pixels, full_sample). The eight smoothing calls'
//   fields (ambient_color, reflection_tint, the two distant lights' color/direction,
//   shadow_vector at 0.0015, shadow_color) are pinned by
//   src/render/render_lighting_step_vector4_toward.c and
//   src/render/render_lighting_step_direction_toward.c's own disassembly-sourced notes for this
//   exact function's call sites. object.flags bit 0x4000 is not documented anywhere in
//   types/objects.h; kept as a raw mask.
// review fix (phase-4 gate, objdump 0x50f270..0x50f51a): the light list (0x4f2430) is
//   regathered whenever a window has passed (0x50f364), not only on a full sample; the
//   shadow vector step is 0.012 (0x3c449ba6), not 0.0015. The control flow is restructured to
//   match the branch targets 0x50f32c / 0x50f364 / 0x50f385 / 0x50f3c3 / 0x50f4f2.
// register convention: stack = (cache_index, object_index, level_of_detail_pixels, full_sample).
//   // blam-cc: stack=(cache_index, object_index, level_of_detail_pixels, full_sample)
// UNSURE: object.flags bit 0x4000 (gates whether staleness can trigger an automatic full
//   resample at all); render_lighting_smoothing_enabled (0x00689480, a debug toggle: when clear,
//   a stale entry is fully overwritten instead of smoothed); and the exact purpose of gating the
//   smoothing step on the object's root velocity being nonzero or its type matching mask 0x80.
// UNSURE: this rewrite uses goto to preserve the decompiled control flow exactly -- the "force a
//   full resample" code is reached from two different conditions (an automatic staleness check
//   gated by object.flags bit 0x4000, and an unconditional check of the caller's own full_sample
//   argument) that do not nest cleanly as structured if/else.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "rasterizer.h"
#include "render.h"

extern data_array *cached_object_render_states; // 0x007c30ec, this module
extern data_array *object_data;                 // 0x008603b0, objects module
extern int32_t render_frame_index;              // 0x007c3100, this module
extern int32_t render_window_count;             // 0x007c3104, this module
extern uint8_t render_lighting_smoothing_enabled; // 0x00689480, UNSURE name (debug toggle)

extern void object_sample_ambient_lighting(datum_index object_index, render_lighting *out); // 0x4f20b0,
    // foreign; blam-cc: EAX=object_index, stack=out
extern void object_gather_light_list(datum_index object_index, render_lighting *out); // 0x4f2430,
    // foreign; blam-cc: EAX=object_index, stack=out
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
                                              real_vector3d *out_angular_velocity); // 0x4f6aa0,
    // objects module; blam-cc: EAX=object_index, ESI=out_velocity, EDI=out_angular_velocity
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0,
    // objects module; blam-cc: ECX=object_index, stack=type_mask

extern void render_lighting_step_vector3_toward(float *current, float *target, float max_delta); // 0x50f520,
    // this module; blam-cc: ECX=current, EDX=target, stack=max_delta
extern void render_lighting_step_vector4_toward(float *current, float *target, float max_delta); // 0x50f5c0,
    // this module; blam-cc: ECX=current, EDX=target, stack=max_delta
extern void render_lighting_step_direction_toward(real_vector3d *current, real_vector3d *target,
                                                  float max_delta); // 0x50f690, this module;
    // blam-cc: ECX=current, EDX=target, stack=max_delta

// Refreshes one cached object render-state entry: a forced (full_sample) or overdue entry
// resamples desired_lighting from the object's surroundings; the light list is regathered for a
// full sample and whenever a window has been drawn since the last refresh; a full sample then
// replaces the smoothed lighting outright. An entry that is not overdue only gets its point
// light list copied (when a window has passed); an overdue one is stepped toward
// desired_lighting (0.03 per call, 0.012 for the shadow vector) when
// render_lighting_smoothing_enabled is set and the object is moving or is a type 7 object
// (object_try_and_get mask 0x80), and overwritten with it otherwise.
void object_render_state_refresh(datum_index cache_index, datum_index object_index,
                                 real level_of_detail_pixels,
                                 uint8_t full_sample) // blam-cc: stack=(cache_index, object_index, level_of_detail_pixels, full_sample)
{
    cached_object_render_state *entry =
        &((cached_object_render_state *)cached_object_render_states->data)[(uint16_t)cache_index];
    object *obj = ((object_header *)object_data->data)[(uint16_t)object_index].data;
    int32_t windows_elapsed = render_window_count - entry->last_update_window;
    int32_t frames_since_update = render_frame_index - entry->last_update_frame;
    int32_t frames_since_sample = render_frame_index - entry->last_sample_frame;
    uint8_t overdue = 0;

    if (frames_since_sample < 0 || windows_elapsed < 0) {
        frames_since_sample = 1;
        windows_elapsed = 1;
    }

    if ((obj->flags & 0x4000) != 0) { // UNSURE: undocumented object_flags bit
        int32_t staleness_threshold;

        if (level_of_detail_pixels > 400.0f) {
            staleness_threshold = 0;
        } else if (level_of_detail_pixels > 100.0f) {
            staleness_threshold = 3;
        } else {
            staleness_threshold = 10;
        }
        overdue = (uint8_t)(frames_since_sample > staleness_threshold);
        if (overdue && frames_since_update > 1) {
            full_sample = 1;
        }
    }

    if (full_sample || overdue) {
        entry->object_index = object_index;
        object_sample_ambient_lighting(object_index, &entry->desired_lighting);
        entry->level_of_detail_pixels = level_of_detail_pixels;
        entry->last_sample_frame = render_frame_index;
    }

    if (full_sample || windows_elapsed > 0) {
        object_gather_light_list(object_index, &entry->desired_lighting);
        if (full_sample) {
            entry->lighting = entry->desired_lighting;
            entry->last_update_window = render_window_count;
            entry->last_update_frame = render_frame_index;
            return;
        }
    }

    if (!overdue) {
        if (windows_elapsed > 0) {
            entry->lighting.point_light_count = entry->desired_lighting.point_light_count;
            entry->lighting.point_light_indices[0] = entry->desired_lighting.point_light_indices[0];
            entry->lighting.point_light_indices[1] = entry->desired_lighting.point_light_indices[1];
        }
        entry->last_update_window = render_window_count;
        entry->last_update_frame = render_frame_index;
        return;
    }

    if (render_lighting_smoothing_enabled == 0) {
        entry->lighting = entry->desired_lighting;
        entry->last_update_window = render_window_count;
        entry->last_update_frame = render_frame_index;
        return;
    }

    {
        real_vector3d root_velocity;

        object_get_root_object_velocities(object_index, &root_velocity, 0);
        if (root_velocity.i != 0.0f || root_velocity.j != 0.0f || root_velocity.k != 0.0f ||
            object_try_and_get(object_index, 0x80) != 0) {
            render_lighting_step_vector3_toward(&entry->lighting.ambient_color.red,
                                                &entry->desired_lighting.ambient_color.red, 0.03f);
            render_lighting_step_vector4_toward(&entry->lighting.reflection_tint.alpha,
                                                &entry->desired_lighting.reflection_tint.alpha, 0.03f);
            render_lighting_step_vector3_toward(&entry->lighting.distant_lights[0].color.red,
                                                &entry->desired_lighting.distant_lights[0].color.red, 0.03f);
            render_lighting_step_direction_toward(&entry->lighting.distant_lights[0].direction,
                                                  &entry->desired_lighting.distant_lights[0].direction, 0.03f);
            render_lighting_step_vector3_toward(&entry->lighting.distant_lights[1].color.red,
                                                &entry->desired_lighting.distant_lights[1].color.red, 0.03f);
            render_lighting_step_direction_toward(&entry->lighting.distant_lights[1].direction,
                                                  &entry->desired_lighting.distant_lights[1].direction, 0.03f);
            render_lighting_step_direction_toward(&entry->lighting.shadow_vector,
                                                  &entry->desired_lighting.shadow_vector, 0.012f);
            render_lighting_step_vector3_toward(&entry->lighting.shadow_color.red,
                                                &entry->desired_lighting.shadow_color.red, 0.03f);
        }
    }
    entry->lighting.point_light_count = entry->desired_lighting.point_light_count;
    entry->lighting.point_light_indices[0] = entry->desired_lighting.point_light_indices[0];
    entry->lighting.point_light_indices[1] = entry->desired_lighting.point_light_indices[1];
    entry->last_update_window = render_window_count;
    entry->last_update_frame = render_frame_index;
}

#if 0
Original Ghidra decompilation (0x50f270):

void FUN_0050f270(uint param_1,uint param_2,float param_3,char param_4)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  bool bVar8;
  bool bVar9;
  bool bVar10;
  float local_c;
  float local_8;
  float local_4;

  iVar2 = *(int *)(DAT_007c30ec + 0x34);
  iVar3 = (param_1 & 0xffff) * 0x100;
  iVar4 = iVar3 + iVar2;
  iVar5 = DAT_007c3104 - *(int *)(iVar3 + 0xc + iVar2);
  iVar2 = DAT_007c3100 - *(int *)(iVar3 + 8 + iVar2);
  bVar1 = false;
  if ((iVar2 < 0) || (iVar5 < 0)) {
    iVar2 = 1;
    iVar5 = 1;
  }
  if ((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc) + 0x10) &
      0x4000) == 0) {
LAB_0050f320:
    if ((param_4 != '\0') || (bVar1)) goto LAB_0050f32c;
LAB_0050f364:
    if (0 < iVar5) goto LAB_0050f368;
  }
  else {
    if (param_3 <= 400.0) {
      if (param_3 <= 100.0) {
        bVar10 = SBORROW4(iVar2,10);
        bVar9 = iVar2 + -10 < 0;
        bVar8 = iVar2 == 10;
      }
      else {
        bVar10 = SBORROW4(iVar2,3);
        bVar9 = iVar2 + -3 < 0;
        bVar8 = iVar2 == 3;
      }
    }
    else {
      bVar10 = false;
      bVar9 = iVar2 < 0;
      bVar8 = iVar2 == 0;
    }
    bVar1 = !bVar8 && bVar10 == bVar9;
    if ((bVar8 || bVar10 != bVar9) || (DAT_007c3100 - *(int *)(iVar4 + 0x10) < 2))
    goto LAB_0050f320;
    param_4 = '\x01';
LAB_0050f32c:
    *(uint *)(iVar4 + 4) = param_2;
    object_sample_ambient_lighting(iVar4 + 0x88);
    *(float *)(iVar4 + 0xfc) = param_3;
    *(int *)(iVar4 + 8) = DAT_007c3100;
    if (param_4 == '\0') goto LAB_0050f364;
LAB_0050f368:
    object_gather_light_list(iVar4 + 0x88);
    if (param_4 != '\0') goto LAB_0050f4f2;
  }
  if (!bVar1) {
    if (0 < iVar5) {
      *(undefined2 *)(iVar4 + 0x54) = *(undefined2 *)(iVar4 + 200);
      *(undefined4 *)(iVar4 + 0x58) = *(undefined4 *)(iVar4 + 0xcc);
      *(undefined4 *)(iVar4 + 0x5c) = *(undefined4 *)(iVar4 + 0xd0);
    }
    *(int *)(iVar4 + 0xc) = DAT_007c3104;
    *(int *)(iVar4 + 0x10) = DAT_007c3100;
    return;
  }
  if (DAT_00689480 != '\0') {
    FUN_004f6aa0();
    if ((((local_c != 0.0) || (local_8 != 0.0)) || (local_4 != 0.0)) ||
       (iVar2 = object_try_and_get(0x80), iVar2 != 0)) {
      FUN_0050f520(0x3cf5c28f);
      FUN_0050f5c0(0x3cf5c28f);
      FUN_0050f520(0x3cf5c28f);
      FUN_0050f690(0x3cf5c28f);
      FUN_0050f520(0x3cf5c28f);
      FUN_0050f690(0x3cf5c28f);
      FUN_0050f690(0x3c449ba6);
      FUN_0050f520(0x3cf5c28f);
    }
    *(undefined2 *)(iVar4 + 0x54) = *(undefined2 *)(iVar4 + 200);
    *(undefined4 *)(iVar4 + 0x58) = *(undefined4 *)(iVar4 + 0xcc);
    *(undefined4 *)(iVar4 + 0x5c) = *(undefined4 *)(iVar4 + 0xd0);
    *(int *)(iVar4 + 0xc) = DAT_007c3104;
    *(int *)(iVar4 + 0x10) = DAT_007c3100;
    return;
  }
LAB_0050f4f2:
  puVar6 = (undefined4 *)(iVar4 + 0x88);
  puVar7 = (undefined4 *)(iVar4 + 0x14);
  for (iVar2 = 0x1d; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar7 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar7 = puVar7 + 1;
  }
  *(int *)(iVar4 + 0xc) = DAT_007c3104;
  *(int *)(iVar4 + 0x10) = DAT_007c3100;
  return;
}
#endif
