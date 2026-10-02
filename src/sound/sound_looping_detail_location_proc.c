// sound_looping_detail_location_proc  (Ghidra: FUN_0054dc70, still unnamed there)
// address 0x54dc70, size 275 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Fills in a detail sound's 3D orientation/position
// parameters, used as a predicted-resource callback by the driver."; signature matches
// types/sound.h's own sound_location_proc typedef exactly (owner, callback_data, location); the
// owner-validation logic is byte-identical to datum_get (index bounds + identifier + salt), so
// this rewrite calls that established helper instead of re-inlining it. Field offsets confirmed
// against looping_sound's embedded sound_location (looping_sound+0x0c) and sound_location's own
// forward/up/leaf_index/cluster_index/obstruction/occlusion offsets.
// register convention: none visible; called indirectly through sound.location_proc with the
// typedef'd (owner, callback_data, location) argument order.
// Phase-4 review (disassembly appended below): location+0x24 is the source velocity (see
// types/sound.h sound_location), so the unplaced case copying the global origin there is a
// zero velocity, not an odd "up" vector. The body otherwise matched the binary.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *looping_sound_data; // 0x00724a50, "looping sounds" 0x80 x 0xe4

extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, memory module

extern const real_vector3d *global_forward3d_pointer; // 0x00696718
extern const real_point3d *global_origin3d_pointer;  // 0x00696714

