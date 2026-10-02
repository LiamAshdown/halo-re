// decal_geometry_cache_restore_procs  (not a Ghidra function; game state after-load proc 5)
// address 0x511ed0, size 20 bytes
// name confidence: 0.4  rewrite confidence: 0.95
// evidence: game_state_after_load_procs[5] (0x0069e7c8) holds 0x511ed0. The loaded game state holds the
//   decal geometry cache header, so its two procedure slots are re-pointed at this build's code.
// objdump 0x511ed0: mov eax,ds:0x71d1c0 / mov [eax+0x20],0x51a660 (decal_vertex_cache_release) /
//   mov [eax+0x24],0x51a670 (decal_vertex_cache_in_use) / ret
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *rasterizer_decal_vertex_cache_handle; // 0x0071d1c0

extern void decal_vertex_cache_release(void); // 0x51a660
extern void decal_vertex_cache_in_use(void);  // 0x51a670

void decal_geometry_cache_restore_procs(void)
{
    *(void **)(rasterizer_decal_vertex_cache_handle + 0x20) = (void *)decal_vertex_cache_release;
    *(void **)(rasterizer_decal_vertex_cache_handle + 0x24) = (void *)decal_vertex_cache_in_use;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
