// sound_build_cluster_range_bitmap  (Ghidra: sound_build_cluster_range_bitmap, already named)
// address 0x544980, size 274 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Rebuilds the bitmap of BSP clusters within
//   sound-audible range of the current listener cluster."; structure_bsp_globals+0x134 is
//   clusters.count (types/structures.h "+0x134 clusters.count +0x138 ptr stride 0x68
//   ScenarioStructureBSPCluster"); player_globals.local_players[0] (types/game.h) gates on a
//   local player existing; observer_cameras[0] (types/sound.h sound_observer_camera, the table
//   src/game names camera_state_table, 0x006ac6d0) .cluster_index (+0x10) is the listener cluster.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// UNSURE: structure_bsp_globals+0x220 (a byte "cluster distance" table indexed by a triangular
//   formula over two cluster indices) has no established name or type; kept as a raw offset. The
//   distance byte is masked to its low 7 bits and scaled by 2.015748 before the < 256.0 audible
//   test; neither constant's derivation is recovered here.
// Phase-4 review (disassembly appended below): the observer camera table at 0x006ac6d0 is an
// array (the draft dereferenced it as a pointer), and the triangular cluster-pair index is
// truncated to 16 bits (movsx edx, ax) before the table read. Otherwise confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "sound.h"

extern uint8_t *structure_bsp_globals;    // 0x00746f9c
extern uint32_t sound_cluster_audible_bitmap[k_sound_cluster_bitmap_words]; // 0x00746160
extern player_globals *local_player_globals; // 0x0087a478
extern sound_observer_camera observer_cameras[]; // 0x006ac6d0 (an array, not a pointer), types/sound.h

