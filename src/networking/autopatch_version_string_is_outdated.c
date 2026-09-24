// autopatch_version_string_is_outdated  (Ghidra: autopatch_version_string_is_outdated, already named)
// address 0x5781c0, size 238 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary: "Compares a version string against the
// current build and a minimum baseline build to decide whether the client is out of date."; the
// two hardcoded strings decode to "01.00.08.0616" (a minimum baseline) and "01.00.10.0621" (the
// current build, matching autopatch_current_version_string_get).
// register convention: the version string to check in EDX (in_EDX, unresolved register read).
// blam-cc: EDX -> version

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t strcmp(const char *a, const char *b);

// blam-cc: EDX -> version
// UNSURE: despite computing a signed strcmp-shaped result, only equality is ever tested here.
// Returns 1 if version exactly matches either the current build string ("01.00.10.0621") or the
// minimum baseline ("01.00.08.0616"), 0 for any other string -- so, despite the name, this reads
// as a "recognized version" check rather than a true outdated/not-outdated comparison.
uint32_t autopatch_version_string_is_outdated(char *version)
{
    static const char minimum_baseline[] = "01.00.08.0616";
    static const char current_build[] = "01.00.10.0621";

    if (strcmp(version, current_build) != 0) {
        if (strcmp(version, minimum_baseline) != 0) {
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x5781c0):

undefined4 autopatch_version_string_is_outdated(void)

{
  byte bVar1;
  byte *pbVar2;
  int iVar3;
  byte *in_EDX;
  byte *pbVar4;
  undefined4 *puVar5;
  bool bVar6;
  byte local_118 [16];
  byte local_108;
  undefined1 auStack_107 [7];
  undefined4 local_100;
  undefined2 local_fc;

  local_118[0] = 0x30;
  local_118[1] = 0x31;
  local_118[2] = 0x2e;
  local_118[3] = 0x30;
  local_118[4] = 0x30;
  local_118[5] = 0x2e;
  local_118[6] = 0x30;
  local_118[7] = 0x38;
  local_118[8] = 0x2e;
  local_118[9] = 0x30;
  local_118[10] = 0x36;
  local_118[0xb] = 0x31;
  local_118[0xc] = 0x36;
  local_118[0xd] = 0;
  puVar5 = (undefined4 *)auStack_107;
  for (iVar3 = 0x3f; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  *(undefined2 *)puVar5 = 0;
  *(undefined1 *)((int)puVar5 + 2) = 0;
  _local_108 = 0x302e3130;
  auStack_107._3_4_ = 0x30312e30;
  local_100 = 0x3236302e;
  local_fc = 0x31;
  pbVar4 = &local_108;
  pbVar2 = in_EDX;
  do {
    bVar1 = *pbVar2;
    bVar6 = bVar1 < *pbVar4;
    if (bVar1 != *pbVar4) {
LAB_00578259:
      iVar3 = (1 - (uint)bVar6) - (uint)(bVar6 != 0);
      goto LAB_0057825e;
    }
    if (bVar1 == 0) break;
    bVar1 = pbVar2[1];
    bVar6 = bVar1 < pbVar4[1];
    if (bVar1 != pbVar4[1]) goto LAB_00578259;
    pbVar2 = pbVar2 + 2;
    pbVar4 = pbVar4 + 2;
  } while (bVar1 != 0);
  iVar3 = 0;
LAB_0057825e:
  if (iVar3 != 0) {
    pbVar4 = local_118;
    do {
      bVar1 = *in_EDX;
      bVar6 = bVar1 < *pbVar4;
      if (bVar1 != *pbVar4) {
LAB_00578290:
        iVar3 = (1 - (uint)bVar6) - (uint)(bVar6 != 0);
        goto LAB_00578295;
      }
      if (bVar1 == 0) break;
      bVar1 = in_EDX[1];
      bVar6 = bVar1 < pbVar4[1];
      if (bVar1 != pbVar4[1]) goto LAB_00578290;
      in_EDX = in_EDX + 2;
      pbVar4 = pbVar4 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_00578295:
    if (iVar3 != 0) {
      return 0;
    }
  }
  return 1;
}
#endif
