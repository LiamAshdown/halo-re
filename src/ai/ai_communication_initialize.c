// ai_communication_initialize  (Ghidra: ai_communication_initialize, already named)
// address 0x42cf20, size 775 bytes
// name confidence: 0.9   rewrite confidence: 0.6
// evidence: out/phase4/ai_types_notes.md's own "What the binary itself fixed" section cites
// this function directly: it builds the ai_conversation data_array inline with
// strncpy(dest, "ai conversation", 0x1f), maximum_count = 8, size = 0x64 (matching
// types/memory.h's data_array header exactly, including the 'd@t@' signature), and the two
// count/base globals at 0x6f0c98/0x6f0c9c and 0x6f0ca0/0x6f0ca4 are "the two communication
// timestamp tables 0x42d230 resets". The four inlined CRC-32 rolls (matching
// src/memory/crc32_update.c's algorithm exactly, one 4-byte value each) are replaced with
// direct calls to that function; game_state_base/_cursor/_crc are this repo's established
// names for the raw bump allocator DAT_006e2dc8/cc/d4.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
//
// UNSURE: the two static .rdata tables at 0x655aa0 (stride 0x28) and 0x656b08 (stride 0x24)
// are not modeled as named structs -- neither appears in types/tags.h, and their element
// layout beyond the terminator (offset 0x28 = the field this loop tests) and the
// conversation-index field (also offset 0x28 within the first table) is not established
// here. Treated as opaque byte arrays, matching how Ghidra itself decompiled the walk.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern int16_t communication_line_count;    // 0x006f0c98, UNSURE name
extern int32_t communication_line_base;     // 0x006f0c9c, UNSURE name
extern int16_t conversation_line_count;     // 0x006f0ca0, UNSURE name
extern int32_t conversation_line_base;      // 0x006f0ca4, UNSURE name
extern int16_t conversation_index_lookup[0x39]; // 0x008802e0, UNSURE size/name (57 entries: sVar6 runs 0..0x38)
extern data_array *ai_conversation_data;    // 0x008802d4
extern uint8_t ai_communication_line_table[];                          // 0x00655aa0, stride 0x28
extern ai_communication_event_definition ai_communication_event_definitions[]; // 0x00656b08, stride 0x24

extern uint8_t *game_state_base;   // 0x006e2dc8
extern int32_t game_state_cursor;  // 0x006e2dcc
extern uint32_t game_state_crc;    // 0x006e2dd4

extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0

// blam-cc: (no arguments)
// Initializes the AI conversation/communication subsystem: counts the two static
// communication-line tables (allocating their per-line timestamp tracking out of the game
// state the first time each is needed), builds the reverse lookup from conversation index to
// its position in the first table, and hand-builds the 8-element "ai conversation" data_array
// out of the game state.
void ai_communication_initialize(void)
{
    uint8_t *entry;
    int16_t conversation_index;
    int16_t position;
    int16_t next_conversation_index;
    uint8_t *dest;
    uint8_t *header;
    int32_t i;

    communication_line_count = 0;
    entry = ai_communication_line_table;
    do {
        entry = entry + 0x28;
        communication_line_count = communication_line_count + 1;
    } while (*(int16_t *)entry != -1);

    if (communication_line_base == 0) {
        int32_t count = communication_line_count;
        communication_line_base = (int32_t)(game_state_base + game_state_cursor);
        game_state_cursor = game_state_cursor + count * 0x10;
        crc32_update(&game_state_crc, (uint8_t *)&count, 4);
    }

    conversation_line_count = 0;
    entry = (uint8_t *)ai_communication_event_definitions;
    do {
        entry = entry + 0x24;
        conversation_line_count = conversation_line_count + 1;
    } while (*(int16_t *)entry != -1);

    if (conversation_line_base == 0) {
        int32_t count = conversation_line_count;
        conversation_line_base = (int32_t)(game_state_base + game_state_cursor);
        game_state_cursor = game_state_cursor + count * 0x10;
        crc32_update(&game_state_crc, (uint8_t *)&count, 4);
    }

    conversation_index = 0;
    for (;;) {
        conversation_index_lookup[conversation_index] = -1;
        entry = ai_communication_line_table;
        position = 0;
        next_conversation_index = *(int16_t *)(entry + 0x28);
        for (;;) {
            if (position == conversation_index) {
                conversation_index_lookup[conversation_index] = position;
                break;
            }
            if (next_conversation_index == -1) {
                break;
            }
            entry = entry + 0x28;
            next_conversation_index = *(int16_t *)(entry + 0x28);
            position = position + 1;
        }
        conversation_index = conversation_index + 1;
        if (0x38 < conversation_index) {
            break;
        }
    }

    dest = game_state_base + game_state_cursor;
    game_state_cursor = game_state_cursor + 0x358;
    {
        int32_t allocation_size = 0x358; // matches this allocation's own byte count exactly
        crc32_update(&game_state_crc, (uint8_t *)&allocation_size, 4);
    }

    header = dest;
    for (i = 0; i < 0x38; i++) {
        header[i] = 0;
    }
    strncpy((char *)dest, "ai conversation", 0x1f);
    ai_conversation_data = (data_array *)dest;
    ai_conversation_data->maximum_count = 8;
    ai_conversation_data->size = 0x64;
    ai_conversation_data->valid = 0;
    ai_conversation_data->signature = k_data_array_signature;
    ai_conversation_data->data = dest + 0x38;
}

