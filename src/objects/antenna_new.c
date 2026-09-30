// antenna_new  (Ghidra: antenna_new, already named)
// address 0x4faa90, size 487 bytes
// name confidence: 0.55 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Creates a new antenna instance for an attachment, laying out its
//   initial chain of segment vertices from the antenna tag definition")
// rewrite confidence: 0.35 (zero recorded callers in this module; the bitmap-sequence texture
//   scale computation at the end uses several Bitmap/BitmapGroupSequence/BitmapGroupSprite
//   offsets this pass did not fully verify against types/tags.h -- see UNSURE below)
// evidence: types/objects.h antenna (degenerate 0x05, definition_tag 0x08, object_index 0x0c,
//   previous_marker_position 0x10, vertices 0x1c stride 0x20), antenna_vertex (position 0x00,
//   velocity 0x0c, texture_scale 0x18); types/tags.h Antenna (bitmaps TagDependency,
//   vertices TagReflexive 0xc4), AntennaVertex (length 0x24, sequence_index 0x28, offset
//   Point3D 0x74); global 0x0087bc14 tag_instances, 0x008603ac antenna_data; callees datum_new
//   (established), bitmap_group_get_bitmap_data (0x43f250, foreign module).
// register convention: antenna tag as the sole parameter, per Ghidra's own
//   "antenna_new(uint param_1)" with no in_REG markers.
// UNSURE: the texture_scale divisor (BitmapGroupSprite.right - .left, times a bitmap width,
//   minus twice a Bitmap+0x50 field) is preserved as raw offsets rather than fully named
//   struct fields; datum_new's 64-bit {handle, array} return is modeled here using antenna_data
//   directly for the element base, matching the house style used elsewhere in this codebase
//   (e.g. src/objects/light_new_attached.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *antenna_data; // 0x008603ac
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index datum_new(data_array *array); // memory module, 0x4d0480
extern void *bitmap_group_get_bitmap_data(void); // 0x43f250, UNSURE: signature guessed, see file header

