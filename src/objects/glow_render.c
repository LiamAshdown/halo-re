// glow_render
// address 0x4fe570, size 259 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fe570 |
//   lightning_render | glow_render")
// rewrite confidence: 0.85 (REWRITTEN from objdump, see below)
// evidence: types/objects.h globals list (glow_data 0x008603a0), glow_particle (t 0x28 doubles
//   as the vertex-position argument here via +0x2c, flags 0x54, next 0x5c); types/memory.h
//   data_array (size 0x22, last_index 0x2e, data 0x34); resolved against
//   glow_render_dispatch.c's call site (`mov ecx,esi; call 0x4fe570`), where esi carries the
//   full packed glow_handle (index low 16, salt high 16), the same shape this function itself
//   validates against.
// register convention: `in_ECX` unresolved by Ghidra; ECX -> glow_handle per the call site.
// blam-cc: ECX -> glow_handle

// FIXED 2026-09-28: global_zero_vector3d_pointer here is the global at its address comment, global_origin3d_pointer (the name belonged to another
// global at a different address, so the link bound it there).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "rasterizer.h"
#include "render.h"
#include <string.h>

extern data_array *glow_data; // 0x008603a0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_point3d *global_zero_vector3d_pointer; // 0x006966f8
extern uint8_t glow_sprite_shader[]; // 0x0069e940

extern void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index, int16_t mode,
    real_point3d *origin, real_vector3d *direction, float rotation, float scale, ColorARGB *color, float fade,
    uint32_t flags); // 0x511700, EBX, AX, CX, stack
extern void build_sprites_end(build_sprite_data *data); // 0x511620, ESI

// REWRITTEN from objdump 0x4fe570..0x4fe672: the draft never filled the sprite batch (bitmap from the glow tag
//   +0x150, sprite cap +0x24c, the glow shader 0x69e940, flags 4, centroid at the origin) and called build_sprite
//   and build_sprites_end without it. Each particle of the glow is one sprite at its position (+0x2c) along its
//   marker (+0x2 into the 0x6c-byte marker array at +0x44), scale +0x24, colour +0xc, fade +0x58.
void glow_render(datum_index glow_handle /*ECX*/) // blam-cc: ECX -> glow_handle
{
    uint8_t *entry = 0;
    build_sprite_data data;         // [esp+0x8]
    uint8_t *particle;
    int16_t index = (int16_t)glow_handle;
    int16_t salt = (int16_t)(glow_handle >> 16);

    if (index >= 0 && index < *(int16_t *)((uint8_t *)glow_data + 0x2e)) {
        uint8_t *candidate = (uint8_t *)glow_data->data + *(int16_t *)((uint8_t *)glow_data + 0x22) * index;

        if (*(int16_t *)candidate != 0 && (salt == 0 || salt == *(int16_t *)candidate)) {
            entry = candidate;
        }
    }
    // (with an invalid handle the binary reads through a null glow; nothing is drawn here)
    if (entry == 0) {
        return;
    }
    memset(&data, 0, sizeof(data));
    data.bitmap_group_index = *(datum_index *)((uint8_t *)tag_instances[*(datum_index *)(entry + 0x224) & 0xffff].data + 0x150);
    data.maximum_sprite_count = *(int16_t *)(entry + 0x24c);
    data.shader = (uint32_t)glow_sprite_shader;
    data.sprite_count = 0;
    data.flags = 4;
    data.centroid = *global_zero_vector3d_pointer;
    data.group_count = 0;
    for (particle = *(uint8_t **)(entry + 0x250); particle != 0; particle = *(uint8_t **)(particle + 0x5c)) {
        build_sprite(&data, 0, 0, 0, (real_point3d *)(particle + 0x2c),
                     (real_vector3d *)(entry + *(int16_t *)(particle + 0x2) * 0x6c + 0x44), 0.0f,
                     *(float *)(particle + 0x24), (ColorARGB *)(particle + 0xc), *(float *)(particle + 0x58), 0);
    }
    build_sprites_end(&data);
}

#if 0
Original Ghidra decompilation (0x4fe570):

void lightning_render(void)

{
  int iVar1;
  int iVar2;
  short sVar3;
  undefined4 in_ECX;
  short sVar4;

  sVar3 = (short)in_ECX;
  if ((-1 < sVar3) && (sVar3 < *(short *)(DAT_008603a0 + 0x2e))) {
    iVar2 = (int)*(short *)(DAT_008603a0 + 0x22) * (int)sVar3;
    sVar3 = *(short *)(iVar2 + *(int *)(DAT_008603a0 + 0x34));
    iVar2 = iVar2 + *(int *)(DAT_008603a0 + 0x34);
    if ((sVar3 != 0) && ((sVar4 = (short)((uint)in_ECX >> 0x10), sVar4 == 0 || (sVar4 == sVar3))))
    goto LAB_004fe5b6;
  }
  iVar2 = 0;
LAB_004fe5b6:
  for (iVar1 = *(int *)(iVar2 + 0x250); iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x5c)) {
    render_billboard_quad_build
              (0,iVar1 + 0x2c,*(short *)(iVar1 + 2) * 0x6c + 0x44 + iVar2,0,
               *(undefined4 *)(iVar1 + 0x24),iVar1 + 0xc,*(undefined4 *)(iVar1 + 0x58),0);
  }
  FUN_00511620();
  return;
}
#endif
