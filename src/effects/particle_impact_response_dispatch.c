// particle_impact_response_dispatch  (Ghidra: FUN_004565a0, still unnamed there; named from its
//   own summary in out/phase4/effects_functions.md: "Dispatches a particle's impact response,
//   spawning an effect or playing a sound depending on the referenced tag's group")
// address 0x4565a0, size 326 bytes
// VERIFIED against disassembly 0x4565a0..0x4566e6 (2026-09-30). registers: EAX self, ECX fourcc, ESI
//   definition_index, stack intensity; effect_new_with_color gets its 12 stack args and
//   vector3d_normalize_with_length works on the local direction copy (0x456637)
// name confidence: 0.5   rewrite confidence: 0.85 (REWRITTEN from objdump 0x4565a0..0x4566e5)
// evidence: types/cache.h tag_instance.group_tag; src/items/weapon_play_trigger_tag_effect.c and
//   src/devices/device_play_state_change_effect.c establish the same 0x65666665 ('effe') /
//   0x736e6421 ('snd!') fourcc dispatch idiom against a tag_instance.group_tag.
// register convention: tag group fourcc in ECX (in_ECX); an intensity/magnitude value as the
//   recognized stack parameter (param_1) -- particle_update_motion 0x4561a0 passes a 0..1
//   collision speed fraction here, particle_impact 0x456550 passes a literal 0.
//   // blam-cc: in_ECX -> fourcc, stack -> intensity

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern datum_index effect_new_with_color(uint32_t definition_index, uint32_t creator, real_vector3d *velocity,
    int32_t count, char **names, real_point3d *points, real_vector3d *vectors, float a_scale, float b_scale,
    int32_t color, int32_t tint, int32_t force); // 0x450980, this call site's shape
extern datum_index sound_start_at_location(datum_index definition_index, sound_placement *placement, float scale);
    // 0x543d80, EDX definition_index, EAX placement, stack scale
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern const real_vector3d *global_down3d_pointer;    // 0x0069672c
extern const real_vector3d *global_forward3d_pointer; // 0x00696718
extern char *particle_impact_vector_names[2];         // 0x00687018: "velocity", "gravity"

// REWRITTEN from objdump. The particle's velocity (+0x48) is scaled by 1/30 (per tick). An 'effe' tag spawns
//   effect_new_with_color(tag, -1, &scaled velocity, 2, {"velocity", "gravity"}, {position, position},
//   {normalize(+0x3c), down}, intensity, 0, 0, 0, 0). A 'snd!' tag plays at {position, forward, scaled
//   velocity, the particle's leaf/cluster} scaled by intensity. The draft passed neither the particle nor the
//   tag index, so the effect had no definition and no position, and the sound had no placement.
// blam-cc: EAX -> self, ECX -> fourcc, ESI -> definition_index, stack -> intensity
void particle_impact_response_dispatch(particle *self, tag_group fourcc, datum_index definition_index,
    real intensity)
{
    real_vector3d velocity;

    velocity.i = self->velocity.i * 0.033333335f;
    velocity.j = self->velocity.j * 0.033333335f;
    velocity.k = self->velocity.k * 0.033333335f;
    if (fourcc == 0x65666665) { // 'effe'
        real_point3d points[2];
        real_vector3d vectors[2];

        points[0] = self->position;
        points[1] = self->position;
        vectors[0] = self->direction;
        vectors[1] = *global_down3d_pointer;
        vector3d_normalize_with_length(&vectors[0]);
        effect_new_with_color(definition_index, 0xffffffff, &velocity, 2, particle_impact_vector_names, points,
            vectors, intensity, 0.0f, 0, 0, 0);
    } else if (fourcc == 0x736e6421) { // 'snd!'
        sound_placement placement;

        placement.position = *(Point3D *)&self->position;
        placement.forward = *(Vector3D *)global_forward3d_pointer;
        placement.velocity = *(Vector3D *)&velocity;
        *(bsp_leaf_reference *)&placement.leaf_index = self->location;
        sound_start_at_location(definition_index, &placement, intensity);
    }
}

#if 0
Original Ghidra decompilation (0x4565a0):

void FUN_004565a0(undefined4 param_1)

{
  int in_ECX;

  if (in_ECX == 0x65666665) {
    vector3d_normalize_with_length();
    FUN_00450980();
    return;
  }
  if (in_ECX == 0x736e6421) {
    FUN_00543d80(param_1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
