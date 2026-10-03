// object_tree_collect_matching  (Ghidra: object_tree_collect_matching, already named)
// address 0x4fa0f0, size 168 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Recursively walks a binary tree of objects of a given
//   type/definition, collecting up to a limit into an array, optionally filtered by a
//   callback")
// rewrite confidence: 0.75
// evidence: types/objects.h object (first_child_object 0x118, next_object 0x114); global
//   0x008603b0 object_data.
// register convention: all six parameters are plain stack arguments (Ghidra's own fully
//   recovered signature, no in_REG markers anywhere).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

int32_t object_tree_collect_matching(uint32_t object_index, uint8_t (*filter)(uint32_t, void *),
    void *filter_context, int32_t count, int32_t max_count, datum_index *out)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (max_count <= count) {
        return count;
    }

    if ((filter == 0) || (filter(object_index, filter_context) != 0)) {
        out[count] = object_index;
        count++;
    }

    if (obj->first_child_object != k_datum_index_none) {
        count = object_tree_collect_matching(obj->first_child_object, filter, filter_context, count, max_count, out);
    }

    if (obj->next_object == k_datum_index_none) {
        return count;
    }

    return object_tree_collect_matching(obj->next_object, filter, filter_context, count, max_count, out);
}

#if 0
Original Ghidra decompilation (0x4fa0f0):

int object_tree_collect_matching
              (uint param_1,code *param_2,undefined4 param_3,int param_4,int param_5,int param_6)

{
  int iVar1;
  char cVar2;
  int iVar3;

  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (param_5 <= param_4) {
    return param_4;
  }
  if ((param_2 == (code *)0x0) || (cVar2 = (*param_2)(param_1,param_3), cVar2 != '\0')) {
    *(uint *)(param_6 + param_4 * 4) = param_1;
    param_4 = param_4 + 1;
  }
  iVar1 = *(int *)(iVar3 + 0x118);
  if (iVar1 != -1) {
    param_4 = object_tree_collect_matching(iVar1,param_2,param_3,param_4,param_5,param_6);
  }
  iVar3 = *(int *)(iVar3 + 0x114);
  if (iVar3 == -1) {
    return param_4;
  }
  iVar3 = object_tree_collect_matching(iVar3,param_2,param_3,param_4,param_5,param_6);
  return iVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
