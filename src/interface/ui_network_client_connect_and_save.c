// ui_network_client_connect_and_save  (Ghidra: FUN_004a4a30, renamed)
// renamed from FUN_004a4a30 in the naming pass
// address 0x4a4a30, size 177 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.75
// evidence: functions.md: "Commits an edited text field (likely a server/game name) into the
// network options block and saves the player profile." Same selected_saved_item/
// saved_item_working_copy gate pattern as FUN_004a0050.c/FUN_004a0b50.c, implemented the same way
// (sensible reading, not literal AND-with-address transcription).
// register convention: none (void).
// Phase-4 s2 review against objdump 0x4a4a30..0x4a4ae0: 0x557950 narrows a wide string (ESI out,
// EDI wide source, stack length): the host name field at 0x00719238 (0x20) and the port field at
// 0x007191f0 (9). saved_item_select takes the current profile index in EBX. The working copy at
// 0x00714e80 is a buffer, not a pointer: when the selected saved item is a profile (low nibble
// 0) the host name is copied to its +0xfc2, otherwise the binary copies to address 0xfc2 (the
// sbb/not/and select yields NULL); the earlier rewrite had that test inverted.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint16_t network_host_name_field_00719238[32]; // 0x00719238
extern uint16_t network_host_port_field_007191f0[9];  // 0x007191f0
extern int32_t current_profile_index;                   // 0x00714dd4
extern int32_t selected_saved_item;                     // 0x00714e7c
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself

extern uint8_t * string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity); // 0x557950, blam-cc: ESI out, EDI wide source
extern uint8_t network_game_client_connect_to_address_async(char *name, char *address); // 0x4c8500
extern void saved_item_select(int32_t profile_index); // 0x495be0, blam-cc: EBX profile_index
extern uint32_t FUN_00625b7a(const uint16_t *s); // wide strlen
extern void _wcscpy(uint16_t *dest, const uint16_t *src);
extern uint8_t saved_item_has_unsaved_changes(void); // 0x495ea0
extern uint8_t player_profile_save(void); // 0x495d40

uint8_t ui_network_client_connect_and_save(void)
{
    char name[0x20];
    char port[0xc];
    uint8_t result;

    string_convert_unicode_to_ascii(name, network_host_name_field_00719238, 0x20);
    string_convert_unicode_to_ascii(port, network_host_port_field_007191f0, 9);
    result = network_game_client_connect_to_address_async(name, port);
    if (result == 0 || current_profile_index == -1) {
        return result;
    }

    saved_item_select(current_profile_index);
    {
        uint8_t *record = ((selected_saved_item & 0xf) == 0) ? saved_item_working_copy : (uint8_t *)0;
        FUN_00625b7a(network_host_name_field_00719238);
        _wcscpy((uint16_t *)(record + 0xfc2), network_host_name_field_00719238);
    }
    if (saved_item_has_unsaved_changes() != 0) {
        player_profile_save();
        return result;
    }
    selected_saved_item = -1;
    return result;
}

#if 0
Original Ghidra decompilation (0x4a4a30):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_004a4a30(void)

{
  uint uVar1;
  char cVar2;
  uint uVar3;
  char local_2c [12];
  char local_20 [32];

  FUN_00557950(0x20);
  FUN_00557950(9);
  uVar3 = network_game_client_connect_to_address_async(local_20,local_2c);
  if (((char)uVar3 != '\0') && (DAT_00714dd4 != -1)) {
    FUN_00495be0();
    uVar1 = _DAT_00714e7c & 0xf;
    FUN_00625b7a(&DAT_00719238);
    _wcscpy((wchar_t *)((~-(uint)(uVar1 != 0) & 0x714e80) + 0xfc2),(wchar_t *)&DAT_00719238);
    cVar2 = FUN_00495ea0();
    if (cVar2 != '\0') {
      player_profile_save();
      return uVar3 & 0xff;
    }
    uVar3 = uVar3 & 0xff;
    _DAT_00714e7c = 0xffffffff;
  }
  return uVar3;
}
#endif
