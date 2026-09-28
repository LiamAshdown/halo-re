// flag_new
// address 0x4fb540, size 391 bytes, zero recorded callers (dead/unreferenced in this binary,
//   or its only caller lives outside the address ranges scanned so far)
// name confidence: 0.55 (functions.md: "Creates a new flag instance and builds its cloth
//   simulation grid (vertex UV grid and edge/shape constraints) from the flag tag definition")
// rewrite confidence: 0.55
// evidence: types/objects.h flag (definition_tag 0x0c, invalid 0x02, object_index 0x08,
//   previous_marker_position 0x10, vertices 0x1c stride 0x18, cell_split_codes 0x1534,
//   k_maximum_flag_cloth_vertices 0xe1), flag_vertex (position 0x00, previous_position 0x0c);
//   types/tags.h Flag (width 0x0c, height 0x0e, blue_flag_shader TagDependency 0x50);
//   out/phase4/objects_types_notes.md names the two callees flag_cloth_init_shape_constraints
//   (0x4fb770) directly and cites this function's own `(index & 0xffff) * 0x16bc` datum
//   arithmetic.
// Flag.blue_flag_shader is a TagDependency whose .tag_id field lands at absolute offset 0x50
// (confirmed against out/phase4/objects_types_notes.md: "blue shader TagID 0x50"), so the
// original `*(int *)(iVar2 + 0x50) != -1` is a whole-TagID compare, the usual "is this
// dependency set" idiom -- not a tag_fourcc read as an earlier draft of this file assumed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *flag_data;       // 0x008603a8
extern tag_instance *tag_instances; // 0x0087bc14
extern real_point3d *global_zero_vector3d_pointer;          // 0x006966f8, shared constant vector
extern real_point3d *global_origin3d_pointer; // 0x00696714, shared constant vector

extern datum_index datum_new(data_array *array); // UNSURE: returns {handle, data_array*} as a
    // 64-bit pair in the original; only the handle is modeled here, using flag_data directly
    // for the element base, matching light_new_attached.c's identical simplification.
    // memory module, 0x4d0480
extern void flag_cloth_mark_border_cells(flag *entry); // this module, 0x4fb6d0
extern void flag_cloth_init_shape_constraints(flag *entry); // this module, 0x4fb770

// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> flag_tag
datum_index flag_new(datum_index flag_tag)
{
    datum_index handle = (datum_index)0xffffffff;

    if (flag_tag != (datum_index)0xffffffff) {
        Flag *tag = (Flag *)tag_instances[flag_tag & 0xffff].data;

        handle = datum_new(flag_data);
        if (handle != (datum_index)0xffffffff) {
            flag *entry = &((flag *)flag_data->data)[handle & 0xffff];

            if (tag->height * tag->width < (int32_t)k_maximum_flag_cloth_vertices &&
                tag->width < 0x28 && *(int32_t *)&tag->blue_flag_shader.tag_id != -1) {
                int16_t row;

                entry->definition_tag = flag_tag;
                entry->invalid = 0;
                entry->unknown_03 = 0;
                entry->object_index = (datum_index)0xffffffff;
                entry->previous_marker_position.x = 0.0f;
                entry->previous_marker_position.y = 0.0f;
                entry->previous_marker_position.z = 0.0f;

                for (row = 0; row < tag->width; row++) {
                    int16_t col;

                    for (col = 0; col < tag->height; col++) {
                        flag_vertex *vertex = &entry->vertices[tag->height * row + col];

                        vertex->position = *global_zero_vector3d_pointer;
                        vertex->previous_position = *global_origin3d_pointer;

                        if (row < tag->width - 1 && col < tag->height - 1) {
                            entry->cell_split_codes[(tag->height - 1) * row + col] = 0;
                        }
                    }
                }

                flag_cloth_mark_border_cells(entry);
                flag_cloth_init_shape_constraints(entry);
                return handle;
            }
            entry->invalid = 1;
        }
    }
    return handle;
}

#if 0
Original Ghidra decompilation (0x4fb540):

uint flag_new(uint param_1)

{
  undefined4 *puVar1;
  int iVar2;
  undefined *puVar3;
  short sVar4;
  uint uVar5;
  int iVar6;
  short sVar7;
  int iVar8;
  short sVar9;
  int iVar10;
  undefined8 uVar11;

  uVar5 = 0xffffffff;
  if (param_1 != 0xffffffff) {
    iVar2 = *(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    uVar11 = datum_new();
    uVar5 = (uint)uVar11;
    if (uVar5 != 0xffffffff) {
      iVar10 = (uVar5 & 0xffff) * 0x16bc + *(int *)((int)((ulonglong)uVar11 >> 0x20) + 0x34);
      if ((((int)*(short *)(iVar2 + 0xe) * (int)*(short *)(iVar2 + 0xc) < 0xe1) &&
          (*(short *)(iVar2 + 0xc) < 0x28)) && (*(int *)(iVar2 + 0x50) != -1)) {
        *(uint *)(iVar10 + 0xc) = param_1;
        *(undefined1 *)(iVar10 + 2) = 0;
        *(undefined1 *)(iVar10 + 3) = 0;
        *(undefined4 *)(iVar10 + 8) = 0xffffffff;
        *(undefined4 *)(iVar10 + 0x18) = 0;
        *(undefined4 *)(iVar10 + 0x14) = 0;
        *(undefined4 *)(iVar10 + 0x10) = 0;
        sVar9 = 0;
        if (0 < *(short *)(iVar2 + 0xc)) {
          do {
            sVar4 = *(short *)(iVar2 + 0xe);
            sVar7 = 0;
            if (0 < sVar4) {
              do {
                puVar3 = PTR_DAT_006966f8;
                iVar8 = (int)sVar7;
                puVar1 = (undefined4 *)(iVar10 + 0x1c + ((int)sVar4 * (int)sVar9 + iVar8) * 0x18);
                *puVar1 = *(undefined4 *)PTR_DAT_006966f8;
                puVar1[1] = *(undefined4 *)(puVar3 + 4);
                puVar1[2] = *(undefined4 *)(puVar3 + 8);
                puVar3 = PTR_DAT_00696714;
                puVar1[3] = *(undefined4 *)PTR_DAT_00696714;
                puVar1[4] = *(undefined4 *)(puVar3 + 4);
                puVar1[5] = *(undefined4 *)(puVar3 + 8);
                if (((int)sVar9 < *(short *)(iVar2 + 0xc) + -1) &&
                   (iVar6 = *(short *)(iVar2 + 0xe) + -1, iVar8 < iVar6)) {
                  *(undefined2 *)(iVar10 + 0x1534 + (iVar6 * sVar9 + iVar8) * 2) = 0;
                }
                sVar4 = *(short *)(iVar2 + 0xe);
                sVar7 = sVar7 + 1;
              } while (sVar7 < sVar4);
            }
            sVar9 = sVar9 + 1;
          } while (sVar9 < *(short *)(iVar2 + 0xc));
        }
        FUN_004fb6d0(iVar10);
        FUN_004fb770(iVar10);
        return uVar5;
      }
      *(undefined1 *)(iVar10 + 2) = 1;
    }
  }
  return uVar5;
}
#endif
