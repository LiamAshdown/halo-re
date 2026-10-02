// sound_compute_obstruction_occlusion  (Ghidra: FUN_00544aa0)
// address 0x544aa0, size 360 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: out/phase4/sound_functions.md summary "Computes distance-based attenuation and, when
//   clusters may be occluded, a raycast-derived obstruction factor for a sound source relative to
//   the listener."; writes sound_location.obstruction/occlusion (0x38/0x3c, types/sound.h:
//   0.6 / 0.45 / 0.0); reads sound_location.position/cluster_index (0x0c/0x34) and the observer
//   camera row of the listener (camera.h observer_camera, R17: position 0x00, cluster
//   0x10). The raycast is collision_test_movement_segment (0x505880, physics module) with flags
//   0xc0e1 and no excluded object; the cluster-to-cluster sound distance byte comes from
//   cluster_sound_distance_lookup (0x552210: EAX / ECX clusters, EDI structure bsp, returns AL).
// Phase-4 review (disassembly appended below): the observer camera table is an array at
//   0x006ac6d0 (the draft dereferenced it as a pointer), and the raycast writes a
//   collision_result into a caller buffer (the draft passed NULL).
// register convention: EBX -> location, AX -> listener_index, stack -> reference_distance.
// UNSURE: global_structure_bsp+0x14c (a cluster x cluster "may be occluded" bitmap, rows of
//   (cluster_count + 31) / 32 words, indexed [listener cluster][source cluster]) and +0x134
//   (cluster count) have no established names; kept as raw offsets.
// reconciled: R17 sound_observer_camera -> camera.h observer_camera via observers[i].camera (same bytes)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "camera.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern observer observers[1];                            // 0x006ac65c, camera.h; observers[i].camera is the 0x006ac6d0 row (R17)

extern uint8_t cluster_sound_distance_lookup(int16_t cluster_a, int16_t cluster_b,
                                             ScenarioStructureBSP *structure_bsp); // 0x552210; EAX, ECX, EDI bsp
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, collision_result *result); // 0x505880, physics module

// blam-cc: EBX -> location, AX -> listener_index, stack -> reference_distance
// Defaults a source's obstruction/occlusion to 0.6/1.0. When the source and listener clusters
// are within sound range, a flagged cluster pair is raycast (obstruction 0.45, or 0/0 on a clear
// line); unless the ray was clear, occlusion becomes 1.4 * (1 - reference / (distance +
// reference)) clamped to [0, 1].
void sound_compute_obstruction_occlusion(sound_location *location, int16_t listener_index, float reference_distance)
{
    observer_camera *listener = (observer_camera *)0;
    int16_t listener_cluster;
    float distance;

    if (listener_index != -1) {
        listener = &observers[listener_index].camera;
    }

    location->obstruction = 0.6f;
    location->occlusion = 1.0f;
    if (location->cluster_index == -1) {
        return;
    }

    listener_cluster = listener->cluster_index; // the binary does not test listener for NULL
    if (listener_cluster == -1) {
        return;
    }

    distance = (float)(cluster_sound_distance_lookup(location->cluster_index, listener_cluster,
                                              (ScenarioStructureBSP *)global_structure_bsp /* 0x544bf2: EDI = [0x746f9c] */) & 0x7f) * 2.015748f;
    if (!(distance < 256.0f)) {
        return;
    }

    {
        int32_t cluster_count = (int32_t)global_structure_bsp->clusters.count;
        uint32_t *occlusion_bitmap = (uint32_t *)global_structure_bsp->cluster_data.pointer;
        int32_t word_index = ((cluster_count + 0x1f) >> 5) * listener_cluster + (location->cluster_index >> 5);

        if ((occlusion_bitmap[word_index] & (1u << (location->cluster_index & 0x1f))) != 0) {
            real_vector3d delta;
            collision_result result;

            location->obstruction = 0.45f;
            delta.i = location->position.x - listener->position.x;
            delta.j = location->position.y - listener->position.y;
            delta.k = location->position.z - listener->position.z;

            if (collision_test_movement_segment(0xc0e1, (real_point3d *)&listener->position, &delta, 0xffffffff,
                    &result) == 0) {
                location->obstruction = 0.0f;
                location->occlusion = 0.0f;
            }
        }
    }

    if (location->obstruction != 0.0f) {
        float occlusion = 1.0f - reference_distance / (distance + reference_distance);

        location->occlusion = occlusion;
        occlusion = occlusion * 1.4f;
        if (occlusion < 0.0f) {
            location->occlusion = 0.0f;
            return;
        }
        if (occlusion > 1.0f) {
            occlusion = 1.0f;
        }
        location->occlusion = occlusion;
    }
}

