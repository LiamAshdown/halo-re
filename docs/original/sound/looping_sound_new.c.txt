// looping_sound_new  (Ghidra: looping_sound_new, already named)
// address 0x543c20, size 185 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: types/sound.h game_looping_sound (object_index 0x10, definition_index 0x0c,
//   state 0x02 = _game_looping_sound_stopped, last_update 0x14 = -1, function_index 0x18,
//   node_index 0x1a, position 0x1c, forward 0x28) matches every field write here exactly by
//   offset. The locals Ghidra names local_6c/local_64/local_60/local_5c/local_40/local_3c/
//   local_38 are read but never assigned anywhere in the visible decompile; their stack-frame
//   offsets (0x6c down to 0x38, i.e. bytes 0x00, 0x08/0x0c/0x10, 0x2c/0x30/0x34 of a 0x6c-byte
//   block) line up exactly with types/objects.h object_marker (size 0x6c: node_index 0x00,
//   transform.forward 0x08, transform.position 0x2c) -- this is Ghidra's well-known failure to
//   recognize an on-stack out-parameter struct filled by a callee (the call to
//   object_get_node_local_transform right before them prints with zero visible arguments for the
//   same reason). src/objects/object_get_node_local_transform.c establishes that callee's
//   (object_index, marker_name, object_marker*, flags) signature.
// register convention: object index in EAX (in_EAX), definition (SoundLooping) tag handle in EDI
//   (unaff_EDI), function_index as the one stack argument Ghidra does recognize (param_1).
// blam-cc: EAX -> object_index, EDI -> definition_index, ECX -> marker_name, stack -> function_index
// Phase-4 review (disassembly appended below): the caller loads ECX with 0x0065512c, an empty
//   string, which this function forwards as the marker name, and pushes flags 1:
//   object_get_node_local_transform(object_index, "", &marker, 1), i.e. the object's origin
//   marker. The int16 result is tested in AX. Callers: object_create_attachments,
//   sound_looping_start (0x544090) and sound_looping_start_ambient (0x544250), all with ECX =
//   0x0065512c; the marker name is therefore modeled as a parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *game_looping_sound_data; // 0x007461a0

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, blam-cc: EAX/ECX/EDX/stack

// blam-cc: EAX -> object_index, EDI -> definition_index, ECX -> marker_name, stack -> function_index
// Allocates a new object-looping-sound datum bound to `definition_index`. When `object_index` is
// valid, first probes the object's node transform (failing the allocation if the object has none)
// and copies the resulting node index, position, and forward direction into the new datum.
datum_index looping_sound_new(datum_index object_index, datum_index definition_index, char *marker_name,
    int16_t function_index)
{
    datum_index handle = k_datum_index_none;
    object_marker marker;

    if (definition_index == k_datum_index_none) {
        return handle;
    }

    if (object_index != k_datum_index_none) {
        if ((int16_t)object_get_node_local_transform(object_index, marker_name, &marker, 1) == 0) {
            return handle;
        }
    }

    handle = datum_new(game_looping_sound_data);
    if (handle != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)handle];

        self->object_index = object_index;
        self->definition_index = definition_index;
        self->state = _game_looping_sound_stopped;
        self->flags = 0;
        self->function_index = function_index;
        self->last_update = -1;

        if (object_index != k_datum_index_none) {
            self->node_index = marker.node_index;
            self->position.x = marker.transform.position.x;
            self->position.y = marker.transform.position.y;
            self->position.z = marker.transform.position.z;
            self->forward.i = marker.transform.forward.i;
            self->forward.j = marker.transform.forward.j;
            self->forward.k = marker.transform.forward.k;
        }
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x543c20):

uint looping_sound_new(undefined2 param_1)

