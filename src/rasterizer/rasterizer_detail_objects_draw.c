// rasterizer_detail_objects_draw  (Ghidra: FUN_0051b890, unnamed)
// address 0x51b890, size 761 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: for every batch of the list it resolves Scenario.detail_object_collection_palette
//   [batch.collection_palette_index] (global_scenario +0x3c4, 0x30 byte elements, tag at +0xc)
//   to a DetailObjectCollection, binds its sprite_plate bitmap (+0x40 tag id) through
//   chimera__rasterizer_set_texture, uploads one vec4 per collection type at vertex shader
//   register 0x13 (fade scale from near/far fade distance, and the first sequence bitmap's size
//   times the type size) and one vec4 per sprite of every sequence of the sprite plate at
//   register 0x1d (left, top, width, height), then draws the batch's runs out of the detail
//   object vertex buffer as triangle lists with the detail_object declaration (type 11) and
//   vertex shader 3 + collection_type. The phase 2 summary called this a static decal batch.
//   Ghidra lost the batch pointer in the draw loop; the draw loop follows the raw code
//   0x51bb14..0x51bb4c.
// register convention: __cdecl, list pointer on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                                     // 0x0071d174
extern Scenario *global_scenario;                                   // 0x00746f8c
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern uint8_t console_debug_toggle_689404;                         // 0x00689404 detail objects enable

extern int16_t render_local_view_count(void);                                  // 0x4c9220, UNSURE: local player count
// blam-cc: EAX -> bitmap_tag_id, the rest on the stack
extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
                                                int16_t default_index, int16_t frame); // 0x518960

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

void rasterizer_detail_objects_draw(const rasterizer_detail_object_batches *list)
{
    int16_t batch_index;

    if (console_debug_toggle_689404 == 0 || render_local_view_count() > 1) {
        return;
    }

    for (batch_index = 0; batch_index < list->batch_count; batch_index++) {
        const rasterizer_detail_object_batch *batch = &((const rasterizer_detail_object_batch *)list->batches)[batch_index];
        const uint8_t *palette = (const uint8_t *)global_scenario->detail_object_collection_palette.pointer;
        uint32_t collection_tag = *(const uint32_t *)(palette + batch->collection_palette_index * 0x30 + 0xc);
        const DetailObjectCollection *collection = (const DetailObjectCollection *)tag_instances[collection_tag & 0xffff].data;
        uint32_t sprite_plate_tag = *(const uint32_t *)&collection->sprite_plate.tag_id;
        const Bitmap *sprite_plate = (const Bitmap *)tag_instances[sprite_plate_tag & 0xffff].data;
        float type_constants[16][4];                            // at most 16 types (0x100 bytes)
        float sprite_constants[128][4];                         // at most 128 sprites (0x800 bytes)
        int32_t type_count;
        int16_t type_index;
        int16_t sprite_count = 0;
        int16_t sequence_index;
        int16_t draw_index;

        chimera__rasterizer_set_texture(sprite_plate_tag, 0, 0, 1, 0);

        // c19..: per type fade and size
        type_count = (int32_t)collection->types.count;
        for (type_index = 0; type_index < type_count; type_index++) {
            const DetailObjectCollectionObjectType *type =
                &((const DetailObjectCollectionObjectType *)collection->types.pointer)[type_index];
            const BitmapGroupSequence *sequence =
                &((const BitmapGroupSequence *)sprite_plate->bitmap_group_sequence.pointer)[type->sequence_index];
            const BitmapData *first_bitmap =
                &((const BitmapData *)sprite_plate->bitmap_data.pointer)[(int16_t)sequence->first_bitmap_index];
            float fade_scale = type->far_fade_distance - type->near_fade_distance;

            if (fade_scale > 0.0f) {
                fade_scale = 1.0f / fade_scale;
            }
            type_constants[type_index][0] = fade_scale * type->far_fade_distance;
            type_constants[type_index][1] = -fade_scale;
            type_constants[type_index][2] = (float)(int16_t)first_bitmap->width * type->size;
            type_constants[type_index][3] = (float)(int16_t)first_bitmap->height * type->size;
        }

        // c29..: every sprite rectangle of every sequence, in order
        for (sequence_index = 0; sequence_index < (int32_t)sprite_plate->bitmap_group_sequence.count; sequence_index++) {
            const BitmapGroupSequence *sequence = &((const BitmapGroupSequence *)sprite_plate->bitmap_group_sequence.pointer)[sequence_index];
            int16_t sprite_index;

            for (sprite_index = 0; sprite_index < (int32_t)sequence->sprites.count; sprite_index++) {
                const BitmapGroupSprite *sprite = &((const BitmapGroupSprite *)sequence->sprites.pointer)[sprite_index];

                sprite_constants[sprite_count][0] = sprite->left;
                sprite_constants[sprite_count][1] = sprite->top;
                sprite_constants[sprite_count][2] = sprite->right - sprite->left;
                sprite_constants[sprite_count][3] = sprite->bottom - sprite->top;
                sprite_count++;
            }
        }

        ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0x13, &type_constants[0][0], (uint32_t)type_count);
        ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0x1d, &sprite_constants[0][0], (uint32_t)sprite_count);
        ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(
            rasterizer_device, (void *)rasterizer_vertex_declarations[_rasterizer_vertex_type_detail_object].declaration);
        ((d3d_call1_fn)device_vtable()[0x134 / 4])(
            rasterizer_device, ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                rasterizer_vertex_declarations[_rasterizer_vertex_type_detail_object].usage) & 0x10);
        ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(
            rasterizer_device, (void *)rasterizer_vertex_shaders[3 + collection->collection_type].shader);

        for (draw_index = 0; draw_index < batch->draw_count; draw_index++) {
            const rasterizer_detail_object_draw *draw = &((const rasterizer_detail_object_draw *)batch->draws)[draw_index];

            ((d3d_call3_fn)device_vtable()[0x144 / 4])(rasterizer_device, 4, (uint32_t)draw->first_vertex,
                                                       (uint32_t)(draw->quad_count * 2)); // DrawPrimitive list
        }
        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
    }
}

