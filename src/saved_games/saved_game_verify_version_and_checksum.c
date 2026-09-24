// saved_game_verify_version_and_checksum  (Ghidra: saved_game_verify_version_and_checksum, already named)
// address 0x538430, size 405 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md's
// game_state_header field table (build_version, scenario_name, allocation_checksum,
// local_player_count, map_checksum are all read and compared here, matching the exact literal
// strings/offsets game_state_build_header (0x538000) wrote); shell_fatal_error_argument /
// shell_display_fatal_error_dialog signatures from src/cache/cache_file_open_by_name.c.
// register convention: __cdecl (Ghidra-recognized), header and report_error are the recognized
// parameters.
// The two hand-rolled byte-compare loops (build_version against nine literal build strings,
// scenario_name against the running scenario's tag path) are rewritten as strcmp, following the
// precedent in src/interface/console_update_display.c ("inlined byte-compare loop in the
// original"); semantics (equality only, no other use of the ordering result) are unchanged.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "cache.h"

extern uint32_t game_state_crc; // 0x006e2dd4
extern int16_t local_player_count; // 0x006894b8
extern uint32_t cache_file_current_header_crc32; // 0x006a81b8
extern datum_index global_scenario_index; // 0x0069e8d4
extern tag_instance *tag_instances; // 0x0087bc14
extern char *shell_fatal_error_argument; // 0x00722bbc

extern int32_t strcmp(const char *a, const char *b);
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

