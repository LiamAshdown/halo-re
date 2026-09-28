// tag_lookup
// address 0x442550, size 114 bytes
// name confidence: 0.8 (already named by Ghidra; out/phase4/cache_functions.md: "Looks up a tag
// by group (register EDI) and name string, returning its tag id or -1 if not found or no map is
// loaded")
// rewrite confidence: 0.85
// evidence: types/cache.h tag_instance / cache_file_tag_header layouts and globals list.
// register convention: tag_group group in EDI (unaff_EDI); char *path is an ordinary parameter.

#include "crt.h"
#include "tags.h"
#include "cache.h"


extern uint8_t cache_file_loaded;              // 0x006a8150
extern cache_file_tag_header *tag_header;      // 0x006a8954
extern tag_instance *tag_instances;            // 0x0087bc14

// blam-cc: group in EDI
// Linear-scans the resident tag table for an entry whose primary group matches `group` and whose
// path matches `path` case-insensitively, returning its tag id, or k_datum_index_none if no map
// is loaded or nothing matches.
datum_index tag_lookup(tag_group group, char *path)
{
    int16_t index;

    if (!cache_file_loaded) {
        return (datum_index)0xffffffff;
    }

    for (index = 0; index < tag_header->tag_count; index++) {
        if (tag_instances[index].group_tag == group) {
            if (_stricmp(path, tag_instances[index].path) == 0) {
                return tag_instances[index].tag_id;
            }
        }
    }
    return (datum_index)0xffffffff;
}

#if 0
Original Ghidra decompilation (0x442550):

undefined4 tag_lookup(char *param_1)

{
  int iVar1;
  int *piVar2;
  short sVar3;
  int unaff_EDI;

  if (DAT_006a8150 == '\0') {
    return 0xffffffff;
  }
  sVar3 = 0;
  if (0 < *(int *)(DAT_006a8954 + 0xc)) {
    iVar1 = 0;
    do {
      piVar2 = (int *)(iVar1 * 0x20 + DAT_0087bc14);
      if (unaff_EDI == *piVar2) {
        iVar1 = __stricmp(param_1,(char *)piVar2[4]);
        if (iVar1 == 0) {
          return *(undefined4 *)(sVar3 * 0x20 + 0xc + DAT_0087bc14);
        }
      }
      sVar3 = sVar3 + 1;
      iVar1 = (int)sVar3;
    } while (iVar1 < *(int *)(DAT_006a8954 + 0xc));
  }
  return 0xffffffff;
}
#endif