// Rebuilds sound_cluster_audible_bitmap: bit `cluster` is set when the BSP's per-cluster distance
// table places `cluster` within sound-audible range (scaled distance < 256.0) of the listener's
// current cluster (observer_cameras[0].cluster_index), gated on a local player existing.
void sound_build_cluster_range_bitmap(void)
{
    int32_t cluster_count = *(int32_t *)(structure_bsp_globals + 0x134);
    uint8_t *distance_table = *(uint8_t **)(structure_bsp_globals + 0x220); // UNSURE, see file header
    int32_t word_count = (cluster_count + 0x1f) >> 5;
    int32_t i;

    for (i = 0; i < word_count; i++) {
        sound_cluster_audible_bitmap[i] = 0;
    }

    if (local_player_globals->local_players[0] != (datum_index)k_datum_index_none &&
        observer_cameras[0].cluster_index != -1 && cluster_count > 0) {
        int16_t listener_cluster = observer_cameras[0].cluster_index;
        int32_t cluster;

        for (cluster = 0; cluster < cluster_count; cluster++) {
            uint8_t distance;

            if (cluster == listener_cluster) {
                distance = 0;
            } else {
                int16_t high = cluster;
                int16_t low = listener_cluster;
                int32_t index;

                if (cluster < listener_cluster) {
                    high = listener_cluster;
                    low = cluster;
                }

                // the binary truncates the triangular-table index to 16 bits (movsx edx, ax)
                index = (int16_t)((uint16_t)(cluster_count - 1) * low - ((low + 1) * (int32_t)low) / 2 - 1 + high);
                distance = distance_table[index];
            }

            if ((float)(distance & 0x7f) * 2.015748f < 256.0f) {
                sound_cluster_audible_bitmap[cluster >> 5] |= 1u << (cluster & 0x1f);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x544980):

void sound_build_cluster_range_bitmap(void)

{
  int iVar1;
  byte bVar2;
  short sVar3;
  uint uVar4;
  int iVar5;
  undefined4 uVar6;
  short sVar7;
  short sVar8;
  short sVar9;
  undefined4 *puVar10;
  int local_8;

  iVar1 = DAT_00746f9c;
  puVar10 = &DAT_00746160;
  for (uVar4 = *(int *)(DAT_00746f9c + 0x134) + 0x1f >> 5 & 0x3fffffff; uVar4 != 0;
      uVar4 = uVar4 - 1) {
    *puVar10 = 0;
    puVar10 = puVar10 + 1;
  }
  for (iVar5 = 0; iVar5 != 0; iVar5 = iVar5 + -1) {
    *(undefined1 *)puVar10 = 0;
    puVar10 = (undefined4 *)((int)puVar10 + 1);
  }
  if (((*(int *)(DAT_0087a478 + 4) != -1) && ((short)DAT_006ac6e0 != -1)) &&
     (sVar9 = 0, 0 < *(int *)(iVar1 + 0x134))) {
    local_8 = 0;
    uVar6 = DAT_006ac6e0;
    do {
      sVar3 = (short)uVar6;
      if (sVar9 == sVar3) {
        bVar2 = 0;
      }
      else {
        sVar7 = sVar3;
        sVar8 = sVar9;
        if (sVar3 < sVar9) {
          sVar7 = sVar9;
          sVar8 = sVar3;
        }
        bVar2 = *(byte *)((int)(short)(((*(short *)(iVar1 + 0x134) + -1) * sVar8 -
                                       (short)(((sVar8 + 1) * (int)sVar8) / 2)) + -1 + sVar7) +
                         *(int *)(iVar1 + 0x220));
      }
      if ((float)(bVar2 & 0x7f) * 2.015748 < 256.0) {
        (&DAT_00746160)[local_8 >> 5] = (&DAT_00746160)[local_8 >> 5] | 1 << ((byte)local_8 & 0x1f);
        uVar6 = DAT_006ac6e0;
      }
      sVar9 = sVar9 + 1;
      local_8 = (int)sVar9;
    } while (local_8 < *(int *)(iVar1 + 0x134));
  }
  return;
}

Disassembly (0x544980..0x544a92, capstone; phase-4 review):

0x544980: sub esp, 8
0x544983: push ebp
0x544984: mov ebp, dword ptr [0x746f9c]
0x54498a: mov ecx, dword ptr [ebp + 0x134]
0x544990: add ecx, 0x1f
0x544993: sar ecx, 5
0x544996: shl ecx, 2
0x544999: mov edx, ecx
0x54499b: shr ecx, 2
0x54499e: push edi
0x54499f: xor eax, eax
0x5449a1: mov edi, 0x746160
0x5449a6: rep stosd dword ptr es:[edi], eax
0x5449a8: mov ecx, edx
0x5449aa: and ecx, 3
0x5449ad: rep stosb byte ptr es:[edi], al
0x5449af: mov eax, dword ptr [0x87a478]
0x5449b4: cmp dword ptr [eax + 4], -1
0x5449b8: je 0x544a8c
0x5449be: mov ecx, dword ptr [0x6ac6e0]
0x5449c4: cmp cx, -1
0x5449c8: je 0x544a8c
0x5449ce: mov eax, dword ptr [ebp + 0x134]
0x5449d4: xor edi, edi
0x5449d6: cmp eax, edi
0x5449d8: jle 0x544a8c
0x5449de: push ebx
0x5449df: mov dword ptr [esp + 0xc], edi
0x5449e3: push esi
0x5449e4: cmp di, cx
0x5449e7: mov ebx, ecx
0x5449e9: mov esi, edi
0x5449eb: je 0x544a23
0x5449ed: jle 0x5449f3
0x5449ef: mov esi, ecx
0x5449f1: mov ebx, edi
0x5449f3: movsx edx, si
0x5449f6: lea eax, [edx + 1]
0x5449f9: imul eax, edx
0x5449fc: cdq 
0x5449fd: sub eax, edx
0x5449ff: xor edx, edx
0x544a01: mov dx, word ptr [ebp + 0x134]
0x544a08: sar eax, 1
0x544a0a: dec dx
0x544a0c: imul edx, esi
0x544a0f: sub edx, eax
0x544a11: lea eax, [edx + ebx - 1]
0x544a15: movsx edx, ax
0x544a18: mov eax, dword ptr [ebp + 0x220]
0x544a1e: mov al, byte ptr [edx + eax]
0x544a21: jmp 0x544a25
0x544a23: xor al, al
0x544a25: movzx edx, al
0x544a28: and edx, 0xffffff7f
0x544a2e: mov dword ptr [esp + 0x14], edx
0x544a32: fild dword ptr [esp + 0x14]
0x544a36: fmul dword ptr [0x672ce4]
0x544a3c: fcomp dword ptr [0x672ce0]
0x544a42: fnstsw ax
0x544a44: test ah, 5
0x544a47: jp 0x544a74
0x544a49: mov ecx, dword ptr [esp + 0x10]
0x544a4d: mov eax, ecx
0x544a4f: and ecx, 0x1f
0x544a52: sar eax, 5
0x544a55: mov edx, 1
0x544a5a: shl edx, cl
0x544a5c: mov ecx, dword ptr [eax*4 + 0x746160]
0x544a63: lea eax, [eax*4 + 0x746160]
0x544a6a: or ecx, edx
0x544a6c: mov dword ptr [eax], ecx
0x544a6e: mov ecx, dword ptr [0x6ac6e0]
0x544a74: mov edx, dword ptr [ebp + 0x134]
0x544a7a: inc edi
0x544a7b: movsx eax, di
0x544a7e: cmp eax, edx
0x544a80: mov dword ptr [esp + 0x10], eax
0x544a84: jl 0x5449e4
0x544a8a: pop esi
0x544a8b: pop ebx
0x544a8c: pop edi
0x544a8d: pop ebp
0x544a8e: add esp, 8
0x544a91: ret 
#endif