#if 0
Original Ghidra decompilation (0x544aa0):

void FUN_00544aa0(float param_1)

{
  short sVar1;
  float fVar2;
  int iVar3;
  char cVar4;
  short in_AX;
  uint uVar5;
  int unaff_EBX;
  float *pfVar6;
  float local_5c;
  float local_58;
  float local_54;
  undefined1 local_50 [80];

  pfVar6 = (float *)0x0;
  if (in_AX != -1) {
    pfVar6 = (float *)(&DAT_006ac6d0 + in_AX * 0xa7);
  }
  *(undefined4 *)(unaff_EBX + 0x38) = 0x3f19999a;
  *(undefined4 *)(unaff_EBX + 0x3c) = 0x3f800000;
  iVar3 = DAT_00746f9c;
  if ((*(short *)(unaff_EBX + 0x34) != -1) && (sVar1 = *(short *)(pfVar6 + 4), sVar1 != -1)) {
    uVar5 = cluster_sound_distance_lookup();
    fVar2 = (float)(uVar5 & 0x7f) * 2.015748;
    if (fVar2 < 256.0) {
      if ((*(uint *)(*(int *)(iVar3 + 0x14c) +
                    ((*(int *)(iVar3 + 0x134) + 0x1f >> 5) * (int)sVar1 +
                    ((int)*(short *)(unaff_EBX + 0x34) >> 5)) * 4) &
          1 << ((byte)*(short *)(unaff_EBX + 0x34) & 0x1f)) != 0) {
        *(undefined4 *)(unaff_EBX + 0x38) = 0x3ee66666;
        local_5c = *(float *)(unaff_EBX + 0xc) - *pfVar6;
        local_58 = *(float *)(unaff_EBX + 0x10) - pfVar6[1];
        local_54 = *(float *)(unaff_EBX + 0x14) - pfVar6[2];
        cVar4 = FUN_00505880(0xc0e1,pfVar6,&local_5c,0xffffffff,local_50);
        if (cVar4 == '\0') {
          *(undefined4 *)(unaff_EBX + 0x38) = 0;
          *(undefined4 *)(unaff_EBX + 0x3c) = 0;
        }
      }
      if (*(float *)(unaff_EBX + 0x38) != 0.0) {
        fVar2 = 1.0 - param_1 / (fVar2 + param_1);
        *(float *)(unaff_EBX + 0x3c) = fVar2;
        fVar2 = fVar2 * 1.4;
        if (fVar2 < 0.0) {
          *(undefined4 *)(unaff_EBX + 0x3c) = 0;
          return;
        }
        if (1.0 < fVar2) {
          fVar2 = 1.0;
        }
        *(float *)(unaff_EBX + 0x3c) = fVar2;
      }
    }
  }
  return;
}

Disassembly (0x544aa0..0x544c08, capstone; phase-4 review):

