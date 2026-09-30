// game_state_new  (Ghidra: game_state_new, already named)
// address 0x5380d0, size 122 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md; identical data_array header layout to
// src/memory/data_new.c (name, maximum_count, size, valid, 'd@t@' signature, data pointer),
// but carved out of the game-state arena at game_state_base + game_state_cursor instead of
// GlobalAlloc, and its block size is folded into game_state_crc via crc32_update.
// register convention: element size in BX (unaff_BX, per
// out/phase4/saved_games_types_notes.md "register conventions"); name and maximum_count are
// the recognized stack parameters (param_1, param_2).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"

extern uint8_t *game_state_base; // 0x006e2dc8
extern int32_t game_state_cursor; // 0x006e2dcc
extern uint32_t game_state_crc; // 0x006e2dd4

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0

// blam-cc: element size in EBX, then the recognized stack parameters (name, maximum_count)
// Carves a new data_array header + storage block out of the game-state arena, folding the
// block's total size into the running game-state crc, and initializes the header exactly as
// data_new does. Returns a pointer to the new header.
data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size)
{
    data_array *array;
    int32_t block_size;
    uint8_t *zero;
    int32_t i;

    block_size = (int32_t)maximum_count * (int32_t)element_size + 0x38;
    array = (data_array *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + block_size;
    crc32_update(&game_state_crc, (uint8_t *)&block_size, 4);

    zero = (uint8_t *)array;
    for (i = 0xe; i != 0; i = i - 1) {
        zero[0] = 0;
        zero[1] = 0;
        zero[2] = 0;
        zero[3] = 0;
        zero = zero + 4;
    }
    strncpy(array->name, name, 0x1f);
    array->maximum_count = maximum_count;
    array->size = element_size;
    array->signature = k_data_array_signature; // '@t@d' in memory order, reads 'd@t@'
    array->data = (uint8_t *)array + 0x38;
    array->valid = 0;
    return array;
}

#if 0
Original Ghidra decompilation (0x5380d0):

char * game_state_new(char *param_1,short param_2)

{
  char *_Dest;
  short sVar1;
  int iVar2;
  short unaff_BX;
  char *pcVar3;

  sVar1 = param_2;
  _param_2 = (int)param_2 * (int)unaff_BX + 0x38;
  _Dest = (char *)(DAT_006e2dcc + DAT_006e2dc8);
  DAT_006e2dcc = DAT_006e2dcc + _param_2;
  crc32_update(&DAT_006e2dd4,&param_2,4);
  pcVar3 = _Dest;
  for (iVar2 = 0xe; iVar2 != 0; iVar2 = iVar2 + -1) {
    pcVar3[0] = '\0';
    pcVar3[1] = '\0';
    pcVar3[2] = '\0';
    pcVar3[3] = '\0';
    pcVar3 = pcVar3 + 4;
  }
  _strncpy(_Dest,param_1,0x1f);
  *(short *)(_Dest + 0x20) = sVar1;
  *(short *)(_Dest + 0x22) = unaff_BX;
  _Dest[0x28] = '@';
  _Dest[0x29] = 't';
  _Dest[0x2a] = '@';
  _Dest[0x2b] = 'd';
  *(char **)(_Dest + 0x34) = _Dest + 0x38;
  _Dest[0x24] = '\0';
  return _Dest;
}
#endif