datum_index antenna_new(datum_index antenna_tag)
{
    datum_index handle = k_datum_index_none;

    if (antenna_tag != k_datum_index_none) {
        Antenna *tag = (Antenna *)tag_instances[antenna_tag & 0xffff].data;
        handle = datum_new(antenna_data);

        if (handle != k_datum_index_none) {
            antenna *ant = (antenna *)antenna_data->data + (handle & 0xffff);
            int32_t tag_vertex_count = tag->vertices.count;
            AntennaVertex *tag_vertices = (AntennaVertex *)tag->vertices.pointer;
            real_point3d position = { 0.0f, 0.0f, 0.0f };
            int32_t i;

            ant->unknown_04 = 0;
            ant->degenerate = tag_vertex_count < 2;
            ant->definition_tag = antenna_tag;
            ant->object_index = k_datum_index_none;
            ant->update_counter = 0;
            ant->previous_marker_position.x = 0.0f;
            ant->previous_marker_position.y = 0.0f;
            ant->previous_marker_position.z = 0.0f;

            for (i = 0; i < tag_vertex_count; i++) {
                antenna_vertex *vertex = &ant->vertices[i];
                AntennaVertex *tag_vertex = &tag_vertices[i];

                vertex->position = position;
                vertex->velocity.i = 0.0f;
                vertex->velocity.j = 0.0f;
                vertex->velocity.k = 0.0f;
                vertex->texture_scale = 0.0f;
                vertex->step_count = 0;

                if (tag->bitmaps.tag_id.index != 0xffff) {
                    Bitmap *bitmap = (Bitmap *)tag_instances[tag->bitmaps.tag_id.index & 0xffff].data;
                    int16_t sequence_index = tag_vertex->sequence_index;

                    if ((sequence_index >= 0) && (sequence_index < (int16_t)bitmap->bitmap_group_sequence.count)) {
                        BitmapGroupSequence *sequence = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer + sequence_index;
                        if (sequence->sprites.count != 0) {
                            BitmapGroupSprite *sprite = (BitmapGroupSprite *)sequence->sprites.pointer;
                            void *bitmap_data = bitmap_group_get_bitmap_data();
                            if (bitmap_data != 0) {
                                // UNSURE: raw offsets, see file header
                                float width = (float)*(int16_t *)((uint8_t *)bitmap_data + 4);
                                float spacing = (float)*(int16_t *)&((struct Bitmap *)bitmap)->sprite_spacing;
                                vertex->texture_scale = tag_vertex->length /
                                    (((sprite->right - sprite->left) * width - (spacing + spacing)) - 1.0f);
                            }
                        }
                    }
                }

                position.x += tag_vertex->offset.x;
                position.y += tag_vertex->offset.y;
                position.z += tag_vertex->offset.z;
            }

            {
                antenna_vertex *tip = &ant->vertices[tag_vertex_count];
                tip->position = position;
                tip->velocity.i = 0.0f;
                tip->velocity.j = 0.0f;
                tip->velocity.k = 0.0f;
            }
        }
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x4faa90):

uint antenna_new(uint param_1)

{
  float *pfVar1;
  short sVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  undefined8 uVar11;
  float local_c;
  float local_8;
  float local_4;

  uVar4 = 0xffffffff;
  if (param_1 != 0xffffffff) {
    iVar3 = *(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    uVar11 = datum_new();
    uVar4 = (uint)uVar11;
    if (uVar4 != 0xffffffff) {
      iVar10 = (uVar4 & 0xffff) * 700 + *(int *)((int)((ulonglong)uVar11 >> 0x20) + 0x34);
      *(undefined1 *)(iVar10 + 4) = 0;
      *(bool *)(iVar10 + 5) = *(int *)(iVar3 + 0xc4) < 2;
      *(uint *)(iVar10 + 8) = param_1;
      *(undefined4 *)(iVar10 + 0xc) = 0xffffffff;
      *(undefined2 *)(iVar10 + 6) = 0;
      *(undefined4 *)(iVar10 + 0x18) = 0;
      *(undefined4 *)(iVar10 + 0x14) = 0;
      *(undefined4 *)(iVar10 + 0x10) = 0;
      sVar8 = 0;
      local_8 = 0.0;
      local_c = 0.0;
      local_4 = 0.0;
      if (0 < *(int *)(iVar3 + 0xc4)) {
        iVar5 = 0;
        sVar8 = 0;
        do {
          pfVar1 = (float *)(iVar5 * 0x20 + 0x1c + iVar10);
          iVar6 = iVar5 * 0x80 + *(int *)(iVar3 + 200);
          *pfVar1 = local_c;
          pfVar1[1] = local_8;
          pfVar1[2] = local_4;
          pfVar1[5] = 0.0;
          pfVar1[4] = 0.0;
          pfVar1[3] = 0.0;
          *(undefined2 *)(pfVar1 + 7) = 0;
          pfVar1[6] = 0.0;
          if (*(uint *)(iVar3 + 0x2c) != 0xffffffff) {
            iVar5 = *(int *)((*(uint *)(iVar3 + 0x2c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            sVar2 = *(short *)(iVar6 + 0x28);
            if (((-1 < sVar2) && ((int)sVar2 < *(int *)(iVar5 + 0x54))) &&
               (iVar9 = sVar2 * 0x40 + *(int *)(iVar5 + 0x58), *(int *)(iVar9 + 0x34) != 0)) {
              iVar9 = *(int *)(iVar9 + 0x38);
              iVar7 = bitmap_group_get_bitmap_data();
              if (iVar7 != 0) {
                pfVar1[6] = *(float *)(iVar6 + 0x24) /
                            (((*(float *)(iVar9 + 0xc) - *(float *)(iVar9 + 8)) *
                              (float)(int)*(short *)(iVar7 + 4) -
                             ((float)(int)*(short *)(iVar5 + 0x50) +
                             (float)(int)*(short *)(iVar5 + 0x50))) - 1.0);
              }
            }
          }
          local_c = local_c + *(float *)(iVar6 + 0x74);
          sVar8 = sVar8 + 1;
          iVar5 = (int)sVar8;
          local_8 = local_8 + *(float *)(iVar6 + 0x78);
          local_4 = local_4 + *(float *)(iVar6 + 0x7c);
        } while (iVar5 < *(int *)(iVar3 + 0xc4));
      }
      pfVar1 = (float *)(sVar8 * 0x20 + 0x1c + iVar10);
      *pfVar1 = local_c;
      pfVar1[1] = local_8;
      pfVar1[5] = 0.0;
      pfVar1[4] = 0.0;
      pfVar1[3] = 0.0;
      pfVar1[2] = local_4;
    }
  }
  return uVar4;
}
#endif