// Predicted-resource location callback for a detail sound spawned by a looping sound: places it
// at a caller-supplied offset from the owning looping sound's own location (when that location
// is absolute), inheriting its orientation, leaf/cluster and obstruction/occlusion. Returns 0 if
// the owning looping_sound handle is no longer valid.
// Matches the sound_location_proc typedef exactly (void *callback_data), which is really a
// pointer to 3 floats here; cast locally rather than widening the typedef.
uint8_t sound_looping_detail_location_proc(datum_index owner, void *callback_data, sound_location *location)
{
    looping_sound *owner_sound;
    float *offset = (float *)callback_data;

    owner_sound = (looping_sound *)datum_get(owner, looping_sound_data);
    if (owner_sound == 0) {
        return 0;
    }

    location->obstruction = owner_sound->location.obstruction;
    location->occlusion = owner_sound->location.occlusion;

    if (owner_sound->location.type == _sound_location_none) {
        location->forward = *(Vector3D *)global_forward3d_pointer;
        location->velocity = *(Vector3D *)global_origin3d_pointer; // zero velocity
    } else {
        location->forward = owner_sound->location.forward;
        location->velocity = owner_sound->location.velocity;
        *(uint32_t *)&location->leaf_index = *(uint32_t *)&owner_sound->location.leaf_index;
        *(uint32_t *)&location->cluster_index = *(uint32_t *)&owner_sound->location.cluster_index;
    }

    location->position.x = offset[0];
    location->position.y = offset[1];
    location->position.z = offset[2];
    if (location->type == _sound_location_absolute) {
        location->position.x += owner_sound->location.position.x;
        location->position.y += owner_sound->location.position.y;
        location->position.z += owner_sound->location.position.z;
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x54dc70):

undefined4 FUN_0054dc70(int param_1,float *param_2,short *param_3)

{
  float *pfVar1;
  undefined *puVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  short sVar6;
  short sVar7;

  iVar5 = 0;
  if (((param_1 != -1) && (sVar6 = (short)param_1, -1 < sVar6)) &&
     (sVar6 < *(short *)(DAT_00724a50 + 0x20))) {
    iVar3 = (int)*(short *)(DAT_00724a50 + 0x22) * (int)sVar6;
    sVar6 = *(short *)(iVar3 + *(int *)(DAT_00724a50 + 0x34));
    if ((sVar6 != 0) && ((sVar7 = (short)((uint)param_1 >> 0x10), sVar7 == 0 || (sVar6 == sVar7))))
    {
      iVar5 = iVar3 + *(int *)(DAT_00724a50 + 0x34);
    }
  }
  uVar4 = 0;
  if (iVar5 != 0) {
    *(undefined4 *)(param_3 + 0x1c) = *(undefined4 *)(iVar5 + 0x44);
    *(undefined4 *)(param_3 + 0x1e) = *(undefined4 *)(iVar5 + 0x48);
    puVar2 = PTR_DAT_00696718;
    if (*(short *)(iVar5 + 0xc) == 0) {
      *(undefined4 *)(param_3 + 0xc) = *(undefined4 *)PTR_DAT_00696718;
      *(undefined4 *)(param_3 + 0xe) = *(undefined4 *)(puVar2 + 4);
      *(undefined4 *)(param_3 + 0x10) = *(undefined4 *)(puVar2 + 8);
      puVar2 = PTR_DAT_00696714;
      *(undefined4 *)(param_3 + 0x12) = *(undefined4 *)PTR_DAT_00696714;
      *(undefined4 *)(param_3 + 0x14) = *(undefined4 *)(puVar2 + 4);
      *(undefined4 *)(param_3 + 0x16) = *(undefined4 *)(puVar2 + 8);
    }
    else {
      *(undefined4 *)(param_3 + 0x12) = *(undefined4 *)(iVar5 + 0x30);
      *(undefined4 *)(param_3 + 0x14) = *(undefined4 *)(iVar5 + 0x34);
      *(undefined4 *)(param_3 + 0x16) = *(undefined4 *)(iVar5 + 0x38);
      *(undefined4 *)(param_3 + 0xc) = *(undefined4 *)(iVar5 + 0x24);
      *(undefined4 *)(param_3 + 0xe) = *(undefined4 *)(iVar5 + 0x28);
      *(undefined4 *)(param_3 + 0x10) = *(undefined4 *)(iVar5 + 0x2c);
      *(undefined4 *)(param_3 + 0x18) = *(undefined4 *)(iVar5 + 0x3c);
      *(undefined4 *)(param_3 + 0x1a) = *(undefined4 *)(iVar5 + 0x40);
    }
    pfVar1 = (float *)(param_3 + 6);
    *pfVar1 = *param_2;
    *(float *)(param_3 + 8) = param_2[1];
    *(float *)(param_3 + 10) = param_2[2];
    if (*param_3 == 1) {
      *pfVar1 = *(float *)(iVar5 + 0x18) + *pfVar1;
      *(float *)(param_3 + 8) = *(float *)(iVar5 + 0x1c) + *(float *)(param_3 + 8);
      *(float *)(param_3 + 10) = *(float *)(iVar5 + 0x20) + *(float *)(param_3 + 10);
    }
    uVar4 = 1;
  }
  return uVar4;
}

Disassembly (0x54dc70..0x54dd83, capstone; phase-4 review):

0x54dc70: mov edx, dword ptr [esp + 4]
0x54dc74: push ebx
0x54dc75: push esi
0x54dc76: xor ecx, ecx
0x54dc78: cmp edx, -1
0x54dc7b: push edi
0x54dc7c: je 0x54dcb8
0x54dc7e: mov edi, edx
0x54dc80: sar edi, 0x10
0x54dc83: test dx, dx
0x54dc86: jl 0x54dcb8
0x54dc88: mov esi, dword ptr [0x724a50]
0x54dc8e: cmp dx, word ptr [esi + 0x20]
0x54dc92: jge 0x54dcb8
0x54dc94: movsx eax, word ptr [esi + 0x22]
0x54dc98: mov ebx, dword ptr [esi + 0x34]
0x54dc9b: movsx edx, dx
0x54dc9e: imul eax, edx
0x54dca1: mov dx, word ptr [eax + ebx]
0x54dca5: add eax, ebx
0x54dca7: test dx, dx
0x54dcaa: je 0x54dcb8
0x54dcac: test di, di
0x54dcaf: je 0x54dcb6
0x54dcb1: cmp dx, di
0x54dcb4: jne 0x54dcb8
0x54dcb6: mov ecx, eax
0x54dcb8: xor al, al
0x54dcba: test ecx, ecx
0x54dcbc: je 0x54dd7f
0x54dcc2: mov edx, dword ptr [ecx + 0x44]
0x54dcc5: mov eax, dword ptr [esp + 0x18]
0x54dcc9: mov dword ptr [eax + 0x38], edx
0x54dccc: mov edx, dword ptr [ecx + 0x48]
0x54dccf: mov dword ptr [eax + 0x3c], edx
0x54dcd2: cmp word ptr [ecx + 0xc], 0
0x54dcd7: je 0x54dd13
0x54dcd9: lea edx, [ecx + 0x30]
0x54dcdc: mov edi, dword ptr [edx]
0x54dcde: lea esi, [eax + 0x24]
0x54dce1: mov dword ptr [esi], edi
0x54dce3: mov edi, dword ptr [edx + 4]
0x54dce6: mov dword ptr [esi + 4], edi
0x54dce9: mov edx, dword ptr [edx + 8]
0x54dcec: mov dword ptr [esi + 8], edx
0x54dcef: lea edx, [ecx + 0x24]
0x54dcf2: mov edi, dword ptr [edx]
0x54dcf4: lea esi, [eax + 0x18]
0x54dcf7: mov dword ptr [esi], edi
0x54dcf9: mov edi, dword ptr [edx + 4]
0x54dcfc: mov dword ptr [esi + 4], edi
0x54dcff: mov edx, dword ptr [edx + 8]
0x54dd02: mov dword ptr [esi + 8], edx
0x54dd05: mov edx, dword ptr [ecx + 0x3c]
0x54dd08: mov dword ptr [eax + 0x30], edx
0x54dd0b: mov edx, dword ptr [ecx + 0x40]
0x54dd0e: mov dword ptr [eax + 0x34], edx
0x54dd11: jmp 0x54dd45
0x54dd13: mov esi, dword ptr [0x696718]
0x54dd19: mov edi, dword ptr [esi]
0x54dd1b: lea edx, [eax + 0x18]
0x54dd1e: mov dword ptr [edx], edi
0x54dd20: mov edi, dword ptr [esi + 4]
0x54dd23: mov dword ptr [edx + 4], edi
0x54dd26: mov esi, dword ptr [esi + 8]
0x54dd29: mov dword ptr [edx + 8], esi
0x54dd2c: mov esi, dword ptr [0x696714]
0x54dd32: mov edi, dword ptr [esi]
0x54dd34: lea edx, [eax + 0x24]
0x54dd37: mov dword ptr [edx], edi
0x54dd39: mov edi, dword ptr [esi + 4]
0x54dd3c: mov dword ptr [edx + 4], edi
0x54dd3f: mov esi, dword ptr [esi + 8]
0x54dd42: mov dword ptr [edx + 8], esi
0x54dd45: cmp word ptr [eax], 1
0x54dd49: mov esi, dword ptr [esp + 0x14]
0x54dd4d: mov ebx, dword ptr [esi]
0x54dd4f: lea edx, [eax + 0xc]
0x54dd52: mov edi, edx
0x54dd54: mov dword ptr [edi], ebx
0x54dd56: mov ebx, dword ptr [esi + 4]
0x54dd59: mov dword ptr [edi + 4], ebx
0x54dd5c: mov esi, dword ptr [esi + 8]
0x54dd5f: mov dword ptr [edi + 8], esi
0x54dd62: jne 0x54dd7d
0x54dd64: fld dword ptr [ecx + 0x18]
0x54dd67: fadd dword ptr [edx]
0x54dd69: fstp dword ptr [edx]
0x54dd6b: fld dword ptr [ecx + 0x1c]
0x54dd6e: fadd dword ptr [eax + 0x10]
0x54dd71: fstp dword ptr [eax + 0x10]
0x54dd74: fld dword ptr [ecx + 0x20]
0x54dd77: fadd dword ptr [eax + 0x14]
0x54dd7a: fstp dword ptr [eax + 0x14]
0x54dd7d: mov al, 1
0x54dd7f: pop edi
0x54dd80: pop esi
0x54dd81: pop ebx
0x54dd82: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
