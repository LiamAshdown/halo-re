// unit_reset_light_effect  (Ghidra: FUN_0056ec10; renamed from the phase2 proposal)
// address 0x56ec10, size 72 bytes
// name confidence: 0.25 (phase2 proposal "unit_reset_light_effect" at 0.25, kept for lack of a
//   better candidate)
// rewrite confidence: 0.9 (VERIFIED against objdump 0x56ec10..0x56ec57)
// evidence: callee sound_start_at_object_marker established elsewhere in this module (0x543ce0, see
//   src/units/unit_update_animation_timers.c) as a "set effect/sound intensity" helper; the
//   (effect, 0, 1.0, 0) argument pattern matches the "trigger at full intensity" call sites in
//   0x56f210 and 0x574f30.
// register convention: an animation_state pointer in ECX; the return value is whatever
//   animation_state_advance returns.
//   // blam-cc: ECX -> state, EAX -> animation_graph_tag_index
// FIXED (register inputs, objdump): EAX (read at 0x56ec1c, the call to animation_state_advance)
// was completely undeclared, and the previously-declared ECX parameter ("effect_index") was
// mislabeled. Re-checked against animation_state_advance.c's and sound_start_at_object_marker.c's
// own already-recovered signatures (both established convention: EAX -> animation_graph_tag_index,
// ESI -> state, EBX -> sound_tag_id, stack -> random_stream / and ESI -> object_index, ECX ->
// position, EAX -> forward, stack -> definition_index, node_index, scale, first_person_hint):
// ECX (this function's own incoming register) is `mov esi,ecx`'d into animation_state_advance's
// state argument, i.e. it is an animation_state*, not an effect/datum index. The old
// "effect_index != k_datum_index_none" guard was reading the WRONG thing too -- objdump
// reloads and compares the sound_tag_id that animation_state_advance itself just wrote (via a
// local out-param, EBX), not this function's own ECX register. The forwarded value to
// sound_start_at_object_marker is that sound_tag_id, together with a stack parameter
// (object_index, at offset 0x4 from this function's own entry esp, loaded into ESI right
// before that second call) that was previously missing from the signature entirely, and two
// already-named globals (global_zero_vector3d_pointer, global_forward3d_pointer) that the previous
// rewrite guessed as a bare NULL position/no forward vector at all.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "models.h"
#include "objects.h"

extern void *global_zero_vector3d_pointer;               // 0x006966f8, types/devices.h
extern real_vector3d *global_forward3d_pointer;   // 0x00696718, types/math.h

extern animation_state_advance_result animation_state_advance(uint32_t animation_graph_tag_index,
    animation_state *state, int32_t *sound_tag_id, animation_random_stream random_stream); // 0x4d48d0, src/models/animation_state_advance.c
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward,
    datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint); // 0x543ce0, src/sound/sound_start_at_object_marker.c

// Advances *state by one frame (animation_state_advance) and, if that frame starts a sound,
// plays it attached to object_index's marker (node 0) via sound_start_at_object_marker, at the
// fixed creation origin/forward and full volume.
uint16_t unit_reset_light_effect(animation_state *state, uint32_t animation_graph_tag_index, datum_index object_index)
{
    int32_t sound_tag_id;
    uint16_t result = animation_state_advance(animation_graph_tag_index, state, &sound_tag_id, _animation_random_global);

    if (sound_tag_id != -1) {
        sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer,
            (Vector3D *)global_forward3d_pointer, (datum_index)sound_tag_id, 0, 1.0f, 0);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x56ec10):

undefined2 FUN_0056ec10(void)

{
  undefined2 uVar1;
  int in_ECX;

  uVar1 = FUN_004d48d0(1);
  if (in_ECX != -1) {
    FUN_00543ce0(in_ECX,0,0x3f800000,0);
  }
  return uVar1;
}
#endif
