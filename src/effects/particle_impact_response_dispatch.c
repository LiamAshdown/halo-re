// particle_impact_response_dispatch  (Ghidra: FUN_004565a0, still unnamed there; named from its
//   own summary in out/phase4/effects_functions.md: "Dispatches a particle's impact response,
//   spawning an effect or playing a sound depending on the referenced tag's group")
// address 0x4565a0, size 326 bytes
// name confidence: 0.5   rewrite confidence: 0.15 (LOW -- see UNSURE)
// evidence: types/cache.h tag_instance.group_tag; src/items/weapon_play_trigger_tag_effect.c and
//   src/devices/device_play_state_change_effect.c establish the same 0x65666665 ('effe') /
//   0x736e6421 ('snd!') fourcc dispatch idiom against a tag_instance.group_tag.
// register convention: tag group fourcc in ECX (in_ECX); an intensity/magnitude value as the
//   recognized stack parameter (param_1) -- particle_update_motion 0x4561a0 passes a 0..1
//   collision speed fraction here, particle_impact 0x456550 passes a literal 0.
//   // blam-cc: in_ECX -> fourcc, stack -> intensity
// UNSURE (heavily): Ghidra drops every other argument to both callees, including whatever
//   position/bundle data they need -- neither call site shows anything besides the fourcc and
//   this one float. `vector3d_normalize_with_length` is called with no visible operand at all
//   (kept as a no-op placeholder call site is not possible in C, so it is omitted here rather
//   than fabricated with a wrong vector), and effect_new_with_color 0x450980 -- a 10 parameter
//   function -- is called with zero visible arguments; this rewrite calls it with every
//   argument zeroed except the two scale factors (set from `intensity`), which is a guess, not
//   evidence. sound_start_at_location's bundle pointer is likewise unrecoverable here and passed as NULL.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern datum_index effect_new_with_color(datum_index definition_index, datum_index creator_object_index,
    const real_vector3d *velocity, uint16_t unknown_a, uint32_t ctx_c, uint32_t ctx_10,
    real_point3d *position, real a_scale, real b_scale, uint8_t force_create); // 0x450980, this
                                    // module; UNSURE call, see file header
extern void sound_start_at_location(void *bundle, uint32_t param_2); // 0x543d80, sound module, out of range

// Plays the impact response for a particle's death effect/sound tag: an 'effe' tag spawns a
// free-standing effect scaled by `intensity`, a 'snd!' tag plays a sound. Any other group is
// ignored.
void particle_impact_response_dispatch(tag_group fourcc, real intensity)
{
    if (fourcc == 0x65666665) { // 'effe' (effect), fourcc bytes reversed on x86
        effect_new_with_color((datum_index)0xffffffff, (datum_index)0xffffffff, (real_vector3d *)0,
            0, 0, 0, (real_point3d *)0, intensity, intensity, 0); // UNSURE, see file header
        return;
    }
    if (fourcc == 0x736e6421) { // 'snd!' (sound)
        sound_start_at_location((void *)0, *(uint32_t *)&intensity); // UNSURE, see file header
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
