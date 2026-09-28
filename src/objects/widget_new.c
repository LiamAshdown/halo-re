// widget_new
// address 0x4ffa80, size 334 bytes
// name confidence: 0.85 (out/phase4/objects_functions.md: "Creates a widget attachment instance
//   of the appropriate concrete type for each matching attachment on an object, and links it
//   into the object's widget list"; types/objects.h's own struct comments cite this address
//   directly for both `object.first_widget` and `widget_type_definition`)
// rewrite confidence: 0.55
// evidence: types/objects.h object (first_widget 0x16c), widget (identifier 0x00, type 0x02,
//   instance 0x04, next_widget 0x08), widget_type_definition (group_tag 0x00, new_instance
//   0x18); types/tags.h Object (widgets TagReflexive 0x14c/0x150), ObjectWidget (group tag at
//   0x00, reference TagID at 0x0c).
// register convention: Ghidra shows a single unresolved `in_EAX`; by the object_data lookup
//   shape shared with every function in this module that resolves an object from a bare index,
//   EAX is the object index.
// blam-cc: EAX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;      // 0x008603b0
extern data_array *widget_data;      // 0x00860398
extern tag_instance *tag_instances;  // 0x0087bc14
extern widget_type_definition widget_type_definitions[k_maximum_widget_types]; // 0x0069c010

extern datum_index datum_new(data_array *array);    // memory module, 0x4d0480
extern void datum_delete(data_array *array, datum_index handle); // memory module, 0x4d0510

void widget_new(uint32_t object_index /*EAX*/) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    int32_t i;

    obj->first_widget = (datum_index)0xffffffff;

    for (i = 0; i < (int32_t)tag->widgets.count; i++) {
        ObjectWidget *attachment = &((ObjectWidget *)tag->widgets.pointer)[i];
        int32_t type;

        for (type = 0; type < k_maximum_widget_types; type++) {
            if (widget_type_definitions[type].group_tag == *(uint32_t *)attachment) {
                if (*(int32_t *)&((struct ObjectWidget *)attachment)->reference.tag_id != -1) {
                    datum_index handle = datum_new(widget_data);
                    if (handle != (datum_index)0xffffffff) {
                        widget *entry = &((widget *)widget_data->data)[handle & 0xffff];
                        entry->type = (int16_t)type;

                        if (widget_type_definitions[type].new_instance == 0) {
                            entry->instance = (datum_index)0xffffffff;
                            entry->next_widget = obj->first_widget;
                            obj->first_widget = handle;
                        } else {
                            datum_index (*new_instance)(TagID) =
                                (datum_index (*)(TagID))widget_type_definitions[type].new_instance;
                            datum_index instance = new_instance(((struct ObjectWidget *)attachment)->reference.tag_id);

                            entry->instance = instance;
                            if (instance == (datum_index)0xffffffff) {
                                datum_delete(widget_data, handle);
                            } else {
                                entry->next_widget = obj->first_widget;
                                obj->first_widget = handle;
                            }
                        }
                    }
                }
                break;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ffa80):

void widget_new(void)

{
  uint *puVar1;
  int iVar2;
  short sVar3;
  uint in_EAX;
  int iVar4;
  int *piVar5;
  uint uVar6;
  int iVar7;
  short sVar8;
  undefined8 uVar9;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  puVar1[0x5b] = 0xffffffff;
  iVar4 = 0;
  sVar3 = 0;
  if (0 < *(int *)(iVar2 + 0x14c)) {
    do {
      piVar5 = (int *)(iVar4 * 0x20 + *(int *)(iVar2 + 0x150));
      sVar8 = 0;
      do {
        if ((&DAT_0069c010)[sVar8 * 10] == *piVar5) {
          if ((sVar8 != -1) && (piVar5[3] != -1)) {
            uVar9 = datum_new();
            uVar6 = (uint)uVar9;
            if (uVar6 != 0xffffffff) {
              iVar4 = *(int *)((int)((ulonglong)uVar9 >> 0x20) + 0x34) + (uVar6 & 0xffff) * 0xc;
              *(short *)(iVar4 + 2) = sVar8;
              if ((code *)(&PTR_flag_new_0069c028)[sVar8 * 10] == (code *)0x0) {
                *(uint *)(iVar4 + 8) = puVar1[0x5b];
                puVar1[0x5b] = uVar6;
                *(undefined4 *)(iVar4 + 4) = 0xffffffff;
              }
              else {
                iVar7 = (*(code *)(&PTR_flag_new_0069c028)[sVar8 * 10])(piVar5[3]);
                *(int *)(iVar4 + 4) = iVar7;
                if (iVar7 == -1) {
                  datum_delete();
                }
                else {
                  *(uint *)(iVar4 + 8) = puVar1[0x5b];
                  puVar1[0x5b] = uVar6;
                }
              }
            }
          }
          break;
        }
        sVar8 = sVar8 + 1;
      } while (sVar8 < 5);
      sVar3 = sVar3 + 1;
      iVar4 = (int)sVar3;
    } while (iVar4 < *(int *)(iVar2 + 0x14c));
  }
  return;
}
#endif
