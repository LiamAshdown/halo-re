// network_session_host_dispatch_message  (Ghidra: FUN_00577e40; the qr2 player key callback)
// address 0x577e40, size 241 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x577e40..0x577f30: the qr2 player key callback (key, index, buffer, user
//   data); the earlier version modeled three arguments. The player at that active index (validated: index in range,
//   live, salt 0 or matching) has key 0x15 its name (at most 0x40 characters, ASCII) and 0x19 its team; the game
//   engine's +0xa0 hook answers other keys; anything unanswered is empty. (Name kept.)
// blam-cc: cdecl (a qr2 player key callback)

#include "tags.h"
#include <string.h>
#include <wchar.h>

extern void *current_game_engine; // 0x006f1d20 (game_engine_definition *; +0x9c/+0xa0/+0xa4/+0xa8 the query hooks)
extern void qr2_buffer_add(void *buffer, const char *value); // 0x615590 qr2_buffer_add
extern void qr2_buffer_add_int(void *buffer, int32_t value); // 0x616640 qr2_buffer_add_int
extern void qr2_keybuffer_add(void *keybuffer, int32_t key_id); // 0x615560 qr2_keybuffer_add
typedef struct data_array data_array;
extern uint8_t *player_data; // 0x0087a480 (data_array *)
extern uint32_t players_get_active_by_index(int32_t index); // 0x45c6f0, blam-cc: EAX index
extern uint8_t *string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity); // 0x557950

void network_session_host_dispatch_message(int32_t key_id, int32_t index, void *buffer, void *user_data)
{
    uint32_t handle = players_get_active_by_index(index);
    int16_t player_index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    uint8_t *player;

    (void)user_data;
    if (handle == 0xffffffff || player_index < 0 || player_index >= *(int16_t *)(player_data + 0x20)) {
        qr2_buffer_add(buffer, "");
        return;
    }
    player = *(uint8_t **)(player_data + 0x34) + player_index * *(int16_t *)(player_data + 0x22);
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt)) {
        qr2_buffer_add(buffer, "");
        return;
    }
    if (key_id == 0x15) {
        uint8_t name[0x40];

        memset(name, 0, sizeof(name));
        qr2_buffer_add(buffer, (const char *)string_convert_unicode_to_ascii(name, (uint16_t *)(player + 4), 0x40));
        return;
    }
    if (key_id == 0x19) {
        qr2_buffer_add_int(buffer, *(int32_t *)(player + 0x20));
        return;
    }
    if (current_game_engine != 0) {
        uint8_t (*hook)(int32_t, int32_t, void *) = *(uint8_t (**)(int32_t, int32_t, void *))((uint8_t *)current_game_engine + 0xa0);

        if (hook != 0 && hook(key_id, index, buffer) != 0) {
            return;
        }
    }
    qr2_buffer_add(buffer, "");
}

#if 0
Original Ghidra decompilation (0x577e40):

void FUN_00577e40(int param_1,undefined4 param_2,undefined4 param_3)

{
  char cVar1;
  short sVar2;
  int iVar3;
  undefined4 uVar4;
  short *psVar5;
  short sVar6;
  undefined4 *puVar7;
  undefined4 local_3f;

  iVar3 = FUN_0045c6f0();
  if (((iVar3 != -1) && (sVar2 = (short)iVar3, -1 < sVar2)) &&
     (sVar2 < *(short *)(DAT_0087a480 + 0x20))) {
    psVar5 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar2 +
                      *(int *)(DAT_0087a480 + 0x34));
    sVar2 = *psVar5;
    if ((sVar2 != 0) && ((sVar6 = (short)((uint)iVar3 >> 0x10), sVar6 == 0 || (sVar2 == sVar6)))) {
      if (param_1 == 0x15) {
        puVar7 = &local_3f;
        for (iVar3 = 0xf; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar7 = 0;
          puVar7 = puVar7 + 1;
        }
        *(undefined2 *)puVar7 = 0;
        *(undefined1 *)((int)puVar7 + 2) = 0;
        uVar4 = FUN_00557950(0x40);
        FUN_00615590(param_3,uVar4);
        return;
      }
      if (param_1 == 0x19) {
        FUN_00616640(param_3,*(undefined4 *)(psVar5 + 0x10));
        return;
      }
      if (((DAT_006f1d20 != 0) && (*(code **)(DAT_006f1d20 + 0xa0) != (code *)0x0)) &&
         (cVar1 = (**(code **)(DAT_006f1d20 + 0xa0))(param_1,param_2,param_3), cVar1 != '\0')) {
        return;
      }
    }
  }
  FUN_00615590(param_3,&DAT_0065512c);
  return;
}
#endif
