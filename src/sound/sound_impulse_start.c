// sound_impulse_start  (Ghidra: FUN_00543e10; earlier draft name sound_refresh_scripted_sound)
// address 0x543e10, size 427 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: the argument list (sound tag, object, scale) is the hs command
//   sound_impulse_start <sound> <object> <real>; the previous scripted instance
//   (Sound.scripting_sound, tag+0x94) is faded out through sound_impulse_fade_out (0x549ee0), and
//   Sound.scripting_time (tag+0x90) is set to the tick the new one ends, which
//   sound_impulse_time (0x543fc0) reads back. The object path attaches the sound to the
//   object's "head" marker (string at 0x0066bfa0) through sound_start_at_object_marker.
// Phase-4 review: rewritten from the disassembly appended below. The earlier draft lost the
//   [0,1] clamp of scale, the "head" marker, the owner and first-person arguments, the
//   node-space position/forward (and their global_origin/global_forward fallback) handed to
//   0x543ce0 in ECX/EAX, and the unspatialized location's scale.
// register convention: EAX -> object_index, ECX -> definition_index, stack -> scale.
// UNSURE: object_type_definitions_notify_0x58 (0x4f4480, objects module) is called with EBX =
//   object and two stack words (definition, new sound) that it forwards to every
//   sub-definition's notify_58(object, definition, sound) -- the disassembly shows the forward;
//   src/objects/object_type_definitions_notify_0x58.c declares it with the object only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "sound.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern const real_point3d *global_origin3d_pointer;   // 0x00696714 (the copy read here is 0x006966f8, same target 0x0065c230)
extern const real_vector3d *global_forward3d_pointer; // 0x00696718
extern char ai_marker_name_a[];                      // 0x0066bfa0, "head"

extern void sound_impulse_fade_out(datum_index sound_index); // 0x549ee0, blam-cc: ECX
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward,
    datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint); // 0x543ce0, blam-cc: ESI, ECX, EAX, stack
extern datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint); // 0x549af0
extern void object_type_definitions_notify_0x58(uint32_t object_index, datum_index definition_index,
    datum_index sound_index); // 0x4f4480, blam-cc: EBX -> object_index, stack -> (definition_index, sound_index); see UNSURE

// blam-cc: EAX -> object_index, ECX -> definition_index, stack -> scale
// hs sound_impulse_start: replaces the tag's scripted one-shot with a new instance, on the
// object's head marker (or its origin) when an object is given, unspatialized otherwise.
void sound_impulse_start(datum_index object_index, datum_index definition_index, float scale)
{
    Sound *tag;
    datum_index new_sound;

    if (definition_index == k_datum_index_none) {
        return;
    }

    tag = (Sound *)tag_instances[definition_index & 0xffff].data;
    sound_impulse_fade_out(*(datum_index *)&tag->scripting_sound);
    tag->scripting_time = ((int32_t)tag->longest_permutation_length * 30) / 1000 + game_time->game_time;

    if (scale < 0.0f) {
        scale = 0.0f;
    } else if (scale > 1.0f) {
        scale = 1.0f;
    }

    if (object_index == k_datum_index_none) {
        sound_location location;

        location.type = _sound_location_none;
        location.scale = scale;
        location.gain = 1.0f;
        new_sound = sound_play_new(definition_index, &location, k_datum_index_none, (sound_location_proc)0,
            (void *)0, 0, 0);
    } else {
        object_marker marker;
        Point3D position;
        Vector3D forward;
        int16_t node_index;

        if ((int16_t)object_get_node_local_transform(object_index, ai_marker_name_a, &marker, 1) != 0) {
            position = *(Point3D *)&marker.transform.position;
            forward = *(Vector3D *)&marker.transform.forward;
            node_index = marker.node_index;
        } else {
            position = *(Point3D *)global_origin3d_pointer;
            forward = *(Vector3D *)global_forward3d_pointer;
            node_index = 0;
        }

        new_sound = sound_start_at_object_marker(object_index, &position, &forward, definition_index, node_index,
            scale, 0);
        if (new_sound != k_datum_index_none) {
            object_type_definitions_notify_0x58(object_index, definition_index, new_sound);
        }
    }

    *(datum_index *)&tag->scripting_sound = new_sound;
}

#if 0
Original Ghidra decompilation (0x543e10):

void FUN_00543e10(void)

