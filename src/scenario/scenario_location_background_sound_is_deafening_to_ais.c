// scenario_location_background_sound_is_deafening_to_ais  (Ghidra: FUN_0053e810, still unnamed
// there; renamed per out/phase4/scenario_types_notes.md, which places this among the
// "scenario_location_*" family of resident-structure-bsp query helpers)
// address 0x53e810, size 87 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase4/scenario_types_notes.md: EAX = bsp_leaf_reference *; follows
// location->cluster_index into global_structure_bsp->clusters[].background_sound (+0x04,
// confirmed against types/tags.h ScenarioStructureBSPCluster), bounds-checks it against
// background_sound_palette.count (+0x1fc), reads that palette entry's background_sound tag id
// (+0x2c, ScenarioStructureBSPBackgroundSoundPalette), and if present tests bit 0 of the
// referenced SoundLooping tag's flags -- k_sound_looping_flag_deafening_to_ais.
// register convention: raw disassembly (0x53e810) opens with `movsx ecx,WORD PTR [eax+0x4]`
// with no prior setup, so EAX is the bsp_leaf_reference parameter; no stack parameters. Returns
// a bool in AL (0 for every early-out: no background sound, out of range, or no tag).
//   // blam-cc: EAX -> location; return in AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "scenario.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances;                 // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp;   // 0x00746f9c

// blam-cc: EAX -> location
// True when the background sound of location's cluster is a SoundLooping tag with the
// deafening_to_ais flag set. False when the cluster has no background sound, the index is out
// of range, or the referenced tag id is unset.
uint8_t scenario_location_background_sound_is_deafening_to_ais(bsp_leaf_reference *location)
{
    ScenarioStructureBSPCluster *clusters;
    ScenarioStructureBSPBackgroundSoundPalette *palette;
    int16_t background_sound_index;
    uint32_t sound_tag;
    SoundLooping *sound;

    clusters = (ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer;
    background_sound_index = (int16_t)clusters[location->cluster_index].background_sound;

    if (background_sound_index != -1 &&
        (int32_t)background_sound_index < (int32_t)global_structure_bsp->background_sound_palette.count) {
        palette = (ScenarioStructureBSPBackgroundSoundPalette *)
            global_structure_bsp->background_sound_palette.pointer;
        sound_tag = *(uint32_t *)&palette[background_sound_index].background_sound.tag_id;
        if (sound_tag != 0xffffffff) {
            sound = (SoundLooping *)tag_instances[sound_tag & 0xffff].data;
            return (sound->flags & k_sound_looping_flag_deafening_to_ais) != 0;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x53e810):

uint FUN_0053e810(void)

{
  short sVar1;
  uint uVar2;
  undefined1 *puVar3;
  uint in_EAX;
  uint uVar4;

  sVar1 = *(short *)(*(short *)(in_EAX + 4) * 0x68 + *(int *)(DAT_00746f9c + 0x138) + 4);
  uVar4 = in_EAX & 0xffffff00;
  if (((sVar1 != -1) && ((int)sVar1 < *(int *)(DAT_00746f9c + 0x1fc))) &&
     (uVar2 = *(uint *)(sVar1 * 0x74 + *(int *)(DAT_00746f9c + 0x200) + 0x2c), uVar2 != 0xffffffff))
  {
    puVar3 = *(undefined1 **)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    uVar4 = CONCAT31((int3)((uint)puVar3 >> 8),*puVar3) & 0xffffff01;
  }
  return uVar4;
}

Raw disassembly (0x53e810-0x53e866):

  53e810: movsx  ecx,WORD PTR [eax+0x4]        ; location->cluster_index
  53e814: mov    edx,DWORD PTR ds:0x746f9c     ; global_structure_bsp
  53e81a: imul   ecx,ecx,0x68                  ; * cluster stride
  53e81d: add    ecx,DWORD PTR [edx+0x138]     ; + clusters.pointer
  53e823: mov    cx,WORD PTR [ecx+0x4]         ; cluster.background_sound
  53e827: xor    al,al
  53e829: cmp    cx,0xffff
  53e82d: je     0x53e866
  53e82f: movsx  ecx,cx
  53e832: push   esi
  53e833: cmp    ecx,DWORD PTR [edx+0x1fc]     ; background_sound_palette.count
  53e839: jge    0x53e865
  53e83b: mov    esi,DWORD PTR [edx+0x200]     ; background_sound_palette.pointer
  53e841: imul   ecx,ecx,0x74
  53e844: add    ecx,esi
  53e846: mov    ecx,DWORD PTR [ecx+0x2c]      ; palette[i].background_sound.tag_id
  53e849: cmp    ecx,0xffffffff
  53e84c: je     0x53e865
  53e84e: mov    edx,DWORD PTR ds:0x87bc14     ; tag_instances
  53e854: and    ecx,0xffff
  53e85a: shl    ecx,0x5
  53e85d: mov    eax,DWORD PTR [ecx+edx*1+0x14]
  53e861: mov    al,BYTE PTR [eax]             ; SoundLooping.flags low byte
  53e863: and    al,0x1
  53e865: pop    esi
  53e866: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