{
  short sVar1;
  int in_EAX;
  uint uVar2;
  int iVar3;
  int unaff_EDI;
  undefined8 uVar4;
  undefined2 local_6c;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;

  if (unaff_EDI == -1) {
LAB_00543cd1:
    uVar2 = 0xffffffff;
  }
  else {
    if (in_EAX != -1) {
      sVar1 = object_get_node_local_transform();
      if (sVar1 == 0) goto LAB_00543cd1;
    }
    uVar4 = datum_new();
    uVar2 = (uint)uVar4;
    if (uVar2 != 0xffffffff) {
      iVar3 = (uVar2 & 0xffff) * 0x34 + *(int *)((int)((ulonglong)uVar4 >> 0x20) + 0x34);
      *(int *)(iVar3 + 0x10) = in_EAX;
      *(int *)(iVar3 + 0xc) = unaff_EDI;
      *(undefined2 *)(iVar3 + 2) = 2;
      *(undefined4 *)(iVar3 + 4) = 0;
      *(undefined2 *)(iVar3 + 0x18) = param_1;
      *(undefined4 *)(iVar3 + 0x14) = 0xffffffff;
      if (in_EAX != -1) {
        *(undefined2 *)(iVar3 + 0x1a) = local_6c;
        *(undefined4 *)(iVar3 + 0x1c) = local_40;
        *(undefined4 *)(iVar3 + 0x20) = local_3c;
        *(undefined4 *)(iVar3 + 0x24) = local_38;
        *(undefined4 *)(iVar3 + 0x28) = local_64;
        *(undefined4 *)(iVar3 + 0x2c) = local_60;
        *(undefined4 *)(iVar3 + 0x30) = local_5c;
        return uVar2;
      }
    }
  }
  return uVar2;
}

Disassembly (0x543c20..0x543cd9, capstone; phase-4 review):

0x543c20: sub esp, 0x6c
0x543c23: push ebx
0x543c24: or ebx, 0xffffffff
0x543c27: cmp edi, ebx
0x543c29: push esi
0x543c2a: mov esi, eax
0x543c2c: je 0x543cd1
0x543c32: cmp esi, ebx
0x543c34: je 0x543c50
0x543c36: push 1
0x543c38: lea eax, [esp + 0xc]
0x543c3c: push eax
0x543c3d: push ecx
0x543c3e: push esi
0x543c3f: call 0x4f6080
0x543c44: add esp, 0x10
0x543c47: test ax, ax
0x543c4a: je 0x543cd1
0x543c50: mov edx, dword ptr [0x7461a0]
0x543c56: call 0x4d0480
0x543c5b: cmp eax, ebx
0x543c5d: je 0x543cd3
0x543c5f: mov ecx, eax
0x543c61: and ecx, 0xffff
0x543c67: imul ecx, ecx, 0x34
0x543c6a: push ebp
0x543c6b: mov ebp, dword ptr [edx + 0x34]
0x543c6e: mov dx, word ptr [esp + 0x7c]
0x543c73: add ecx, ebp
0x543c75: cmp esi, ebx
0x543c77: mov dword ptr [ecx + 0x10], esi
0x543c7a: mov dword ptr [ecx + 0xc], edi
0x543c7d: mov word ptr [ecx + 2], 2
0x543c83: mov dword ptr [ecx + 4], 0
0x543c8a: mov word ptr [ecx + 0x18], dx
0x543c8e: mov dword ptr [ecx + 0x14], ebx
0x543c91: pop ebp
0x543c92: je 0x543cd3
0x543c94: mov dx, word ptr [esp + 8]
0x543c99: mov word ptr [ecx + 0x1a], dx
0x543c9d: mov esi, dword ptr [esp + 0x34]
0x543ca1: lea edx, [ecx + 0x1c]
0x543ca4: mov dword ptr [edx], esi
0x543ca6: mov esi, dword ptr [esp + 0x38]
0x543caa: mov dword ptr [edx + 4], esi
0x543cad: mov esi, dword ptr [esp + 0x3c]
0x543cb1: mov dword ptr [edx + 8], esi
0x543cb4: mov edx, dword ptr [esp + 0x10]
0x543cb8: add ecx, 0x28
0x543cbb: mov dword ptr [ecx], edx
0x543cbd: mov edx, dword ptr [esp + 0x14]
0x543cc1: mov dword ptr [ecx + 4], edx
0x543cc4: mov edx, dword ptr [esp + 0x18]
0x543cc8: pop esi
0x543cc9: mov dword ptr [ecx + 8], edx
0x543ccc: pop ebx
0x543ccd: add esp, 0x6c
0x543cd0: ret 
0x543cd1: mov eax, ebx
0x543cd3: pop esi
0x543cd4: pop ebx
0x543cd5: add esp, 0x6c
0x543cd8: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
