// structure_bsp_dispose  (Ghidra: FUN_00442520)
// address 0x442520, size 39 bytes
// name confidence: 0.7 (counterpart of structure_bsp_load @0x4424b0: same tag_instances write,
// same ScenarioBSP.structure_bsp.tag_id field, per out/phase4/cache_functions.md summary
// "Releases a structure_bsp's cluster rendering resources and clears its tag-data pointer")
// rewrite confidence: 0.75
// evidence: types/tags.h ScenarioBSP / TagID; types/cache.h tag_instance and the
// structure_bsp_data global; raw disassembly at 0x442520-0x442546 (objdump -d -M intel
// --start-address=0x442520 --stop-address=0x442550 bin/halo.exe) used to confirm that bsp is an
// ordinary stack parameter (read via [esp+4] after the call, unclobbered by it) while
// structure_bsp_dispose_material_vertex_buffers's argument is the structure_bsp_data global
// loaded into EAX separately, exactly as in structure_bsp_load and cache_file_unload.
// register convention: ScenarioBSP *bsp is a normal (stack) parameter.

#include "tags.h"
#include "cache.h"
#include "fn_cache.h"


    // Ghidra: FUN_004431a0; this module, 0x4431a0

extern void *structure_bsp_data;    // 0x006a8958
extern tag_instance *tag_instances; // 0x0087bc14

// Releases a structure_bsp's per-material rendering resources (through the still-resident
// structure_bsp_data, not through bsp itself) and clears both the owning tag's data pointer and
// the global. The counterpart teardown to structure_bsp_load.
void structure_bsp_dispose(ScenarioBSP *bsp)
{
    structure_bsp_dispose_material_vertex_buffers(
        (ScenarioStructureBSPCompiledHeader *)structure_bsp_data);
    tag_instances[bsp->structure_bsp.tag_id.index].data = 0;
    structure_bsp_data = 0;
}

#if 0
Original Ghidra decompilation (0x442520):

void FUN_00442520(int param_1)

{
  FUN_004431a0();
  *(undefined4 *)(*(short *)(param_1 + 0x1c) * 0x20 + 0x14 + DAT_0087bc14) = 0;
  DAT_006a8958 = 0;
  return;
}

Raw disassembly (0x442520-0x442546), objdump -d -M intel --start-address=0x442520
--stop-address=0x442550 bin/halo.exe:

00442520: mov eax,ds:0x6a8958        ; eax = structure_bsp_data
00442525: call 0x4431a0              ; structure_bsp_dispose_material_vertex_buffers(eax)
0044252a: mov eax,DWORD PTR [esp+0x4]   ; eax = bsp (stack parameter, unclobbered by the call)
0044252e: movsx ecx,WORD PTR [eax+0x1c] ; ecx = bsp->structure_bsp.tag_id.index
00442532: mov edx,ds:0x87bc14           ; edx = tag_instances
00442538: xor eax,eax
0044253a: shl ecx,0x5
0044253d: mov DWORD PTR [ecx+edx*1+0x14],eax   ; tag_instances[index].data = 0
00442541: mov ds:0x6a8958,eax               ; structure_bsp_data = 0
00442546: ret
#endif
