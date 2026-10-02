// rasterizer_dx9_pixel_shaders_load_all  (Ghidra: rasterizer_dx9_pixel_shaders_load_all, already named)
// address 0x52fa00, size 168 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: functions.md summary ("Loads and initializes all 122 compiled pixel-shader effect
//   chunks from the fx.bin blob, tearing everything down again if any chunk fails to load or
//   initialize."); walks a buffer of [int32 size][size bytes of data] chunks, one per
//   rasterizer_effects[] slot, exactly k_rasterizer_pixel_shader_effects (0x7a) times.
// register convention: none -- __cdecl, no arguments.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410

// blam-cc: ECX -> path, stack -> (out_buffer, out_size)
extern uint32_t rasterizer_load_file_and_verify(void **out_buffer, uint32_t *out_size, const char *path); // 0x5199f0
// blam-cc: EAX -> effect_index, stack -> (data, size)
extern int32_t rasterizer_dx9_pixel_shader_effect_load(int32_t effect_index, const void *data, uint32_t size); // 0x52f980
// blam-cc: EAX -> effect_index
extern int32_t rasterizer_dx9_shaders_init_effect(int32_t effect_index); // 0x52f780

// Loads shaders\fx.bin, then walks it as a run of [int32 chunk_size][chunk_size bytes] records,
// compiling and initializing one rasterizer_effects[] slot per chunk. If the file is malformed
// (a chunk runs past the end of the buffer) or any chunk fails to load or initialize, every effect
// slot filled so far is released and cleared and the whole load is reported as a failure.
uint8_t rasterizer_dx9_pixel_shaders_load_all(void)
{
    void *buffer;
    uint32_t size;
    uint8_t *cursor;
    uint8_t *end;
    int32_t index;

    if (rasterizer_load_file_and_verify(&buffer, &size, "shaders\\fx.bin") == 0) {
        return 0;
    }

    cursor = (uint8_t *)buffer;
    end = (uint8_t *)buffer + size;
    for (index = 0; index < k_rasterizer_pixel_shader_effects; ) {
        uint8_t *data;
        int32_t chunk_size;

        data = cursor + 4;
        if (end < data) {
            break;
        }
        chunk_size = *(int32_t *)cursor;
        cursor = data + chunk_size;
        if (end < cursor || !rasterizer_dx9_pixel_shader_effect_load(index, data, chunk_size) ||
            !rasterizer_dx9_shaders_init_effect(index)) {
            break;
        }
        index++;
    }

    if (index < k_rasterizer_pixel_shader_effects) {
        int i;
        for (i = 0; i < k_rasterizer_pixel_shader_effects; i++) {
            void *effect = (void *)rasterizer_effects[i].effect;
            if (effect != 0) {
                ((void (__stdcall *)(void *))(*(void ***)effect)[2])(effect); // Release()
                rasterizer_effects[i].effect = 0;
            }
        }
    }

    GlobalFree(buffer);
    return index == k_rasterizer_pixel_shader_effects;
}

#if 0
Original Ghidra decompilation (0x52fa00):

uint __cdecl rasterizer_dx9_pixel_shaders_load_all(void)

{
  int iVar1;
  int *piVar2;
  char cVar3;
  uint uVar4;
  int *piVar5;
  int iVar6;
  int *local_8;
  int local_4;

  uVar4 = rasterizer_load_file_and_verify(&local_8,&local_4);
  if ((char)uVar4 == '\0') {
    return uVar4;
  }
  iVar6 = 0;
  piVar5 = local_8;
  do {
    piVar2 = piVar5 + 1;
    if ((int *)(local_4 + (int)local_8) < piVar2) {
LAB_0052fa6a:
      if (iVar6 < 0x7a) {
        piVar5 = &DAT_0069d410;
        do {
          piVar2 = (int *)*piVar5;
          if (piVar2 != (int *)0x0) {
            (**(code **)(*piVar2 + 8))(piVar2);
            *piVar5 = 0;
          }
          piVar5 = piVar5 + 8;
        } while ((int)piVar5 < 0x69e350);
      }
      break;
    }
    iVar1 = *piVar5;
    piVar5 = (int *)(iVar1 + (int)piVar2);
    if ((((int *)(local_4 + (int)local_8) < piVar5) ||
        (cVar3 = rasterizer_dx9_pixel_shader_effect_load(piVar2,iVar1), cVar3 == '\0')) ||
       (cVar3 = rasterizer_dx9_shaders_init_effect(), cVar3 == '\0')) goto LAB_0052fa6a;
    iVar6 = iVar6 + 1;
  } while (iVar6 < 0x7a);
  GlobalFree(local_8);
  return (uint)(iVar6 == 0x7a);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
