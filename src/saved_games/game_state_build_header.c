// game_state_build_header  (Ghidra: game_state_build_header, already named)
// address 0x538000, size 200 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md's
// game_state_header field table: allocation_checksum, scenario_name (tag_instances[
// global_scenario_index]->path), build_version ("01.00.10.0621"), local_player_count
// (0x006894b8), difficulty (game globals +0x0e), map_checksum (cache_file_current_header.crc32
// at 0x006a81b8). The literal dwords 0x302e3130/0x30312e30/0x3236302e and word 0x31 spell
// "01.00.10.0621" little-endian across build_version.
// register convention: no parameters, no return value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "cache.h"

extern game_state_header *game_state_header_ptr; // 0x006e2de0
extern uint8_t game_state_header_valid; // 0x006e2dd8
extern uint8_t game_state_revert_available; // 0x006e2dd9
extern int32_t game_state_revert_time; // 0x006e2ddc
extern datum_index global_scenario_index; // 0x0069e8d4
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t local_player_count; // 0x006894b8
extern uint8_t *cache_file_slot_table; // 0x006b0b80, opaque here; +0x0e is difficulty (see src/ai, src/game)
extern uint32_t cache_file_current_header_crc32; // 0x006a81b8
extern uint32_t game_state_crc; // 0x006e2dd4

void game_state_build_header(void)
{
    game_state_header *header;
    uint32_t *zero;
    int32_t count;
    char *src;
    char *dst;

    game_state_header_valid = 1;
    game_state_revert_available = 0;
    game_state_revert_time = -1;
    header = game_state_header_ptr;
    zero = (uint32_t *)header;
    for (count = 0x53; count != 0; count = count - 1) {
        *zero = 0;
        zero = zero + 1;
    }

    src = tag_instances[(int16_t)global_scenario_index].path;
    dst = header->scenario_name;
    do {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
    } while (*(dst - 1) != '\0');

    // "01.00.10.0621"
    header->build_version[0] = '0'; header->build_version[1] = '1'; header->build_version[2] = '.';
    header->build_version[3] = '0'; header->build_version[4] = '0'; header->build_version[5] = '.';
    header->build_version[6] = '1'; header->build_version[7] = '0'; header->build_version[8] = '.';
    header->build_version[9] = '0'; header->build_version[10] = '6'; header->build_version[11] = '2';
    header->build_version[12] = '1'; header->build_version[13] = '\0';

    header->local_player_count = local_player_count;
    header->difficulty = *(int16_t *)(cache_file_slot_table + 0xe);
    header->map_checksum = cache_file_current_header_crc32;
    header->allocation_checksum = game_state_crc;
}

#if 0
Original Ghidra decompilation (0x538000):

void game_state_build_header(void)

{
  char cVar1;
  int iVar2;
  char *pcVar3;
  char *pcVar4;
  undefined4 *puVar5;

  DAT_006e2dd8 = 1;
  DAT_006e2dd9 = 0;
  DAT_006e2ddc = 0xffffffff;
  puVar5 = DAT_006e2de0;
  for (iVar2 = 0x53; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  pcVar3 = *(char **)((short)DAT_0069e8d4 * 0x20 + 0x10 + DAT_0087bc14);
  pcVar4 = (char *)(DAT_006e2de0 + 1);
  do {
    cVar1 = *pcVar3;
    pcVar3 = pcVar3 + 1;
    *pcVar4 = cVar1;
    puVar5 = DAT_006e2de0;
    pcVar4 = pcVar4 + 1;
  } while (cVar1 != '\0');
  DAT_006e2de0[0x41] = 0x302e3130;
  puVar5[0x42] = 0x30312e30;
  puVar5[0x43] = 0x3236302e;
  *(undefined2 *)(puVar5 + 0x44) = 0x31;
  iVar2 = DAT_006b0b80;
  *(undefined2 *)(DAT_006e2de0 + 0x49) = DAT_006894b8;
  *(undefined2 *)((int)DAT_006e2de0 + 0x126) = *(undefined2 *)(iVar2 + 0xe);
  DAT_006e2de0[0x4a] = DAT_006a81b8;
  *DAT_006e2de0 = DAT_006e2dd4;
  return;
}
#endif
