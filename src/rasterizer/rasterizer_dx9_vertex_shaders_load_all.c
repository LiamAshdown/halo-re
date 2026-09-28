// rasterizer_dx9_vertex_shaders_load_all  (Ghidra: rasterizer_dx9_vertex_shaders_load_all, already named)
// address 0x5306e0, size 205 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: functions.md summary ("Loads and creates all 64 precompiled vertex shaders from the
//   vsh.bin blob, tearing everything down if any of them fails to load"); the same
//   [int32 size][size bytes] chunk walk as rasterizer_dx9_pixel_shaders_load_all.c, one chunk per
//   rasterizer_vertex_shaders[] slot except slots whose `enabled` field is 0 (the blob carries no
//   chunk for those, per types/rasterizer.h's rasterizer_vertex_shader doc paragraph).
// register convention: none -- __cdecl, no arguments.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device; // 0x0071d174
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern const char *rasterizer_shader_file_name; // 0x00722bbc

// blam-cc: ECX -> path, stack -> (out_buffer, out_size)
extern uint32_t rasterizer_load_file_and_verify(void **out_buffer, uint32_t *out_size, const char *path); // 0x5199f0
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

typedef int32_t (__stdcall *d3d_create_vertex_shader_fn)(void *device, const void *function, void *out_shader);

// Loads shaders\vsh.bin, then walks it as a run of [int32 chunk_size][chunk_size bytes] records,
// creating one vertex shader per rasterizer_vertex_shaders[] slot whose `enabled` field is
// nonzero (disabled slots consume no chunk). Tears every created shader down again if the file is
// malformed or any chunk fails to create.
uint32_t rasterizer_dx9_vertex_shaders_load_all(void)
{
    void *buffer;
    uint32_t size;
    uint8_t *cursor, *end;
    void **vt;
    d3d_create_vertex_shader_fn create_vertex_shader;
    int32_t index;

    if (rasterizer_load_file_and_verify(&buffer, &size, "shaders\\vsh.bin") == 0) {
        return 0;
    }

    cursor = (uint8_t *)buffer;
    end = (uint8_t *)buffer + size;
    vt = *(void ***)rasterizer_device;
    create_vertex_shader = (d3d_create_vertex_shader_fn)vt[0x16c / 4];

    for (index = 0; index < k_rasterizer_vertex_shaders; index++) {
        if (rasterizer_vertex_shaders[index].enabled == 0) {
            continue;
        }
        {
            uint8_t *data = cursor + 4;
            int32_t chunk_size;
            int32_t hr;
            if (end < data) {
                break;
            }
            chunk_size = *(int32_t *)cursor;
            cursor = data + chunk_size;
            if (end < cursor) {
                break;
            }
            hr = create_vertex_shader(rasterizer_device, data, &rasterizer_vertex_shaders[index].shader);
            if (hr < 0) {
                break;
            }
        }
    }

    if (index < k_rasterizer_vertex_shaders) {
        int i;
        for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
            void *shader = (void *)rasterizer_vertex_shaders[i].shader;
            if (shader != 0) {
                ((void (__stdcall *)(void *))(*(void ***)shader)[2])(shader); // Release()
                rasterizer_vertex_shaders[i].shader = 0;
            }
        }
        rasterizer_shader_file_name = "shaders\\vsh.bin";
        shell_display_fatal_error_dialog(0x69, 0x7e, 1);
    }

    GlobalFree(buffer);
    return index == k_rasterizer_vertex_shaders;
}

#if 0
Original Ghidra decompilation (0x5306e0):

uint __cdecl rasterizer_dx9_vertex_shaders_load_all(void)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  int *piVar6;
  int *piVar7;
  int *local_8;
  int local_4;

  uVar2 = rasterizer_load_file_and_verify(&local_8,&local_4);
  if ((char)uVar2 == '\0') {
    return uVar2;
  }
  piVar6 = (int *)((int)local_8 + local_4);
  iVar4 = 0;
  puVar5 = &DAT_0069e350;
  piVar7 = local_8;
  while ((puVar5[1] == 0 ||
         (((piVar1 = piVar7 + 1, piVar1 <= piVar6 &&
           (piVar7 = (int *)(*piVar7 + (int)piVar1), piVar7 <= piVar6)) &&
          (iVar3 = (**(code **)(*DAT_0071d174 + 0x16c))(DAT_0071d174,piVar1,puVar5), -1 < iVar3)))))
  {
    puVar5 = puVar5 + 2;
    iVar4 = iVar4 + 1;
    if (0x69e54f < (int)puVar5) {
LAB_00530795:
      GlobalFree(local_8);
      return (uint)(iVar4 == 0x40);
    }
  }
  if (iVar4 < 0x40) {
    piVar6 = &DAT_0069e350;
    do {
      piVar7 = (int *)*piVar6;
      if (piVar7 != (int *)0x0) {
        (**(code **)(*piVar7 + 8))(piVar7);
        *piVar6 = 0;
      }
      piVar6 = piVar6 + 2;
    } while ((int)piVar6 < 0x69e550);
    DAT_00722bbc = "shaders\\vsh.bin";
    shell_display_fatal_error_dialog(0x69,0x7e,1);
  }
  goto LAB_00530795;
}
#endif
