// structure_bsp_load  (Ghidra: FUN_004424b0)
// address 0x4424b0, size 108 bytes
// name confidence: 0.65 (out/phase4/cache_functions.md summary: "Synchronously loads a
// structure_bsp tag's data block from the map file, installs the buffer as the tag's data
// pointer, and creates its shader rendering resources"; the ScenarioBSP fields it reads match
// types/cache.h's evidence table exactly)
// rewrite confidence: 0.70
// evidence: types/tags.h ScenarioBSP (bsp_start/bsp_size/bsp_address/structure_bsp.tag_id),
// TagID.index; types/cache.h tag_instance, cache_io_completion, globals list
// (structure_bsp_data == the ScenarioStructureBSPCompiledHeader of the resident bsp); the sister
// function structure_bsp_dispose (0x442520) confirms the same ScenarioBSP* parameter and the
// same tag_instances[index].data write.
// register convention: ScenarioBSP *bsp in EDI (unaff_EDI). The cache_io_completion built on the
// stack is passed to cache_io_request_new by pointer in ESI, as elsewhere in this module.
// UNSURE: the wait loop here is a bare busy-spin (`do {} while (!flag)`), unlike cache_file_load's
// equivalent loop, which calls Sleep(0). Preserved exactly; not an "improvement" opportunity.
// UNSURE: the return value's upper 24 bits come from `uVar1 >> 8` in the original (the high bytes
// of the compiled header's `pointer` field with the low byte replaced by 1); nothing in the one
// known caller inspects more than the low byte, so this is reproduced as a plain boolean success
// return instead of reconstructing that upper-byte garbage.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "cache.h"
#include "fn_cache.h"

extern int16_t cache_io_request_new(cache_io_completion *completion, // blam-cc: ESI
    int32_t offset, uint32_t size, void *destination, uint8_t priority,
    uint8_t data_file_index); // this module, 0x442b20

    // 0x443020, called with structure_bsp_data already set as its EAX argument (see that file)

extern void *structure_bsp_data;            // 0x006a8958
extern tag_instance *tag_instances;          // 0x0087bc14

// blam-cc: bsp in EDI
// Synchronously reads a structure_bsp tag's compiled data block (bsp->bsp_start /
// bsp->bsp_size bytes) from the map file straight into its fixed load address (bsp->bsp_address),
// installs that address as structure_bsp_data and as the owning tag's data pointer, and builds
// the tag's per-material shader rendering resources.
uint32_t structure_bsp_load(ScenarioBSP *bsp)
{
    cache_io_completion completion;
    uint8_t completion_flag;
    ScenarioStructureBSPCompiledHeader *header;

    completion_flag = 0;
    completion.flag = &completion_flag;
    completion.procedure = 0;
    completion.data = 0;
    cache_io_request_new(&completion, bsp->bsp_start, bsp->bsp_size,
        (void *)bsp->bsp_address, 1, 0);
    while (completion_flag == 0) {
        // UNSURE: busy-spin, no Sleep -- see file header
    }

    structure_bsp_data = (void *)bsp->bsp_address;
    // `mov eax,[edi+0x8]` / `mov ds:0x6a8958,eax` immediately before the call at 0x004424f8:
    // the callee reads its header through EAX, not through the global it was just stored in.
    structure_bsp_load_material_vertex_buffers(
        (ScenarioStructureBSPCompiledHeader *)structure_bsp_data);

    header = (ScenarioStructureBSPCompiledHeader *)structure_bsp_data;
    tag_instances[bsp->structure_bsp.tag_id.index].data = (void *)header->pointer;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4424b0):

undefined4 FUN_004424b0(void)

{
  undefined4 uVar1;
  undefined4 *unaff_EDI;
  char local_d;
  char *local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_c = &local_d;
  local_8 = 0;
  local_4 = 0;
  cache_io_request_new(*unaff_EDI,unaff_EDI[1],unaff_EDI[2],1,0);
  do {
  } while (local_d == '\0');
  DAT_006a8958 = (undefined4 *)unaff_EDI[2];
  FUN_00443020();
  uVar1 = *DAT_006a8958;
  *(undefined4 *)(*(short *)(unaff_EDI + 7) * 0x20 + 0x14 + DAT_0087bc14) = uVar1;
  return CONCAT31((int3)((uint)uVar1 >> 8),1);
}
#endif
