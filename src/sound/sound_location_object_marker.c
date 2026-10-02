// sound_location_object_marker  (Ghidra: FUN_005448c0)
// address 0x5448c0, size 183 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: its address is pushed as the location_proc of sound_play_new by
//   sound_start_at_object_marker (0x543ce0), which also builds the 0x1c-byte callback data
//   (sound_object_marker_data, types/sound.h); sound_update_active_instances (0x54c900) compares
//   location_proc against this address. Callees resolved from the disassembly below:
//   object_try_and_get (ECX object, stack type mask -1), object_get_root_location (EAX out,
//   ECX object), matrix4x3_transform_point / _normal (EAX out, EDX in, stack matrix) and
//   object_get_root_object_velocities (EAX object, ESI out velocity, EDI out angular = NULL).
// Phase-4 review: the earlier draft transformed the global origin/forward/up vectors and
//   treated the 0x4f6aa0 result as an "up" vector; the binary transforms the node-space
//   position and forward carried in the callback data, and 0x4f6aa0 writes the root object's
//   linear velocity into location+0x24.
// register convention: plain stack (owner, callback_data, location); returns AL.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern void object_get_root_location(int32_t *out, uint32_t object_index); // 0x4f6b10, blam-cc: EAX -> out, ECX -> object_index
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0, blam-cc: EAX out, EDX point
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0, blam-cc: EAX out, EDX normal
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, blam-cc: EAX -> object_index, ESI -> out_velocity, EDI -> out_angular_velocity

// blam-cc: stack -> (owner, callback_data, location)
// sound_location_proc for sounds attached to an object marker: places `location` at the
// callback's node-space point/direction on `owner`'s node, with the root object's cluster
// and velocity. Fails when the object is gone or outside every cluster.
uint8_t sound_location_object_marker(datum_index owner, void *callback_data, sound_location *location)
{
    sound_object_marker_data *marker = (sound_object_marker_data *)callback_data;
    int32_t root_location[2];
    int16_t node_index;
    object *obj;
    real_matrix4x3 *node_matrix;

    if (object_try_and_get(owner, 0xffffffff) == 0) {
        return 0;
    }

    object_get_root_location(root_location, owner);
    if ((int16_t)root_location[1] == -1) {
        return 0;
    }

    node_index = marker->node_index == -1 ? 0 : marker->node_index;
    obj = ((object_header *)object_data->data)[owner & 0xffff].data;
    node_matrix = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset + node_index * 0x34);

    location->leaf_index = root_location[0];
    *(int32_t *)&location->cluster_index = root_location[1];
    matrix4x3_transform_point((real_point3d *)&location->position, (real_point3d *)&marker->position, node_matrix);
    matrix4x3_transform_normal((real_vector3d *)&location->forward, (real_vector3d *)&marker->forward, node_matrix);
    object_get_root_object_velocities(owner, (real_vector3d *)&location->velocity, (real_vector3d *)0);
    return 1;
}

#if 0
Original Ghidra decompilation (0x5448c0):

undefined4 FUN_005448c0(uint param_1,int param_2,int param_3)

{
  int iVar1;
  short sVar2;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = object_try_and_get(0xffffffff);
  if ((iVar1 != 0) && (FUN_004f6b10(), (short)local_4 != -1)) {
    sVar2 = *(short *)(param_2 + 2);
    if (sVar2 == -1) {
      sVar2 = 0;
    }
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    iVar1 = (int)*(short *)(iVar1 + 0x1f2) + sVar2 * 0x34 + iVar1;
    *(undefined4 *)(param_3 + 0x30) = local_8;
    *(undefined4 *)(param_3 + 0x34) = local_4;
    matrix4x3_transform_point(iVar1);
    matrix4x3_transform_normal(iVar1);
    FUN_004f6aa0();
    return 1;
  }
  return 0;
}

Disassembly (0x5448c0..0x544977, capstone; phase-4 review):

0x5448c0: sub esp, 8
0x5448c3: push ebx
0x5448c4: mov ebx, dword ptr [esp + 0x10]
0x5448c8: push -1
0x5448ca: mov ecx, ebx
0x5448cc: call 0x4f6ec0
0x5448d1: add esp, 4
0x5448d4: test eax, eax
0x5448d6: je 0x544970
0x5448dc: lea eax, [esp + 4]
0x5448e0: mov ecx, ebx
0x5448e2: call 0x4f6b10
0x5448e7: mov edx, dword ptr [esp + 8]
0x5448eb: cmp dx, -1
0x5448ef: je 0x544970
0x5448f1: push ebp
0x5448f2: mov ebp, dword ptr [esp + 0x18]
0x5448f6: mov ax, word ptr [ebp + 2]
0x5448fa: cmp ax, 0xffff
0x5448fe: push esi
0x5448ff: push edi
0x544900: jne 0x544906
0x544902: xor ecx, ecx
0x544904: jmp 0x544909
0x544906: movsx ecx, ax
0x544909: mov esi, dword ptr [0x8603b0]
0x54490f: mov esi, dword ptr [esi + 0x34]
0x544912: mov edi, dword ptr [esp + 0x24]
0x544916: movsx ecx, cx
0x544919: imul ecx, ecx, 0x34
0x54491c: mov eax, ebx
0x54491e: and eax, 0xffff
0x544923: lea eax, [eax + eax*2]
0x544926: mov eax, dword ptr [esi + eax*4 + 8]
0x54492a: movsx esi, word ptr [eax + 0x1f2]
0x544931: add ecx, eax
0x544933: mov eax, dword ptr [esp + 0x10]
0x544937: add esi, ecx
0x544939: mov dword ptr [edi + 0x30], eax
0x54493c: mov dword ptr [edi + 0x34], edx
0x54493f: lea eax, [edi + 0xc]
0x544942: lea edx, [ebp + 4]
0x544945: push esi
0x544946: call 0x4cbde0
0x54494b: lea eax, [edi + 0x18]
0x54494e: lea edx, [ebp + 0x10]
0x544951: push esi
0x544952: call 0x4cbec0
0x544957: lea esi, [edi + 0x24]
0x54495a: add esp, 8
0x54495d: xor edi, edi
0x54495f: mov eax, ebx
0x544961: call 0x4f6aa0
0x544966: pop edi
0x544967: pop esi
0x544968: pop ebp
0x544969: mov al, 1
0x54496b: pop ebx
0x54496c: add esp, 8
0x54496f: ret 
0x544970: xor al, al
0x544972: pop ebx
0x544973: add esp, 8
0x544976: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
