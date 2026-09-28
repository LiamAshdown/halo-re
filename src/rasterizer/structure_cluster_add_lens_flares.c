// structure_cluster_add_lens_flares  (Ghidra: FUN_00513a00, unnamed; named per
// out/phase4/rasterizer_types_notes.md's lens flare misattribution table)
// address 0x513a00, size 408 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: walks ScenarioStructureBSPCluster lens flare markers (first index +0x40, count +0x42,
//   clusters at ScenarioStructureBSP +0x138 with stride 0x68) and turns each
//   ScenarioStructureBSPLensFlareMarker (lens_flare_markers +0x12c, 0x10 bytes) into a
//   lens_flare_instance candidate: definition = the LensFlare of
//   ScenarioStructureBSP.lens_flares[marker.lens_flare_index] (+0x120, 0x10 byte dependencies),
//   position = marker position, direction = marker direction components * (1/127), up =
//   vector3d_build_perpendicular(direction), both normalized and packed 11:11:10, color white,
//   object index -1, the 32 bit marker index split over visibility_high/visibility_low, window
//   flags = the current render window (0x007c310a) and intensity 0; then
//   lens_flare_add_instance (candidate in EBX).
//   Spot-check fix (phase 4 review): the earlier file left all five per marker calls without
//   arguments; rewritten from the raw code 0x513a00..0x513b97.
// register convention: CX = cluster index.
// reconciled: R12 0x007c310a render_window_index extern uint8 -> int16 (render.h); this reader keeps only the low byte

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"

extern uint8_t unknown_006893ff;                                    // 0x006893ff UNSURE: lens flares enable toggle
extern int16_t unknown_00719aac;                                    // 0x00719aac UNSURE: at most 1 allowed
extern int16_t screenshot_scale;                                    // 0x00696568 UNSURE: at most 1 when the above is 1
extern ScenarioStructureBSP *global_structure_bsp;                         // 0x00746f9c
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern int16_t render_window_index;                                 // 0x007c310a render module (int16, render.h)

// blam-cc: ECX -> out, EDX -> dir
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir); // 0x4cd670
// blam-cc: ECX -> v
extern real vector3d_normalize_with_length(real_vector3d *v);       // 0x401990
// blam-cc: ESI -> direction
extern uint32_t vector3d_pack_normal_11_11_10(real_vector3d *direction); // 0x5132d0
// blam-cc: EBX -> candidate
extern void lens_flare_add_instance(lens_flare_instance *candidate); // 0x5138a0

// blam-cc: CX -> cluster_index
void structure_cluster_add_lens_flares(int16_t cluster_index)
{
    ScenarioStructureBSP *bsp;
    const uint8_t *cluster;
    uint32_t marker_ordinal;

    if (unknown_006893ff == 0 || unknown_00719aac > 1 || (unknown_00719aac == 1 && screenshot_scale > 1)) {
        return;
    }

    bsp = global_structure_bsp;
    cluster = (const uint8_t *)((struct ScenarioStructureBSP *)bsp)->clusters.pointer + cluster_index * 0x68;
    for (marker_ordinal = 0; marker_ordinal < *(const uint16_t *)(cluster + 0x42); marker_ordinal++) {
        uint32_t marker_index = *(const uint16_t *)(cluster + 0x40) + marker_ordinal;
        const ScenarioStructureBSPLensFlareMarker *marker =
            (const ScenarioStructureBSPLensFlareMarker *)((struct ScenarioStructureBSP *)bsp)->lens_flare_markers.pointer + marker_index;
        const uint8_t *palette = (const uint8_t *)((struct ScenarioStructureBSP *)bsp)->lens_flares.pointer + marker->lens_flare_index * 0x10;
        real_vector3d direction;
        real_vector3d up;
        lens_flare_instance candidate;

        direction.i = (float)marker->direction_i_component * (1.0f / 127.0f);
        direction.j = (float)marker->direction_j_component * (1.0f / 127.0f);
        direction.k = (float)marker->direction_k_component * (1.0f / 127.0f);
        vector3d_build_perpendicular(&up, &direction);
        vector3d_normalize_with_length(&direction);
        vector3d_normalize_with_length(&up);

        candidate.packed_direction = vector3d_pack_normal_11_11_10(&direction);
        candidate.packed_up = vector3d_pack_normal_11_11_10(&up);
        candidate.definition = (uint32_t)tag_instances[*(const uint32_t *)(palette + 0xc) & 0xffff].data;
        candidate.position.x = marker->position.x;
        candidate.position.y = marker->position.y;
        candidate.position.z = marker->position.z;
        candidate.color = 0xffffffff;
        candidate.object_index = -1;
        candidate.visibility_high = (int16_t)((int32_t)marker_index >> 16);
        candidate.visibility_low = (int16_t)marker_index;
        candidate.window_flags = (uint8_t)render_window_index; // low byte only (mov cl,BYTE PTR ds:0x7c310a at 0x513b4e)
        candidate.intensity = 0;
        lens_flare_add_instance(&candidate);                    // sample_count left unset, as in the original
    }
}

#if 0
Original Ghidra decompilation (0x513a00):

void FUN_00513a00(void)

{
  short in_CX;
  int iVar1;
  int local_50;

  if (((DAT_006893ff != '\0') && (DAT_00719aac < 2)) &&
     ((DAT_00719aac != 1 || ((short)DAT_00696568 < 2)))) {
    iVar1 = in_CX * 0x68 + *(int *)(DAT_00746f9c + 0x138);
    local_50 = 0;
    if (*(short *)(iVar1 + 0x42) != 0) {
      do {
        vector3d_build_perpendicular();
        vector3d_normalize_with_length();
        vector3d_normalize_with_length();
        FUN_005132d0();
        FUN_005132d0();
        decal_add_to_active_list();
        local_50 = local_50 + 1;
      } while (local_50 < (int)(uint)*(ushort *)(iVar1 + 0x42));
    }
  }
  return;
}
#endif