{
  int iVar1;
  int in_EAX;
  int iVar2;
  uint in_ECX;

  if (in_ECX != 0xffffffff) {
    iVar1 = *(int *)((in_ECX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    FUN_00549ee0();
    *(int *)(iVar1 + 0x90) = (*(int *)(iVar1 + 0x84) * 0x1e) / 1000 + *(int *)(DAT_006f1d6c + 0xc);
    if (in_EAX == -1) {
      iVar2 = FUN_00549af0();
    }
    else {
      object_get_node_local_transform();
      iVar2 = FUN_00543ce0();
      if (iVar2 != -1) {
        object_type_definitions_notify_0x58();
        *(int *)(iVar1 + 0x94) = iVar2;
        return;
      }
    }
    *(int *)(iVar1 + 0x94) = iVar2;
  }
  return;
}

Disassembly (0x543e10..0x543fbb, capstone; phase-4 review):

0x543e10: sub esp, 0x84
0x543e16: push ebx
0x543e17: mov ebx, ecx
0x543e19: cmp ebx, -1
0x543e1c: push esi
0x543e1d: mov esi, eax
0x543e1f: je 0x543fb2
0x543e25: mov ecx, dword ptr [0x87bc14]
0x543e2b: mov eax, ebx
0x543e2d: and eax, 0xffff
0x543e32: shl eax, 5
0x543e35: push ebp
0x543e36: mov ebp, dword ptr [eax + ecx + 0x14]
0x543e3a: mov ecx, dword ptr [ebp + 0x94]
0x543e40: push edi
0x543e41: call 0x549ee0
0x543e46: fld dword ptr [esp + 0x98]
0x543e4d: mov ecx, dword ptr [ebp + 0x84]
0x543e53: fcomp dword ptr [0x672ac0]
0x543e59: imul ecx, ecx, 0x1e
0x543e5c: mov eax, 0x10624dd3
0x543e61: imul ecx
0x543e63: mov ecx, dword ptr [0x6f1d6c]
0x543e69: sar edx, 6
0x543e6c: mov eax, edx
0x543e6e: shr eax, 0x1f
0x543e71: add eax, edx
0x543e73: add eax, dword ptr [ecx + 0xc]
0x543e76: mov dword ptr [ebp + 0x90], eax
0x543e7c: fnstsw ax
0x543e7e: test ah, 5
0x543e81: jp 0x543e90
0x543e83: mov dword ptr [esp + 0x98], 0
0x543e8e: jmp 0x543eaf
0x543e90: fld dword ptr [esp + 0x98]
0x543e97: fcomp dword ptr [0x672ac4]
0x543e9d: fnstsw ax
0x543e9f: test ah, 0x41
0x543ea2: jne 0x543eaf
0x543ea4: mov dword ptr [esp + 0x98], 0x3f800000
0x543eaf: cmp esi, -1
0x543eb2: je 0x543f76
0x543eb8: push 1
0x543eba: lea edx, [esp + 0x2c]
0x543ebe: push edx
0x543ebf: push 0x66bfa0
0x543ec4: push esi
0x543ec5: call 0x4f6080
0x543eca: add esp, 0x10
0x543ecd: test ax, ax
0x543ed0: je 0x543f00
0x543ed2: mov ecx, dword ptr [esp + 0x54]
0x543ed6: mov edx, dword ptr [esp + 0x58]
0x543eda: mov eax, dword ptr [esp + 0x28]
0x543ede: mov dword ptr [esp + 0x10], ecx
0x543ee2: mov ecx, dword ptr [esp + 0x5c]
0x543ee6: mov dword ptr [esp + 0x14], edx
0x543eea: mov edx, dword ptr [esp + 0x30]
0x543eee: mov dword ptr [esp + 0x18], ecx
0x543ef2: mov ecx, dword ptr [esp + 0x34]
0x543ef6: mov dword ptr [esp + 0x1c], edx
0x543efa: mov edx, dword ptr [esp + 0x38]
0x543efe: jmp 0x543f2e
0x543f00: mov ecx, dword ptr [0x6966f8]
0x543f06: mov edx, dword ptr [ecx]
0x543f08: mov dword ptr [esp + 0x10], edx
0x543f0c: mov edx, dword ptr [ecx + 4]
0x543f0f: mov ecx, dword ptr [ecx + 8]
0x543f12: mov dword ptr [esp + 0x14], edx
0x543f16: mov edx, dword ptr [0x696718]
0x543f1c: mov dword ptr [esp + 0x18], ecx
0x543f20: mov ecx, dword ptr [edx]
0x543f22: mov dword ptr [esp + 0x1c], ecx
0x543f26: mov ecx, dword ptr [edx + 4]
0x543f29: mov edx, dword ptr [edx + 8]
0x543f2c: xor eax, eax
0x543f2e: mov dword ptr [esp + 0x20], ecx
0x543f32: mov ecx, dword ptr [esp + 0x98]
0x543f39: push 0
0x543f3b: push ecx
0x543f3c: push eax
0x543f3d: push ebx
0x543f3e: lea eax, [esp + 0x2c]
0x543f42: lea ecx, [esp + 0x20]
0x543f46: mov dword ptr [esp + 0x34], edx
0x543f4a: call 0x543ce0
0x543f4f: mov edi, eax
0x543f51: add esp, 0x10
0x543f54: cmp edi, -1
0x543f57: je 0x543faa
0x543f59: push edi
0x543f5a: push ebx
0x543f5b: mov ebx, esi
0x543f5d: call 0x4f4480
0x543f62: add esp, 8
0x543f65: mov dword ptr [ebp + 0x94], edi
0x543f6b: pop edi
0x543f6c: pop ebp
0x543f6d: pop esi
0x543f6e: pop ebx
0x543f6f: add esp, 0x84
0x543f75: ret 
0x543f76: mov edx, dword ptr [esp + 0x98]
0x543f7d: push 0
0x543f7f: push 0
0x543f81: push 0
0x543f83: push 0
0x543f85: push -1
0x543f87: lea eax, [esp + 0x3c]
0x543f8b: push eax
0x543f8c: push ebx
0x543f8d: mov word ptr [esp + 0x44], 0
0x543f94: mov dword ptr [esp + 0x48], edx
0x543f98: mov dword ptr [esp + 0x4c], 0x3f800000
0x543fa0: call 0x549af0
0x543fa5: add esp, 0x1c
0x543fa8: mov edi, eax
0x543faa: mov dword ptr [ebp + 0x94], edi
0x543fb0: pop edi
0x543fb1: pop ebp
0x543fb2: pop esi
0x543fb3: pop ebx
0x543fb4: add esp, 0x84
0x543fba: ret 
#endif
