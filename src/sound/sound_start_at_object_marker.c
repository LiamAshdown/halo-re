// sound_start_at_object_marker  (Ghidra: FUN_00543ce0)
// address 0x543ce0, size 156 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: builds the 0x1c-byte sound_object_marker_data (types/sound.h) and an absolute
//   sound_location on the stack, primes the location through sound_location_object_marker
//   (0x5448c0) and then passes that same proc, the callback data and its size (0x1c) to
//   sound_play_new (0x549af0). Twelve callers across the weapon, unit, device and effect code
//   (effect_event_apply, first_person_weapon_update, unit_trigger_material_hit_effect, ...).
// Phase-4 review: rewritten from the disassembly below; the Ghidra C hides the ESI/ECX/EAX
//   arguments and the earlier draft passed the wrong values to sound_play_new.
// register convention: ESI -> object_index, ECX -> node-space position, EAX -> node-space
//   forward, stack -> (definition_index, node_index, scale, first_person_hint).

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"


extern datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint); // 0x549af0

// blam-cc: ESI -> object_index, ECX -> position, EAX -> forward,
//   stack -> (definition_index, node_index, scale, first_person_hint)
// Starts `definition_index` attached to a point on one of `object_index`'s nodes; the sound
// follows the node because the marker data rides along as the sound's callback data.
datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward,
    datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint)
{
    sound_object_marker_data marker;
    sound_location location;

    marker.position = *position;
    marker.forward = *forward;
    marker.node_index = node_index;
    location.type = _sound_location_absolute;
    location.gain = 1.0f;
    location.cluster_index = -1;

    if (sound_location_object_marker(object_index, &marker, &location) == 0) {
        return k_datum_index_none;
    }

    location.scale = scale;
    return sound_play_new(definition_index, &location, object_index, sound_location_object_marker, &marker,
        sizeof(sound_object_marker_data), first_person_hint);
}

#if 0
Original Ghidra decompilation (0x543ce0):

undefined4 FUN_00543ce0(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  char cVar1;
  undefined4 uVar2;
  undefined2 local_40 [2];
  undefined4 local_3c;
  undefined4 local_38;
  undefined2 local_c;

  local_40[0] = 1;
  local_38 = 0x3f800000;
  local_c = 0xffff;
  cVar1 = FUN_005448c0();
  if (cVar1 != '\0') {
    local_3c = param_3;
    uVar2 = FUN_00549af0(param_1,local_40);
    return uVar2;
  }
  return 0xffffffff;
}

Disassembly (0x543ce0..0x543d7c, capstone; phase-4 review):

0x543ce0: sub esp, 0x5c
0x543ce3: mov edx, dword ptr [ecx]
0x543ce5: mov dword ptr [esp + 4], edx
0x543ce9: mov edx, dword ptr [ecx + 4]
0x543cec: mov ecx, dword ptr [ecx + 8]
0x543cef: mov dword ptr [esp + 8], edx
0x543cf3: mov edx, dword ptr [eax]
0x543cf5: mov dword ptr [esp + 0xc], ecx
0x543cf9: mov ecx, dword ptr [eax + 4]
0x543cfc: mov dword ptr [esp + 0x10], edx
0x543d00: mov edx, dword ptr [eax + 8]
0x543d03: mov ax, word ptr [esp + 0x64]
0x543d08: push edi
0x543d09: mov dword ptr [esp + 0x18], ecx
0x543d0d: lea ecx, [esp + 0x20]
0x543d11: mov dword ptr [esp + 0x1c], edx
0x543d15: push ecx
0x543d16: lea edx, [esp + 8]
0x543d1a: push edx
0x543d1b: or edi, 0xffffffff
0x543d1e: push esi
0x543d1f: mov word ptr [esp + 0x2c], 1
0x543d26: mov dword ptr [esp + 0x34], 0x3f800000
0x543d2e: mov word ptr [esp + 0x12], ax
0x543d33: mov word ptr [esp + 0x60], di
0x543d38: call 0x5448c0
0x543d3d: add esp, 0xc
0x543d40: test al, al
0x543d42: je 0x543d75
0x543d44: mov ecx, dword ptr [esp + 0x70]
0x543d48: mov eax, dword ptr [esp + 0x6c]
0x543d4c: push ecx
0x543d4d: mov ecx, dword ptr [esp + 0x68]
0x543d51: push 0x1c
0x543d53: lea edx, [esp + 0xc]
0x543d57: push edx
0x543d58: push 0x5448c0
0x543d5d: mov dword ptr [esp + 0x34], eax
0x543d61: push esi
0x543d62: lea eax, [esp + 0x34]
0x543d66: push eax
0x543d67: push ecx
0x543d68: call 0x549af0
0x543d6d: add esp, 0x1c
0x543d70: pop edi
0x543d71: add esp, 0x5c
0x543d74: ret 
0x543d75: mov eax, edi
0x543d77: pop edi
0x543d78: add esp, 0x5c
0x543d7b: ret 
#endif
