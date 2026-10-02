// network_machine_check_build_version  (Ghidra: FUN_004dff20, unnamed)
// address 0x4dff20, size 72 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Compares the version/build string at
// DAT_00719879 against the caller-supplied string and flags the object at unaff_EDI as
// mismatched (bit 3 of +0xe) if they differ." 0x00719879 is types/networking.h's
// network_build_string; +0xe matches network_machine::flags,
// k_network_machine_version_mismatch = 0x08.
// register convention: EAX = remote_version (const char *), EDI = machine (network_machine *).
// blam-cc: EAX -> remote_version, EDI -> machine
// UNSURE: the hand-rolled byte loop computes a full three-way comparison result that this
// function only ever tests for zero; rewritten as a plain equality scan, which is
// byte-for-byte equivalent for that purpose.
// UNSURE (important): traced literally, the flag is set when the two strings are EQUAL
// (`iVar3 == 0`, reached only by matching all the way to the terminating NUL), not when they
// differ -- the opposite polarity of both the functions.md summary and of what the existing
// k_network_machine_version_mismatch name suggests. Reusing that enumerator here anyway
// (types/*.h is not edited by this batch) but the polarity below is the literal one from the
// decompilation, not the summary's.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char network_build_string[]; // 0x00719879

// Sets k_network_machine_version_mismatch on `machine` when `remote_version` matches this
// build's version string exactly (see the polarity UNSURE note above).
void network_machine_check_build_version(const char *remote_version, network_machine *machine)
{
    const uint8_t *local;
    const uint8_t *remote;
    int32_t equal;

    local = (const uint8_t *)network_build_string;
    remote = (const uint8_t *)remote_version;
    equal = 0;
    while (*local == *remote) {
        if (*local == 0) {
            equal = 1;
            break;
        }
        local = local + 1;
        remote = remote + 1;
    }
    if (equal) {
        machine->flags |= k_network_machine_version_mismatch;
    }
}

#if 0
Original Ghidra decompilation (0x4dff20):

void FUN_004dff20(void)

{
  byte bVar1;
  byte *in_EAX;
  byte *pbVar2;
  int iVar3;
  int unaff_EDI;
  bool bVar4;

  pbVar2 = &DAT_00719879;
  do {
    bVar1 = *pbVar2;
    bVar4 = bVar1 < *in_EAX;
    if (bVar1 != *in_EAX) {
LAB_004dff58:
      iVar3 = (1 - (uint)bVar4) - (uint)(bVar4 != 0);
      goto LAB_004dff5d;
    }
    if (bVar1 == 0) break;
    bVar1 = pbVar2[1];
    bVar4 = bVar1 < in_EAX[1];
    if (bVar1 != in_EAX[1]) goto LAB_004dff58;
    pbVar2 = pbVar2 + 2;
    in_EAX = in_EAX + 2;
  } while (bVar1 != 0);
  iVar3 = 0;
LAB_004dff5d:
  if (iVar3 == 0) {
    *(byte *)(unaff_EDI + 0xe) = *(byte *)(unaff_EDI + 0xe) | 8;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
