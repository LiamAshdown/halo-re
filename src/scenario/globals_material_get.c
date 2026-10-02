// globals_material_get  (Ghidra: FUN_0053e7c0, still unnamed there; name from
// out/phase2/results/scenario_00.json)
// address 0x53e7c0, size 71 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: out/phase4/scenario_types_notes.md: not scenario-specific -- it indexes
// global_globals->materials (Globals.materials, +0x194/+0x198, stride 0x374
// GlobalsMaterial), and physics (0x507cc0, out of this batch) inlines the same lookup; kept in
// this module because it lives here and scenario owns global_globals. Out-of-range indices
// fall back to material_table_fallback, whose melee_hit_sound.tag_id (0x006e3578 ==
// 0x006e3208 + 0x370) is set to "no tag" on first use, latched by material_table_warning_issued
// (0x00721e4c, owned by types/physics.h).
// register convention: raw disassembly (0x53e7c0-0x53e806) shows AX tested and moved into EAX
// with no prior register setup in this function, so it is the material index parameter; there
// are no stack parameters. Return is a GlobalsMaterial * in EAX.
//   // blam-cc: AX -> material_index; return in EAX

#include "tags.h"
#include "scenario.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Globals *global_globals;              // 0x00746fa0
extern uint8_t material_table_warning_issued;      // 0x00721e4c, owned by types/physics.h
extern GlobalsMaterial material_table_fallback;   // 0x006e3208

// blam-cc: AX -> material_index
// Returns a pointer to global_globals->materials[material_index], or, when the index is
// out of range, a pointer to the static default material (clearing its melee_hit_sound tag
// reference the first time this happens).
GlobalsMaterial *globals_material_get(int16_t material_index)
{
    GlobalsMaterial *materials;

    if (0 <= material_index && (int32_t)material_index < (int32_t)global_globals->materials.count) {
        materials = (GlobalsMaterial *)global_globals->materials.pointer;
        return &materials[material_index];
    }

    if (material_table_warning_issued == 0) {
        *(uint32_t *)&material_table_fallback.melee_hit_sound.tag_id = 0xffffffff;
        material_table_warning_issued = 1;
    }
    return &material_table_fallback;
}

#if 0
Original Ghidra decompilation (0x53e7c0):

undefined * FUN_0053e7c0(void)

{
  short in_AX;

  if ((-1 < in_AX) && ((int)in_AX < *(int *)(DAT_00746fa0 + 0x194))) {
    return (undefined *)(in_AX * 0x374 + *(int *)(DAT_00746fa0 + 0x198));
  }
  if (DAT_00721e4c == '\0') {
    DAT_006e3578 = 0xffffffff;
    DAT_00721e4c = '\x01';
  }
  return &DAT_006e3208;
}

Raw disassembly (0x53e7c0-0x53e806):

  53e7c0: test   ax,ax
  53e7c3: mov    ecx,DWORD PTR ds:0x746fa0
  53e7c9: jl     0x53e7e7
  53e7cb: mov    edx,DWORD PTR [ecx+0x194]      ; materials.count
  53e7d1: movsx  eax,ax
  53e7d4: cmp    eax,edx
  53e7d6: jge    0x53e7e7
  53e7d8: mov    edx,DWORD PTR [ecx+0x198]      ; materials.pointer
  53e7de: imul   eax,eax,0x374
  53e7e4: add    eax,edx
  53e7e6: ret
  53e7e7: mov    al,ds:0x721e4c
  53e7ec: test   al,al
  53e7ee: jne    0x53e801
  53e7f0: mov    DWORD PTR ds:0x6e3578,0xffffffff
  53e7fa: mov    BYTE PTR ds:0x721e4c,0x1
  53e801: mov    eax,0x6e3208
  53e806: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
