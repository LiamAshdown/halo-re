// object_type_definition_chain_build
// address 0x4f3db0, size 120 bytes
// name confidence: 0.8 (types/objects.h documents this address by exactly this name: "the chain
//   link at 0xc0" table comment, and functions.md's summary -- "Builds a combined linked list of
//   all object-type sub-definitions and calls each one's initialization vtable hook (offset
//   +0x14)" -- matches the code exactly)
// rewrite confidence: 0.85
// evidence: types/objects.h object_type_definition (subdefinitions[16] at 0x80, next at 0xc0,
//   initialize at 0x14); global 0x0069bfdc object_type_definitions[12]; global 0x008603dc
//   object_type_definition_list (the chain head).
// register convention: none (void). Walks the 12-entry object_type_definitions table and, for
//   every subdefinition not already linked into some other type's chain, appends it once; then
//   runs every chained definition's +0x14 initialize hook in link order.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern object_type_definition *object_type_definition_list; // 0x008603dc, chain head

void object_type_definition_chain_build(void)
{
    object_type_definition **tail = &object_type_definition_list;
    object_type_definition *def;
    object_type_definition *sub;
    int16_t type_index;
    int16_t sub_index;

    for (type_index = 0; type_index < k_maximum_object_types; type_index++) {
        def = object_type_definitions[type_index];
        *tail = def;
        tail = &def->next;
        for (sub_index = 0; sub_index < k_maximum_object_subdefinitions; sub_index++) {
            sub = def->subdefinitions[sub_index];
            if (sub == 0) {
                break;
            }
            if (sub->next == 0) {
                *tail = sub;
                tail = &sub->next;
            }
        }
    }
    *tail = 0;

    for (def = object_type_definition_list; def != 0; def = def->next) {
        if (def->initialize != 0) {
            ((void (*)(void))def->initialize)();
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f3db0):

void FUN_004f3db0(void)

{
  undefined *puVar1;
  int iVar2;
  short sVar3;
  undefined **ppuVar4;
  int iVar5;
  int *piVar6;

  piVar6 = &DAT_008603dc;
  ppuVar4 = &PTR_PTR_0069bfdc;
  iVar5 = 0xc;
  do {
    puVar1 = *ppuVar4;
    *piVar6 = (int)puVar1;
    piVar6 = (int *)(puVar1 + 0xc0);
    sVar3 = 0;
    do {
      iVar2 = *(int *)(puVar1 + sVar3 * 4 + 0x80);
      if (iVar2 == 0) break;
      if (*(int *)(iVar2 + 0xc0) == 0) {
        *piVar6 = iVar2;
        piVar6 = (int *)(iVar2 + 0xc0);
      }
      sVar3 = sVar3 + 1;
    } while (sVar3 < 0x10);
    ppuVar4 = ppuVar4 + 1;
    iVar5 = iVar5 + -1;
    if (iVar5 == 0) {
      *piVar6 = 0;
      for (iVar5 = DAT_008603dc; iVar5 != 0; iVar5 = *(int *)(iVar5 + 0xc0)) {
        if (*(code **)(iVar5 + 0x14) != (code *)0x0) {
          (**(code **)(iVar5 + 0x14))();
        }
      }
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
