// network_join_hostname_resolved_callback  (Ghidra: FUN_004ba270; named per this rewrite)
// address 0x4ba270, size 174 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Completion callback for the
// asynchronous hostname-resolve used when joining a server by name: stores the resolved
// address/port, or falls back to a direct connect-by-string path").
// register convention: Ghidra recognizes all three as ordinary stack parameters (cc=unknown,
// no in_/unaff_ registers), so they are kept as plain parameters with no blam-cc remapping.
// UNSURE: `hostent` is read only by raw offset (+0x02 a 16-bit field, +0x04 a 32-bit field),
// matching a Winsock `hostent`-shaped async-resolve buffer (h_addrtype-ish / first address
// dword), but nothing in this batch confirms the real type; declared as a raw byte pointer.
// UNSURE: the resolved address FUN_006148b0 builds into the stack buffer is never read again in
// this function -- the actual connect call below it always uses one of two persistent globals
// instead. Preserved exactly as decompiled; this may be dead code in the retail build or a
// buffer Ghidra mismatched, not "improved" per the task's no-invented-behaviour rule.
// UNSURE: `network_join_error_code`, `network_join_error_flags` and
// `network_join_target_address` are named from behavior only; FUN_006148a0/FUN_006148b0 are
// unnamed network glue functions outside this batch's address range.

#include "tags.h"
#include "memory.h"

extern uint8_t server_browser_join_target_has_password;   // 0x00719454, nonzero selects network_join_target_address
extern uint16_t network_join_target_address[128]; // 0x00719458, a hostname/address string
    // buffer; UNSURE: element count is a guess, only index 0 is touched in this batch
extern uint16_t empty_string;                   // 0x00660c34 (shared with the game module)
extern int16_t network_join_error_code;         // 0x00718fa4, -1 means "not yet set"
extern int32_t network_join_error_reason;       // 0x0071973c
extern uint8_t network_join_error_flags[4];     // 0x00719754: word 0xffff at +0, byte 1 at +3

// blam-cc: EAX -> address_string (0x4dc790 strcpy's it out of EAX), stack -> the wide
// hostname/password string. Both call sites in this module and the one at 0x4c86ea agree.
extern uint32_t network_game_client_connect_to_address(char *address_string,
                                                        uint16_t *target_string); // 0x4dc790
extern uint32_t FUN_006148a0(int16_t value);                                  // outside this batch
extern void FUN_006148b0(uint32_t address, uint16_t port, void *out_address); // 0x6148b0: fills a
    // 0x16-byte address record (stride confirmed by the imul esi,esi,0x16 at 0x6148c8)  // outside this batch

void network_join_hostname_resolved_callback(int32_t resolve_failed, uint32_t unused, uint8_t *hostent)
{
    char resolved_address[22]; // the 0x16-byte record FUN_006148b0 formats; FIXED in the review
                               // pass: it is not dead, it is the EAX argument of the connect
                               // call below (lea eax,[esp+0x4] at 0x4ba2c5 is this buffer)

    (void)unused;

    if (hostent != 0) {
        uint32_t address = *(uint32_t *)(hostent + 4);
        uint32_t port = FUN_006148a0(*(int16_t *)(hostent + 2));

        FUN_006148b0(address, port, resolved_address);
    }

    if (resolve_failed == 0) {
        uint16_t *address_string = server_browser_join_target_has_password == 0
            ? &empty_string
            : network_join_target_address;

        network_game_client_connect_to_address(resolved_address, address_string);
        network_join_target_address[0] = 0;
        server_browser_join_target_has_password = 0;
        return;
    }

    if (network_join_error_code == -1) {
        network_join_error_code = 0x2b;
    }
    network_join_error_reason = 0;
    *(uint16_t *)network_join_error_flags = 0xffff;
    network_join_error_flags[3] = 1;
}

#if 0
Original Ghidra decompilation (0x4ba270):

void FUN_004ba270(int param_1,undefined4 param_2,int param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined2 *puVar3;
  undefined2 local_18;
  undefined4 local_16;
  undefined4 local_12;
  undefined4 local_e;
  undefined4 local_a;
  undefined4 local_6;

  local_16 = 0;
  local_12 = 0;
  local_e = 0;
  local_a = 0;
  local_6 = 0;
  local_18 = 0;
  if (param_3 != 0) {
    uVar1 = *(undefined4 *)(param_3 + 4);
    uVar2 = FUN_006148a0(*(undefined2 *)(param_3 + 2));
    FUN_006148b0(uVar1,uVar2,&local_18);
  }
  if (param_1 == 0) {
    if (DAT_00719454 == '\0') {
      puVar3 = (undefined2 *)&DAT_00660c34;
    }
    else {
      puVar3 = &DAT_00719458;
    }
    network_game_client_connect_to_address(puVar3);
    DAT_00719458 = 0;
    DAT_00719454 = 0;
    return;
  }
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 0x2b;
  }
  DAT_0071973c = 0;
  DAT_00719754._0_2_ = 0xffff;
  DAT_00719754._3_1_ = 1;
  return;
}
#endif
