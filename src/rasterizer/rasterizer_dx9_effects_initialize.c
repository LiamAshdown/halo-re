// rasterizer_dx9_effects_initialize  (Ghidra: rasterizer_dx9_effects_initialize, already named)
// address 0x5300d0, size 70 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: functions.md summary ("Top-level DX9 shader/effect startup routine that (re)creates
//   vertex declarations and vertex/pixel shaders and initializes the screen_effect and
//   screen-flash technique tables, in order, bailing out on the first failure").
// register convention: none -- __cdecl, no arguments.
// UNSURE: Ghidra shows the success check after rasterizer_dx9_vertex_shaders_reload() as reading
//   `extraout_AL` (a value left in AL that isn't the recognized return of that call), i.e. that
//   function does return a value despite its own summary describing it as a bare teardown+rebuild;
//   modeled here as a uint8_t return.

#include "tags.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void rasterizer_dx9_vertex_declarations_release(void);        // 0x530540
extern int32_t rasterizer_dx9_vertex_declarations_create(void);      // 0x5301b0
extern uint8_t rasterizer_dx9_vertex_shaders_reload(void);           // 0x530800
extern void rasterizer_dx9_pixel_shaders_release(void);              // 0x52ff60
extern uint8_t rasterizer_dx9_shaders_initialize(void);              // 0x52fab0
extern int32_t rasterizer_screen_effect_init_shaders(void);          // 0x52d740
extern int32_t rasterizer_screen_flash_init_shaders(void);           // 0x52ec40
extern uint8_t rasterizer_shader_environment_build_technique_table(void); // 0x526930

// Recreates vertex declarations, reloads vertex shaders, reloads pixel shaders/effects, then
// initializes the screen-effect, screen-flash and shader_environment technique tables in order,
// stopping at the first failure.
uint8_t rasterizer_dx9_effects_initialize(void)
{
    rasterizer_dx9_vertex_declarations_release();
    if (!rasterizer_dx9_vertex_declarations_create()) {
        return 0;
    }
    if (!rasterizer_dx9_vertex_shaders_reload()) {
        return 0;
    }
    rasterizer_dx9_pixel_shaders_release();
    if ((rasterizer_dx9_shaders_initialize() & 0xff) == 0) {
        return 0;
    }
    if (!rasterizer_screen_effect_init_shaders()) {
        return 0;
    }
    if (!rasterizer_screen_flash_init_shaders()) {
        return 0;
    }
    if (!rasterizer_shader_environment_build_technique_table()) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x5300d0):

char __cdecl rasterizer_dx9_effects_initialize(void)

{
  bool bVar1;
  char extraout_AL;
  char cVar2;
  uint uVar3;
  int iVar4;

  rasterizer_dx9_vertex_declarations_release();
  bVar1 = rasterizer_dx9_vertex_declarations_create();
  if (bVar1) {
    rasterizer_dx9_vertex_shaders_reload();
    if (extraout_AL != '\0') {
      rasterizer_dx9_pixel_shaders_release();
      uVar3 = rasterizer_dx9_shaders_initialize();
      if ((char)uVar3 != '\0') {
        iVar4 = rasterizer_screen_effect_init_shaders();
        if ((char)iVar4 != '\0') {
          iVar4 = rasterizer_screen_flash_init_shaders();
          if ((char)iVar4 != '\0') {
            cVar2 = rasterizer_shader_environment_build_technique_table();
            if (cVar2 != '\0') {
              return '\x01';
            }
          }
        }
      }
    }
  }
  return '\0';
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