0x544aa0: sub esp, 0x60
0x544aa3: push esi
0x544aa4: xor esi, esi
0x544aa6: cmp ax, 0xffff
0x544aaa: je 0x544abb
0x544aac: movsx esi, ax
0x544aaf: imul esi, esi, 0x29c
0x544ab5: add esi, 0x6ac6d0
0x544abb: mov ax, word ptr [ebx + 0x34]
0x544abf: cmp ax, 0xffff
0x544ac3: mov dword ptr [ebx + 0x38], 0x3f19999a
0x544aca: mov dword ptr [ebx + 0x3c], 0x3f800000
0x544ad1: je 0x544c03
0x544ad7: push ebp
0x544ad8: xor ebp, ebp
0x544ada: mov bp, word ptr [esi + 0x10]
0x544ade: cmp bp, -1
0x544ae2: je 0x544c02
0x544ae8: push edi
0x544ae9: mov edi, dword ptr [0x746f9c]
0x544aef: mov ecx, ebp
0x544af1: call 0x552210
0x544af6: movzx eax, al
0x544af9: and eax, 0xffffff7f
0x544afe: mov dword ptr [esp + 0xc], eax
0x544b02: fild dword ptr [esp + 0xc]
0x544b06: fmul dword ptr [0x672ce4]
0x544b0c: fst dword ptr [esp + 0xc]
0x544b10: fcomp dword ptr [0x672ce0]
0x544b16: fnstsw ax
0x544b18: test ah, 5
0x544b1b: jp 0x544c01
0x544b21: mov edx, dword ptr [edi + 0x134]
0x544b27: movsx ecx, word ptr [ebx + 0x34]
0x544b2b: add edx, 0x1f
0x544b2e: movsx eax, bp
0x544b31: sar edx, 5
0x544b34: imul edx, eax
0x544b37: mov eax, ecx
0x544b39: sar eax, 5
0x544b3c: add edx, eax
0x544b3e: mov eax, dword ptr [edi + 0x14c]
0x544b44: and ecx, 0x1f
0x544b47: mov edi, 1
0x544b4c: shl edi, cl
0x544b4e: test dword ptr [eax + edx*4], edi
0x544b51: je 0x544b9d
0x544b53: mov dword ptr [ebx + 0x38], 0x3ee66666
0x544b5a: fld dword ptr [ebx + 0xc]
0x544b5d: fsub dword ptr [esi]
0x544b5f: lea ecx, [esp + 0x1c]
0x544b63: push ecx
0x544b64: push -1
0x544b66: fstp dword ptr [esp + 0x18]
0x544b6a: lea edx, [esp + 0x18]
0x544b6e: fld dword ptr [ebx + 0x10]
0x544b71: push edx
0x544b72: fsub dword ptr [esi + 4]
0x544b75: push esi
0x544b76: push 0xc0e1
0x544b7b: fstp dword ptr [esp + 0x28]
0x544b7f: fld dword ptr [ebx + 0x14]
0x544b82: fsub dword ptr [esi + 8]
0x544b85: fstp dword ptr [esp + 0x2c]
0x544b89: call 0x505880
0x544b8e: add esp, 0x14
0x544b91: test al, al
0x544b93: jne 0x544b9d
0x544b95: xor eax, eax
0x544b97: mov dword ptr [ebx + 0x38], eax
0x544b9a: mov dword ptr [ebx + 0x3c], eax
0x544b9d: fld dword ptr [0x672ac0]
0x544ba3: fld dword ptr [ebx + 0x38]
0x544ba6: fucompp 
0x544ba8: fnstsw ax
0x544baa: test ah, 0x44
0x544bad: jnp 0x544c01
0x544baf: fld dword ptr [esp + 0xc]
0x544bb3: fadd dword ptr [esp + 0x70]
0x544bb7: fdivr dword ptr [esp + 0x70]
0x544bbb: fsubr dword ptr [0x672ac4]
0x544bc1: fst dword ptr [ebx + 0x3c]
0x544bc4: fmul dword ptr [0x672edc]
0x544bca: fcom dword ptr [0x672ac0]
0x544bd0: fnstsw ax
0x544bd2: test ah, 5
0x544bd5: jp 0x544be9
0x544bd7: pop edi
0x544bd8: fstp st(0)
0x544bda: fld dword ptr [0x672ac0]
0x544be0: pop ebp
0x544be1: fstp dword ptr [ebx + 0x3c]
0x544be4: pop esi
0x544be5: add esp, 0x60
0x544be8: ret 
0x544be9: fcom dword ptr [0x672ac4]
0x544bef: fnstsw ax
0x544bf1: test ah, 0x41
0x544bf4: jne 0x544bfe
0x544bf6: fstp st(0)
0x544bf8: fld dword ptr [0x672ac4]
0x544bfe: fstp dword ptr [ebx + 0x3c]
0x544c01: pop edi
0x544c02: pop ebp
0x544c03: pop esi
0x544c04: add esp, 0x60
0x544c07: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
