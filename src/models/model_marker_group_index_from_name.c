// model_marker_group_index_from_name  (Ghidra: model_marker_group_index_from_name, already
// named)
// address 0x4d77c0, size 144 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/models_types_notes.md ModelMarker/ModelMarkerInstance section:
//   "0x4d77c0 binary-searches the names with _stricmp, so the block must be sorted." Standard
//   binary search over GBXModel.markers (stride 0x40, ModelMarker), name field first.
// register convention: model tag id in EAX (in_EAX); name as the recognized stack parameter
//   (param_1).
//   // blam-cc: EAX -> model_tag_id, stack -> name

#include "crt.h"
#include "tags.h"
#include "math.h"
#include "cache.h"
#include "models.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14

// Binary-searches a model's sorted marker group table (GBXModel.markers, ModelMarker.name
// first) for a case-insensitive match, returning its index or -1. Also returns -1 for an
// invalid tag id, a NULL name or an empty name.
int16_t model_marker_group_index_from_name(datum_index model_tag_id, const char *name)
{
    GBXModel *model;
    ModelMarker *markers;
    int16_t lo, hi;

    if (model_tag_id == (datum_index)-1 || name == 0 || *name == '\0') {
        return -1;
    }

    model = (GBXModel *)tag_instances[model_tag_id & 0xffff].data;
    markers = (ModelMarker *)model->markers.pointer;

    lo = 0;
    hi = (int16_t)(model->markers.count - 1);
    while (lo <= hi) {
        int16_t mid = (int16_t)((hi + lo) / 2);
        int32_t cmp = _stricmp(name, markers[mid].name.string);

        if (cmp == 0) {
            return mid;
        }
        if (cmp < 0) {
            hi = (int16_t)(mid - 1);
        } else {
            lo = (int16_t)(mid + 1);
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4d77c0):

uint model_marker_group_index_from_name(char *param_1)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  uint uVar4;
  ushort uVar5;
  uint uVar6;

  if (((in_EAX != 0xffffffff) && (param_1 != (char *)0x0)) && (*param_1 != '\0')) {
    iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar3 = 0;
    uVar5 = *(short *)(iVar1 + 0xac) - 1;
    uVar6 = (uint)uVar5;
    if (-1 < (short)uVar5) {
      do {
        uVar4 = ((int)(short)uVar6 + (int)(short)iVar3) / 2;
        iVar2 = __stricmp(param_1,(char *)((short)uVar4 * 0x40 + *(int *)(iVar1 + 0xb0)));
        if (iVar2 == 0) {
          return uVar4 & 0xffff;
        }
        if (iVar2 < 0) {
          uVar6 = uVar4 - 1;
        }
        else {
          iVar3 = uVar4 + 1;
        }
      } while ((short)iVar3 <= (short)uVar6);
      return 0xffff;
    }
  }
  return 0xffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
