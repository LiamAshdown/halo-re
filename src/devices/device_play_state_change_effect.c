// device_play_state_change_effect  (Ghidra: device_play_state_change_effect, already named;
// functions.md: "Given a tag id (register ecx), dispatches to the effect-creation routine if it
// names an effect tag, or the sound-playback routine if it names a sound tag; used to play a
// device's state-change effect or sound")
// address 0x44c1a0, size 126 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: types/devices.h device_constants (k_device_state_change_tag_effect 0x65666665,
// k_device_state_change_tag_sound 0x736e6421); types/cache.h tag_instance (group_tag at +0x00).
// register convention: tag id in ECX (in_ECX), packed as a TagID {index;id} the way every
// caller in this batch passes it; object index in EAX.
//   // blam-cc: EAX -> object_index, ECX -> tag_id
// Fully resolved against disassembly (objdump -d -M intel bin/halo.exe, 0x44c1a0..0x44c21d) in
// the phase-4 review. Ghidra shows both creation calls as bare `FUN_00xxxxxx();`, but neither is
// argument-less:
//   - 0x44c1a0 `cmp ecx,0xffffffff` tests the WHOLE 32-bit tag id, not just its index half, and
//     0x44c1bd `movsx edx,cx` then sign-extends the low half before `shl edx,5`. Both are
//     reproduced literally below; an earlier revision tested only `tag_id.index != 0xffff`,
//     which rejects a {index 0xffff, id != 0xffff} pair the original would have accepted (and
//     then indexed tag_instances[-1] with).
//   - effect path (0x44c1fd): EAX = object_index, ECX = tag_id, and six stack arguments
//     (object_index, 0xffffffff, device_data.position at object+0x208, device_data.power at
//     object+0x1fc, 0, 0). The object record is fetched at 0x44c1a8..0x44c1b9 and is read only
//     here, which is why the power/position pair shows up as the effect's two scalars.
//   - sound path (0x44c1de): EAX = *(void **)0x00696718, ECX = *(void **)0x006966f8 (the two
//     .rdata constants devices.h documents at 0x0065c20c and 0x0065c230), and four stack
//     arguments (tag_id, 0xffffffff, 1.0f, 0).
// UNSURE: only the parameter NAMES of the two callees are guesses -- the values passed are
// exactly what the disassembly loads. Both callees are outside this module, so their real
// prototypes belong to whoever owns 0x4507a0 (effect) and 0x543ce0 (sound).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "devices.h"
#include "effects.h"
#include "fn_devices.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern void *global_forward3d_pointer; // 0x00696718, a constant pointer to .rdata 0x0065c20c
extern void *global_zero_vector3d_pointer;  // 0x006966f8, a constant pointer to .rdata 0x0065c230

// 0x4507a0, out of range (effect-creation routine).
//   // blam-cc: EAX = object_index, ECX = tag_id, the rest on the stack
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source); // EAX creator, ECX definition
// 0x543ce0, out of range (sound-playback routine).
//   // blam-cc: EAX = global_forward3d_pointer, ECX = global_zero_vector3d_pointer, the rest on the stack
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward,
    datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint); // ESI object, ECX position, EAX forward

void device_play_state_change_effect(uint32_t object_index, TagID tag_id)
{
    // 0x44c1a0 tests all 32 bits of ECX against -1, so both halves must be 0xffff to bail out.
    if (tag_id.index != 0xffff || tag_id.id != 0xffff) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        // 0x44c1bd sign-extends the low half before scaling by sizeof(tag_instance).
        uint32_t group_tag = tag_instances[(int16_t)tag_id.index].group_tag;

        if (group_tag == k_device_state_change_tag_effect) {
            device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
            // 0x44c1fd..0x44c214: EAX = object, ECX = tag_id (entry ECX), push object, -1, position (+0x208),
            // power (+0x1fc), 0, 0
            effect_new_on_object(object_index, *(datum_index *)&tag_id, object_index, -1, dev->position, dev->power,
                (const ColorRGB *)0, (const effect_tint_source *)0);
        } else if (group_tag == k_device_state_change_tag_sound) {
            // 0x44c1de..0x44c1f3: ESI = object, ECX = [0x6966f8], EAX = [0x696718], push tag, -1, 1.0f, 0
            sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer, (Vector3D *)global_forward3d_pointer,
                *(datum_index *)&tag_id, -1, 1.0f, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x44c1a0), from tools/pack.py 0x44c1a0:

void device_play_state_change_effect(void)

{
  int iVar1;
  int in_ECX;

  if (in_ECX != -1) {
    iVar1 = *(int *)((short)in_ECX * 0x20 + DAT_0087bc14);
    if (iVar1 == 0x65666665) {
      FUN_004507a0();
    }
    else if (iVar1 == 0x736e6421) {
      FUN_00543ce0();
      return;
    }
  }
  return;
}
#endif
