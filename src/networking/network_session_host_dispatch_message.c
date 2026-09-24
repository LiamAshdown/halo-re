// network_session_host_dispatch_message  (Ghidra: FUN_00577e40; named per this rewrite)
// address 0x577e40, size 241 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary: "Validates an incoming network message's
// sender against a machine table and dispatches it to a type-specific or generic handler,
// replying with an error string if invalid." Registered as the message-handler callback in
// network_session_host_start (0x577850).
// register convention: the three __cdecl stack parameters Ghidra recognized.
// UNSURE: the exact shape of the "machine table" record read through DAT_0087a480 (a table
// header: count at +0x20, stride at +0x22, base pointer at +0x34) is not attested anywhere else
// in this module and is not declared as a type; accessed here via raw offsets. FUN_0045c6f0
// (identifies the sender), FUN_00557950 and the two FUN_006155xx/FUN_006166xx GameSpy-shaped
// reply helpers are foreign.
// reconciled: R04 0x006f1d20 void * network_game_engine_callback_block -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t *network_session_machine_table; // 0x0087a480, UNSURE layout, see file header
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern char *network_session_generic_error_string; // 0x0065512c, UNSURE

extern int32_t FUN_0045c6f0(void); // foreign, identifies the sender (index in low word, salt in high word)
extern char *FUN_00557950(int32_t id); // foreign, UNSURE
extern void FUN_00615590(void *reply_target, void *text); // foreign, UNSURE
extern void FUN_00616640(void *reply_target, int32_t value); // foreign, UNSURE

// Validates an incoming network message's sender against the machine table and dispatches it to
// a type-specific handler (message type 0x15 replies with a formatted string, 0x19 replies with a
// stored value) or, failing that, to the generic ownership-handoff callback; replies with a
// generic error string if the sender could not be validated.
void network_session_host_dispatch_message(int32_t message_type, int32_t param_2, void *reply_target)
{
    int32_t sender = FUN_0045c6f0();

    if (sender != -1) {
        int16_t index = (int16_t)sender;
        if (index >= 0 && index < *(int16_t *)(network_session_machine_table + 0x20)) {
            uint8_t *entry = network_session_machine_table +
                             (int32_t)(*(int16_t *)(network_session_machine_table + 0x22)) * index +
                             *(int32_t *)(network_session_machine_table + 0x34);
            int16_t entry_salt = *(int16_t *)entry;
            int16_t sender_salt = (int16_t)((uint32_t)sender >> 16);

            if (entry_salt != 0 && (sender_salt == 0 || entry_salt == sender_salt)) {
                if (message_type == 0x15) {
                    char text[64];
                    int32_t i;
                    for (i = 0; i < 64; i++) {
                        text[i] = 0;
                    }
                    FUN_00615590(reply_target, FUN_00557950(0x40));
                    (void)text;
                    return;
                }
                if (message_type == 0x19) {
                    FUN_00616640(reply_target, *(int32_t *)(entry + 0x40));
                    return;
                }
                if (current_game_engine != 0 &&
                    *(void **)((uint8_t *)current_game_engine + 0xa0) != 0) {
                    typedef char (*handoff_fn)(int32_t, int32_t, void *);
                    handoff_fn handoff = *(handoff_fn *)((uint8_t *)current_game_engine + 0xa0);
                    if (handoff(message_type, param_2, reply_target) != 0) {
                        return;
                    }
                }
            }
        }
    }
    FUN_00615590(reply_target, network_session_generic_error_string);
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
