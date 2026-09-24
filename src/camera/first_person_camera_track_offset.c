// first_person_camera_track_offset  (Ghidra: FUN_00447190; renamed for this rewrite)
// address 0x447190, size 247 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/camera_types_notes.md calls this "the first person camera track offset
// 0x447190" (the only caller of vector3d_catmull_rom_interpolate besides that helper itself).
// ECX's +0x4c/+0x50 accesses match unit_camera_properties.camera_tracks (types/camera.h) field
// for field; the CameraTrack / CameraTrackControlPoint / UnitCameraTrack layouts and the
// GlobalsCamera.default_unit_camera_track fallback (types/tags.h) match exactly. Confirmed
// instruction-by-instruction against objdump for the stack shuffling __ftol and the final call's
// argument order (matches vector3d_catmull_rom_interpolate's stack/register split exactly).
// register convention: unit_camera_properties * in ECX (in_ECX); angle and out on the stack
// (Ghidra's recognized param_1, param_2).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "camera.h"

extern tag_instance *tag_instances;  // 0x0087bc14
extern Globals *global_globals;      // 0x00746fa0

// blam-cc: source1 = EBX, source3 = ESI, source2 = EDI; out, source0, time0, dt, time = stack
extern void vector3d_catmull_rom_interpolate(Vector3D *source1, Vector3D *source3,
    Vector3D *source2, Vector3D *out, Vector3D *source0, float time0, float dt, float time);

// blam-cc: ECX -> properties; angle, out = stack (param_1, param_2)
// Picks a camera track (the unit's own if it has one and a valid track tag, otherwise
// GlobalsCamera.default_unit_camera_track), maps angle onto a position along the track's control
// points (angle == -pi/2 is the first point, +pi/2 the last), and writes the Catmull-Rom
// interpolated position at that point into *out.
void first_person_camera_track_offset(unit_camera_properties *properties, float angle,
    Vector3D *out)
{
    UnitCameraTrack *first_track;
    int32_t count_minus_one;
    int32_t clamped_index;
    datum_index track_tag;
    CameraTrack *track;
    int32_t control_point_count;
    float time;
    int16_t frame_guess;
    int16_t base_index;
    float dt;
    float time0;
    CameraTrackControlPoint *points;
    CameraTrackControlPoint *source0;
    CameraTrackControlPoint *source1;
    CameraTrackControlPoint *source2;
    CameraTrackControlPoint *source3;

    track_tag = (datum_index)k_datum_index_none;
    if (properties->camera_tracks.count != 0) {
        // Ghidra's rendering of min(count - 1, 0); always 0 here since count != 0 was just
        // ruled out above, but the clamp is preserved verbatim.
        count_minus_one = (int32_t)properties->camera_tracks.count - 1;
        clamped_index = (count_minus_one < 0) ? count_minus_one : 0;
        first_track = (UnitCameraTrack *)((uint8_t *)properties->camera_tracks.pointer +
            clamped_index * (int32_t)sizeof(UnitCameraTrack));
        if (first_track != 0) {
            track_tag = *(datum_index *)&first_track->track.tag_id;
        }
    }
    if (track_tag == (datum_index)k_datum_index_none) {
        track_tag = *(datum_index *)&((GlobalsCamera *)global_globals->camera.pointer)->
            default_unit_camera_track.tag_id;
    }

    track = (CameraTrack *)tag_instances[track_tag & 0xffff].data;
    control_point_count = (int32_t)track->control_points.count;

    // time in [0, 1]: angle == -pi/2 maps to 0, angle == +pi/2 maps to 1.
    time = (angle + 1.5707964f) * 0.31830987f;

    dt = 1.0f / (float)(control_point_count - 1);
    frame_guess = (int16_t)(time * (float)(control_point_count - 1));

    // Walk the guessed frame back until there is room for four consecutive control points
    // (base_index, +1, +2, +3), or until it hits 0. The first step back is unconditional.
    base_index = frame_guess;
    if (frame_guess > 0) {
        do {
            if (base_index + 4 <= control_point_count && base_index <= frame_guess - 1) {
                break;
            }
            base_index -= 1;
        } while (base_index > 0);
    }

    time0 = (float)base_index * dt;

    points = (CameraTrackControlPoint *)track->control_points.pointer;
    source0 = points + base_index;
    source1 = points + base_index + 1;
    source2 = points + base_index + 2;
    source3 = points + base_index + 3;

    vector3d_catmull_rom_interpolate((Vector3D *)source1, (Vector3D *)source3,
        (Vector3D *)source2, out, (Vector3D *)source0, time0, dt, time);
}

#if 0
Original Ghidra decompilation (0x447190):

void FUN_00447190(float param_1,undefined4 param_2)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  short sVar4;
  int in_ECX;
  float10 extraout_ST0;

  if (((*(int *)(in_ECX + 0x4c) == 0) ||
      (uVar2 = *(int *)(in_ECX + 0x4c) - 1,
      iVar3 = (uVar2 & (-1 < (int)uVar2) - 1) * 0x1c + *(int *)(in_ECX + 0x50), iVar3 == 0)) ||
     (uVar2 = *(uint *)(iVar3 + 0xc), uVar2 == 0xffffffff)) {
    uVar2 = *(uint *)(*(int *)(DAT_00746fa0 + 0x108) + 0xc);
  }
  iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar1 = __ftol();
  sVar4 = sVar1;
  if (0 < sVar1) {
    do {
      if ((sVar4 + 4 <= *(int *)(iVar3 + 4)) && ((int)sVar4 <= sVar1 + -1)) break;
      sVar4 = sVar4 + -1;
    } while (0 < sVar4);
  }
  vector3d_catmull_rom_interpolate
            (param_2,sVar4 * 0x3c + *(int *)(iVar3 + 8),
             (float)(int)sVar4 * (float)((float10)1.0 / extraout_ST0),
             (float)((float10)1.0 / extraout_ST0),(param_1 + 1.5707964) * 0.31830987);
  return;
}
#endif