// Verifies a loaded game_state_header against the running build: its build_version string must
// match the current build or one of eight older accepted builds, and its scenario_name,
// allocation_checksum, local_player_count and map_checksum must all match the running state.
// On any mismatch, if report_error is set, points shell_fatal_error_argument at the header's
// scenario_name and raises a fatal error dialog (string 0x89, title 0x7e).
uint8_t saved_game_verify_version_and_checksum(game_state_header *header, uint8_t report_error)
{
    static const char *k_accepted_build_versions[] = {
        "01.00.03.0606", "01.00.04.0607", "01.00.05.0610", "01.00.06.0612",
        "01.00.07.0613", "01.00.08.0616", "01.00.09.0619", "01.00.10.0620"
    };
    uint8_t version_ok;
    int32_t i;
    char *tag_path;

    version_ok = (strcmp(header->build_version, "01.00.10.0621") == 0) ? 1 : 0;
    if (version_ok == 0) {
        for (i = 0; i < 8; i = i + 1) {
            if (strcmp(header->build_version, k_accepted_build_versions[i]) == 0) {
                version_ok = 1;
                break;
            }
        }
    }

    if (version_ok == 0) {
        if (report_error == 0) {
            return 0;
        }
        shell_fatal_error_argument = header->scenario_name;
        shell_display_fatal_error_dialog(0x89, 0x7e, 1);
        return 0;
    }

    tag_path = tag_instances[(int16_t)global_scenario_index].path;
    if (strcmp(header->scenario_name, tag_path) == 0 &&
        header->allocation_checksum == game_state_crc &&
        header->local_player_count == local_player_count &&
        header->map_checksum == cache_file_current_header_crc32) {
        return 1;
    }

    if (report_error != 0) {
        shell_fatal_error_argument = header->scenario_name;
        shell_display_fatal_error_dialog(0x89, 0x7e, 1);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x538430):

uint __cdecl saved_game_verify_version_and_checksum(int *header,char report_error)

{
  byte bVar1;
  int *piVar2;
  uint uVar3;
  byte *pbVar4;
  int iVar5;
  undefined1 uVar6;
  int *piVar7;
  int *piVar8;
  byte *pbVar9;
  char *pcVar10;
  bool bVar11;

  piVar8 = header + 0x41;
  iVar5 = 0xe;
  bVar11 = true;
  piVar2 = piVar8;
  pcVar10 = "01.00.10.0621";
  do {
    if (iVar5 == 0) break;
    iVar5 = iVar5 + -1;
    bVar11 = (char)*piVar2 == *pcVar10;
    piVar2 = (int *)((int)piVar2 + 1);
    pcVar10 = pcVar10 + 1;
  } while (bVar11);
  if (bVar11) goto LAB_00538521;
  iVar5 = 0xe;
  bVar11 = true;
  piVar2 = piVar8;
  pcVar10 = "01.00.03.0606";
  do {
    if (iVar5 == 0) break;
    iVar5 = iVar5 + -1;
    bVar11 = (char)*piVar2 == *pcVar10;
    piVar2 = (int *)((int)piVar2 + 1);
    pcVar10 = pcVar10 + 1;
  } while (bVar11);
  piVar2 = piVar8;
  if (bVar11) {
LAB_005384eb:
    uVar3 = CONCAT31((int3)((uint)piVar2 >> 8),1);
  }
  else {
    iVar5 = 0xe;
    bVar11 = true;
    piVar7 = piVar8;
    pcVar10 = "01.00.04.0607";
    do {
      if (iVar5 == 0) break;
      iVar5 = iVar5 + -1;
      bVar11 = (char)*piVar7 == *pcVar10;
      piVar7 = (int *)((int)piVar7 + 1);
      pcVar10 = pcVar10 + 1;
    } while (bVar11);
    if (bVar11) goto LAB_005384eb;
    iVar5 = 0xe;
    bVar11 = true;
    piVar7 = piVar8;
    pcVar10 = "01.00.05.0610";
    do {
      if (iVar5 == 0) break;
      iVar5 = iVar5 + -1;
      bVar11 = (char)*piVar7 == *pcVar10;
      piVar7 = (int *)((int)piVar7 + 1);
      pcVar10 = pcVar10 + 1;
    } while (bVar11);
    if (bVar11) goto LAB_005384eb;
    iVar5 = 0xe;
    bVar11 = true;
    piVar7 = piVar8;
    pcVar10 = "01.00.06.0612";
    do {
      if (iVar5 == 0) break;
      iVar5 = iVar5 + -1;
      bVar11 = (char)*piVar7 == *pcVar10;
      piVar7 = (int *)((int)piVar7 + 1);
      pcVar10 = pcVar10 + 1;
    } while (bVar11);
    if (bVar11) goto LAB_005384eb;
    iVar5 = 0xe;
    bVar11 = true;
    piVar7 = piVar8;
    pcVar10 = "01.00.07.0613";
    do {
      if (iVar5 == 0) break;
      iVar5 = iVar5 + -1;
      bVar11 = (char)*piVar7 == *pcVar10;
      piVar7 = (int *)((int)piVar7 + 1);
      pcVar10 = pcVar10 + 1;
    } while (bVar11);
    if (bVar11) goto LAB_005384eb;
    iVar5 = 0xe;
    bVar11 = true;
    piVar7 = piVar8;
    pcVar10 = "01.00.08.0616";
    do {
      if (iVar5 == 0) break;
      iVar5 = iVar5 + -1;
      bVar11 = (char)*piVar7 == *pcVar10;
      piVar7 = (int *)((int)piVar7 + 1);
      pcVar10 = pcVar10 + 1;
    } while (bVar11);
    if (bVar11) goto LAB_005384eb;
    iVar5 = 0xe;
    bVar11 = true;
    piVar7 = piVar8;
    pcVar10 = "01.00.09.0619";
    do {
      if (iVar5 == 0) break;
      iVar5 = iVar5 + -1;
      bVar11 = (char)*piVar7 == *pcVar10;
      piVar7 = (int *)((int)piVar7 + 1);
      pcVar10 = pcVar10 + 1;
    } while (bVar11);
    if (bVar11) goto LAB_005384eb;
    iVar5 = 0xe;
    piVar2 = (int *)0x0;
    bVar11 = true;
    pcVar10 = "01.00.10.0620";
    do {
      if (iVar5 == 0) break;
      iVar5 = iVar5 + -1;
      bVar11 = (char)*piVar8 == *pcVar10;
      piVar8 = (int *)((int)piVar8 + 1);
      pcVar10 = pcVar10 + 1;
    } while (bVar11);
    if (bVar11) goto LAB_005384eb;
    uVar3 = 0;
  }
  if (report_error == '\0') {
    if ((char)uVar3 == '\0') {
      return uVar3;
    }
  }
  else if ((char)uVar3 == '\0') {
    DAT_00722bbc = (byte *)(header + 1);
    uVar3 = shell_display_fatal_error_dialog(0x89,0x7e,1);
    return uVar3 & 0xffffff00;
  }
LAB_00538521:
  uVar6 = 0;
  pbVar9 = *(byte **)((short)DAT_0069e8d4 * 0x20 + 0x10 + DAT_0087bc14);
  pbVar4 = (byte *)(header + 1);
  do {
    bVar1 = *pbVar4;
    bVar11 = bVar1 < *pbVar9;
    if (bVar1 != *pbVar9) {
LAB_00538564:
      iVar5 = (1 - (uint)bVar11) - (uint)(bVar11 != 0);
      goto LAB_00538569;
    }
    if (bVar1 == 0) break;
    bVar1 = pbVar4[1];
    bVar11 = bVar1 < pbVar9[1];
    if (bVar1 != pbVar9[1]) goto LAB_00538564;
    pbVar4 = pbVar4 + 2;
    pbVar9 = pbVar9 + 2;
  } while (bVar1 != 0);
  iVar5 = 0;
LAB_00538569:
  if ((((iVar5 == 0) && (iVar5 = *header, iVar5 == DAT_006e2dd4)) &&
      ((short)header[0x49] == DAT_006894b8)) && (header[0x4a] == DAT_006a81b8)) {
    uVar6 = 1;
  }
  else {
    iVar5 = CONCAT31((int3)((uint)iVar5 >> 8),report_error);
    if (report_error != '\0') {
      DAT_00722bbc = (byte *)(header + 1);
      uVar3 = shell_display_fatal_error_dialog(0x89,0x7e,1);
      return uVar3 & 0xffffff00;
    }
  }
  return CONCAT31((int3)((uint)iVar5 >> 8),uVar6);
}
#endif
