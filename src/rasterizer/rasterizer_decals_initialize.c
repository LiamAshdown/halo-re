// rasterizer_decals_initialize  (Ghidra: rasterizer_decals_initialize, already named --
// confirmed by a CEA PDB symbol match on the "decal vertex cache" string)
// address 0x51a6a0, size 201 bytes
// name confidence: 0.9    rewrite confidence: 0.9
// evidence: CreateVertexBuffer (device vtable +0x68) for the decal dynamic vertex buffer, then
//   registers a 0xe07c byte region with crc32_update and cache_new for the "decal vertex cache".
// register convention: none -- __cdecl, no parameters.
// Spot-check fix (phase 4 review, raw code 0x51a6a0..0x51a768): the buffer global 0x0071d1bc
//   receives CreateVertexBuffer's out pointer (NULL on failure), not the usage mask; the pool is
//   2 when the usage has SOFTWAREPROCESSING or WRITEONLY, else 1 (the same inline helper as
//   0x51b370); the cache handle 0x0071d1c0 is the game state block itself (ESI), handed to
//   cache_new with the "decal vertex cache" name in EBX (0x0066ede8). The two LAB_ addresses
//   passed to cache_new are callback function pointers whose real signatures are unknown.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern void *rasterizer_device;   // 0x0071d174
extern void *rasterizer_decal_vertex_cache; // 0x0071d1bc
extern uint8_t *game_state_base;                                    // 0x006e2dc8, game.h (saved_games)
extern int32_t game_state_cursor;                                   // 0x006e2dcc, game.h (saved_games)
extern uint32_t game_state_crc;                                     // 0x006e2dd4, game.h (saved_games)
extern uint8_t *rasterizer_decal_vertex_cache_handle;               // 0x0071d1c0 UNSURE: +0x2c shift, +0x3c data_array

extern void crc32_update(uint32_t *crc, const void *data, uint32_t length); // 0x4d02d0

typedef int32_t (__stdcall *d3d_create_vertex_buffer_fn)(void *device, uint32_t length, uint32_t usage, uint32_t fvf,
                                                 uint32_t pool, void **out_buffer, void *shared_handle);

extern void LAB_0051a660(void); // decal vertex cache load callback, UNSURE signature
extern void LAB_0051a670(void); // decal vertex cache verify callback, UNSURE signature

// blam-cc: EBX -> name for cache_new
extern void *cache_new(uint8_t *block, int32_t a, int32_t b, int32_t c, void *load_fn, void *verify_fn, const char *name); // 0x4d1750, UNSURE signature

void __cdecl rasterizer_decals_initialize(void)
{
    void *buffer = 0;
    uint32_t usage = (rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                     rasterizer_vertex_declarations[_rasterizer_vertex_type_decal].usage | 0x200;
    uint32_t pool = (usage & 0x10) != 0 || (usage & 0x200) != 0 ? 2 : 1;
    uint32_t region_size = 0xe07c;
    uint8_t *block;
    void **vtable = *(void ***)rasterizer_device;
    int32_t hr = ((d3d_create_vertex_buffer_fn)vtable[0x68 / 4])(rasterizer_device, 0x3c000, usage, 0, pool, &buffer, 0);

    rasterizer_decal_vertex_cache = hr < 0 ? 0 : buffer;

    // game_state_malloc(0xe07c), inlined
    block = game_state_base + game_state_cursor;
    game_state_cursor = game_state_cursor + 0xe07c;
    crc32_update(&game_state_crc, &region_size, 4);
    cache_new(block, 0xa00, 6, 0x800, (void *)LAB_0051a660, (void *)LAB_0051a670, "decal vertex cache");
    rasterizer_decal_vertex_cache_handle = block;
}


#if 0
Original Ghidra decompilation (0x51a6a0):

/* rasterizer_decals_initialize */

void __cdecl rasterizer_decals_initialize(void)

{
  uint uVar1;
  int iVar2;
  uint auStack_20 [2];
  undefined4 *puStack_18;
  undefined4 uStack_14;
  undefined4 local_4;

  local_4 = 0;
  uStack_14 = 0;
  puStack_18 = &local_4;
  auStack_20[1] = 2;
  auStack_20[0] = 0;
  iVar2 = (**(code **)(*DAT_0071d174 + 0x68))
                    (DAT_0071d174,0x3c000,
                     -(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1b10 | 0x200);
  uVar1 = auStack_20[0];
  auStack_20[0] = 0xe07c;
  DAT_0071d1bc = (iVar2 < 0) - 1 & uVar1;
  iVar2 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0xe07c;
  crc32_update(&DAT_006e2dd4,auStack_20,4);
  cache_new(iVar2,0xa00,6,0x800,&LAB_0051a660,&LAB_0051a670);
  DAT_0071d1c0 = iVar2;
  return;
}
#endif
