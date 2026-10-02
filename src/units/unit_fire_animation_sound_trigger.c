// unit_fire_animation_sound_trigger  (Ghidra: unit_fire_animation_sound_trigger)
// address 0x560590, size 153 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.9 (VERIFIED against objdump 0x560590..0x560628)
// evidence: types/units.h k_unit_object_size chain; types/tags.h Biped.footsteps (TagDependency
//   at 0x38c, tag_id at +0xc = object 0x398) and Biped.contact_point (TagReflexive at 0x4e8,
//   count at +0x0). object.definition_tag (0x000) resolves through tag_instances to the Biped
//   tag record itself (not the animation graph), which is what puts footsteps/contact_point in
//   reach at these offsets.
// register convention: the unit/object index is not a parameter of this function at all --
//   Ghidra shows it as unaff_EBX, i.e. the caller (unit_fire_footstep_and_idle_triggers,
//   0x560410) leaves its own object index sitting in EBX and this callee reads it straight out
//   of that register without it ever being pushed or moved. Modelled here as an explicit
//   parameter passed by the caller, with a comment at the call site.
//   // blam-cc: unaff_EBX -> unit_index; stack param_1 -> trigger_kind (unused by the body, kept
//   //   for call-site parity); stack param_2 -> contact_point_index
// UNSURE: object_get_node_local_transform is cdecl with 4 stack arguments, but Ghidra recovered
//   none of them here -- the callee reads them off a stack frame this decompile does not show.
//   The marker name and output buffer are guessed as the contact-point index turned into a
//   marker lookup and a scratch local; flags is 1 to match every other caller of this function
//   in the codebase. FUN_00453330 and FUN_004533b0 are unresolved leaf helpers (no strings, no
//   further callees) -- likely a "should this object make sound" gate and a "play sound effect
//   at tag" call respectively, kept under their Ghidra names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void effect_marker_environment_probe(uint32_t definition_index, int16_t location_index,
    real_point3d *marker_position, uint32_t sound_param); // 0x4533b0, blam-cc: stack, ESI, EAX, stack
extern uint8_t any_local_player_within_10_units(const real_point3d *query_point); // 0x453330, EDX
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t maximum); // 0x4f6080

// REWRITTEN (objdump 0x560590..0x560628; the draft passed no point to the proximity test and no marker name).
// EBX = the unit, [esp+4] = the trigger kind, [esp+8] = the contact point. A contact point below the biped tag's
// count (+0x4e8) with a footsteps tag (+0x398) set, near a local player (any_local_player_within_10_units, EDX =
// the unit's centre +0xa0), finds its marker (contact point +0x20 name, 0x40 each at +0x4ec; one marker) and
// probes the environment at its position (node_transform +0x60): effect_marker_environment_probe(ESI = the
// trigger kind, EAX = &position, stack: footsteps, 0).
void unit_fire_animation_sound_trigger(uint32_t unit_index, uint32_t trigger_kind, int16_t contact_point_index)
{
    uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 0xc + 8);
    uint8_t *biped_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
    object_marker marker;

    if ((int32_t)contact_point_index >= *(int32_t *)(biped_tag + 0x4e8) ||
        *(datum_index *)(biped_tag + 0x398) == k_datum_index_none) {
        return;
    }
    if (!any_local_player_within_10_units((real_point3d *)(unit + 0xa0))) {
        return;
    }
    if ((int16_t)object_get_node_local_transform(unit_index,
            (char *)(*(uint8_t **)(biped_tag + 0x4ec) + contact_point_index * 0x40 + 0x20), &marker, 1) == 0) {
        return;
    }
    effect_marker_environment_probe(*(datum_index *)(biped_tag + 0x398), (int16_t)trigger_kind,
        (real_point3d *)((uint8_t *)&marker + 0x60), 0);
}

#if 0
Original Ghidra decompilation (0x560590):

void FUN_00560590(undefined4 param_1,short param_2)

{
  int iVar1;
  char cVar2;
  short sVar3;
  uint unaff_EBX;

  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((int)param_2 < *(int *)(iVar1 + 0x4e8)) && (*(int *)(iVar1 + 0x398) != -1)) {
    cVar2 = FUN_00453330();
    if (cVar2 != '\0') {
      sVar3 = object_get_node_local_transform();
      if (sVar3 != 0) {
        FUN_004533b0(*(undefined4 *)(iVar1 + 0x398),0);
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