#if 0
Original Ghidra decompilation (0x42cf20):

void __cdecl ai_communication_initialize(void)

{
  char *_Dest;
  short *psVar1;
  uint uVar2;
  short sVar3;
  uint uVar4;
  undefined *puVar5;
  short sVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  short sVar10;
  uint *puVar11;
  char *pcVar12;

  DAT_006f0c98 = 0;
  psVar1 = (short *)&DAT_00655aa0;
  do {
    psVar1 = psVar1 + 0x14;
    DAT_006f0c98 = DAT_006f0c98 + 1;
  } while (*psVar1 != -1);
  if (DAT_006f0c9c == 0) {
    uVar7 = (uint)DAT_006f0c98;
    iVar9 = DAT_006e2dc8 + DAT_006e2dcc;
    iVar8 = DAT_006e2dcc + uVar7 * 0x10;
    if (DAT_00719cd8 == '\0') {
      crc32_build_table();
      DAT_00719cd8 = '\x01';
    }
    uVar2 = DAT_006e2dd4 >> 8 ^ (&DAT_006b7b00)[(uVar7 * 0x10 & 0xff ^ DAT_006e2dd4) & 0xff];
    uVar2 = uVar2 >> 8 ^ (&DAT_006b7b00)[((uVar7 & 0xfffffff) >> 4 & 0xff ^ uVar2) & 0xff];
    uVar2 = uVar2 >> 8 ^ (&DAT_006b7b00)[((uVar7 & 0xfffffff) >> 0xc & 0xff ^ uVar2) & 0xff];
    DAT_006e2dd4 = uVar2 >> 8 ^ (&DAT_006b7b00)[((uVar7 & 0xfffffff) >> 0x14 ^ uVar2) & 0xff];
    DAT_006e2dcc = iVar8;
    DAT_006f0c9c = iVar9;
  }
  DAT_006f0ca0 = 0;
  psVar1 = &DAT_00656b08;
  do {
    psVar1 = psVar1 + 0x12;
    DAT_006f0ca0 = DAT_006f0ca0 + 1;
  } while (*psVar1 != -1);
  if (DAT_006f0ca4 == 0) {
    DAT_006f0ca4 = DAT_006e2dc8 + DAT_006e2dcc;
    uVar7 = (uint)DAT_006f0ca0;
    DAT_006e2dcc = DAT_006e2dcc + uVar7 * 0x10;
    if (DAT_00719cd8 == '\0') {
      uVar2 = 0;
      puVar11 = &DAT_006b7b00;
      iVar9 = 0x100;
      do {
        iVar8 = 8;
        uVar4 = uVar2;
        do {
          if ((uVar4 & 1) == 0) {
            uVar4 = uVar4 >> 1;
          }
          else {
            uVar4 = uVar4 >> 1 ^ 0xedb88320;
          }
          iVar8 = iVar8 + -1;
        } while (iVar8 != 0);
        *puVar11 = uVar4;
        uVar2 = uVar2 + 1;
        puVar11 = puVar11 + 1;
        iVar9 = iVar9 + -1;
      } while (iVar9 != 0);
      DAT_00719cd8 = '\x01';
    }
    uVar2 = DAT_006e2dd4 >> 8 ^ (&DAT_006b7b00)[(uVar7 * 0x10 & 0xff ^ DAT_006e2dd4) & 0xff];
    uVar2 = uVar2 >> 8 ^ (&DAT_006b7b00)[((uVar7 & 0xfffffff) >> 4 & 0xff ^ uVar2) & 0xff];
    uVar2 = uVar2 >> 8 ^ (&DAT_006b7b00)[((uVar7 & 0xfffffff) >> 0xc & 0xff ^ uVar2) & 0xff];
    DAT_006e2dd4 = uVar2 >> 8 ^ (&DAT_006b7b00)[((uVar7 & 0xfffffff) >> 0x14 ^ uVar2) & 0xff];
  }
  sVar6 = 0;
  psVar1 = &DAT_008802e0;
  do {
    sVar10 = 0;
    *psVar1 = -1;
    puVar5 = &DAT_00655aa0;
    sVar3 = 0;
    do {
      if (sVar3 == sVar6) {
        *psVar1 = sVar10;
        break;
      }
      sVar3 = *(short *)(puVar5 + 0x28);
      puVar5 = puVar5 + 0x28;
      sVar10 = sVar10 + 1;
    } while (sVar3 != -1);
    sVar6 = sVar6 + 1;
    psVar1 = psVar1 + 1;
    if (0x38 < sVar6) {
      _Dest = (char *)(DAT_006e2dc8 + DAT_006e2dcc);
      DAT_006e2dcc = DAT_006e2dcc + 0x358;
      if (DAT_00719cd8 == '\0') {
        uVar7 = 0;
        puVar11 = &DAT_006b7b00;
        iVar9 = 0x100;
        do {
          iVar8 = 8;
          uVar2 = uVar7;
          do {
            if ((uVar2 & 1) == 0) {
              uVar2 = uVar2 >> 1;
            }
            else {
              uVar2 = uVar2 >> 1 ^ 0xedb88320;
            }
            iVar8 = iVar8 + -1;
          } while (iVar8 != 0);
          *puVar11 = uVar2;
          uVar7 = uVar7 + 1;
          puVar11 = puVar11 + 1;
          iVar9 = iVar9 + -1;
        } while (iVar9 != 0);
        DAT_00719cd8 = '\x01';
      }
      uVar7 = DAT_006e2dd4 >> 8 ^ (&DAT_006b7b00)[(DAT_006e2dd4 ^ 0x58) & 0xff];
      uVar7 = uVar7 >> 8 ^ (&DAT_006b7b00)[(uVar7 ^ 3) & 0xff];
      uVar7 = uVar7 >> 8 ^ (&DAT_006b7b00)[uVar7 & 0xff];
      DAT_006e2dd4 = uVar7 >> 8 ^ (&DAT_006b7b00)[uVar7 & 0xff];
      pcVar12 = _Dest;
      for (iVar9 = 0xe; iVar9 != 0; iVar9 = iVar9 + -1) {
        pcVar12[0] = '\0';
        pcVar12[1] = '\0';
        pcVar12[2] = '\0';
        pcVar12[3] = '\0';
        pcVar12 = pcVar12 + 4;
      }
      _strncpy(_Dest,"ai conversation",0x1f);
      _Dest[0x20] = '\b';
      _Dest[0x21] = '\0';
      _Dest[0x22] = 'd';
      _Dest[0x23] = '\0';
      _Dest[0x28] = '@';
      _Dest[0x29] = 't';
      _Dest[0x2a] = '@';
      _Dest[0x2b] = 'd';
      *(char **)(_Dest + 0x34) = _Dest + 0x38;
      _Dest[0x24] = '\0';
      DAT_008802d4 = _Dest;
      return;
    }
  } while( true );
}
#endif