#if 0
Original Ghidra decompilation (0x51b890):

void FUN_0051b890(int *param_1)

{
  float fVar1;
  short *psVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  float fVar6;
  int iVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  short sVar14;
  short sVar15;
  short sVar16;
  float afStackY_80800 [130978];
  int *piVar17;
  float local_900 [60];
  undefined1 auStack_810 [16];
  float local_800 [512];
  
  if (((DAT_00689404 != '\0') && (sVar8 = FUN_004c9220(), iVar7 = global_scenario, sVar8 < 2)) &&
     (sVar8 = 0, 0 < (short)param_1[1])) {
    do {
      psVar2 = *(short **)
                ((*(uint *)(*(short *)(*param_1 + sVar8 * 8 + 6) * 0x30 + 0xc +
                           *(int *)(iVar7 + 0x3c4)) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      iVar3 = *(int *)((*(uint *)(psVar2 + 0x20) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      chimera__rasterizer_set_texture(0,0,1,0);
      iVar4 = *(int *)(psVar2 + 0x22);
      sVar16 = 0;
      sVar15 = 0;
      if (0 < iVar4) {
        iVar5 = *(int *)(iVar3 + 0x58);
        iVar12 = *(int *)(iVar3 + 100);
        iVar13 = 0;
        do {
          iVar9 = iVar13 * 0x60 + *(int *)(psVar2 + 0x24);
          fVar6 = *(float *)(iVar13 * 0x60 + 0x34 + *(int *)(psVar2 + 0x24)) -
                  *(float *)(iVar9 + 0x30);
          fVar1 = *(float *)(iVar9 + 0x38);
          iVar11 = *(short *)((uint)*(byte *)(iVar9 + 0x20) * 0x40 + 0x20 + iVar5) * 0x30 + iVar12;
          if (0.0 < fVar6) {
            fVar6 = 1.0 / fVar6;
          }
          sVar15 = sVar15 + 1;
          local_900[iVar13 * 4] = fVar6 * *(float *)(iVar9 + 0x34);
          local_900[iVar13 * 4 + 1] = -fVar6;
          local_900[iVar13 * 4 + 2] = (float)(int)*(short *)(iVar11 + 4) * fVar1;
          local_900[iVar13 * 4 + 3] = (float)(int)*(short *)(iVar11 + 6) * fVar1;
          iVar13 = (int)sVar15;
        } while (sVar15 < iVar4);
      }
      iVar5 = *(int *)(iVar3 + 0x54);
      iVar12 = 0;
      sVar15 = 0;
      if (0 < iVar5) {
        iVar3 = *(int *)(iVar3 + 0x58);
        do {
          iVar13 = iVar12 * 0x40 + iVar3;
          sVar14 = 0;
          if (0 < *(int *)(iVar12 * 0x40 + 0x34 + iVar3)) {
            iVar12 = 0;
            do {
              iVar9 = *(int *)(iVar13 + 0x38);
              iVar11 = iVar12 * 0x20 + iVar9;
              iVar10 = (int)sVar16;
              local_800[iVar10 * 4] = *(float *)(iVar12 * 0x20 + 8 + iVar9);
              local_800[iVar10 * 4 + 1] = *(float *)(iVar11 + 0x10);
              sVar16 = sVar16 + 1;
              sVar14 = sVar14 + 1;
              local_800[iVar10 * 4 + 2] = *(float *)(iVar11 + 0xc) - *(float *)(iVar11 + 8);
              iVar12 = (int)sVar14;
              local_800[iVar10 * 4 + 3] = *(float *)(iVar11 + 0x14) - *(float *)(iVar11 + 0x10);
            } while (iVar12 < *(int *)(iVar13 + 0x34));
          }
          sVar15 = sVar15 + 1;
          iVar12 = (int)sVar15;
        } while (iVar12 < iVar5);
      }
      piVar17 = (int *)0x13;
      (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0x13,local_900,iVar4);
      (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0x1d,auStack_810,(int)sVar16);
      (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1b14);
      (**(code **)(*DAT_0071d174 + 0x134))
                (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1b1c & 0x10);
      (**(code **)(*DAT_0071d174 + 0x170))
                (DAT_0071d174,*(undefined4 *)(&DAT_0069e368 + *psVar2 * 8));
      sVar15 = 0;
      if (0 < (short)piVar17[1]) {
        do {
          (**(code **)(*DAT_0071d174 + 0x144))
                    (DAT_0071d174,4,*(undefined4 *)(*piVar17 + sVar15 * 0x18 + 0x10),
                     *(int *)(*piVar17 + 4 + sVar15 * 0x18) << 1);
          sVar15 = sVar15 + 1;
        } while (sVar15 < (short)piVar17[1]);
      }
      (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
      sVar8 = sVar8 + 1;
    } while (sVar8 < (short)param_1[1]);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
