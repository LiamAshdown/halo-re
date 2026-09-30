// network_game_client_connect_to_address_async  (Ghidra: network_game_client_connect_to_address_async,
// already named)
// address 0x4c8500, size 341 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/main_functions.md summary; confirmed against objdump -d -M intel
// bin/halo.exe 0x4c8500..0x4c8654. Ghidra's own decompile mislabels a scratch stack slot as
// "address" in two places (`(uchar *)&address` for network_address_string_normalize's
// out_is_any argument, and the later `(char)address == '\0'` test) purely because the compiler
// reused that stack offset; disassembly shows the real operand of both is the out_is_any byte
// network_address_string_normalize just wrote (`cmp BYTE PTR [esp+0x2c],bl` at 0x4c856c, no
// reload of the address argument), so this rewrite names it is_any and reads it, not `address`.
// register convention: __cdecl, both parameters ordinary stack (esi/edi at entry are just the
// compiler's register-allocated copies of them, confirmed at 0x4c8505/0x4c850e:
// `mov esi,[esp+0x28]` / `mov edi,[esp+0x30]`, i.e. the two stack arguments after the 3 pushed
// callee-saved registers).
// phase 4 review (disassembly 0x4c8500..0x4c8654): the normalize buffer is 0x20 bytes
// ([esp+0xc] up to is_any at [esp+0x2c]), not 0x19; the result is returned in AL (uint8_t).
// UNSURE: the GlobalAlloc size computed for the hostname copy (0x4c85b3..0x4c85c1) is
// strlen(address)+1, matching a plain strcpy of address into the new buffer, immediately below.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include "fn_networking.h"
#include "fn_main.h"
#include <string.h>

extern main_globals main_globals_data; // 0x00719700
extern void *connect_thread;   // 0x00719b64, this module; HANDLE of the hostname worker

extern int32_t join_ui_state; // 0x00718f8c, foreign (interface module); see
    // src/networking/network_join_request_resolve_host.c for the same global under this name


    // blam-cc: EAX -> address_string


    // blam-cc: EAX -> address_string, stack -> port_out
extern void widget_close_all(void);              // 0x498650, foreign (interface module)
extern void interface_loading_screen_reset(void); // 0x4978d0, foreign
extern void interface_loading_screen_set_text(const char *text); // 0x4978a0, foreign; blam-cc: EAX -> text


extern void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal,
                           uint8_t is_error); // 0x498f20, foreign (interface module)

// Stages a "connect to address[:port], with optional password" request. With both arguments
// NULL, just clears the staged connect state and returns success (the "cancel" call other
// functions in this module make before showing an error). Otherwise validates address: a
// syntactically valid, already-numeric (or exact) address is staged directly into
// main_globals_data.connect_address; one that looks like a resolvable "host[:port]" instead spins up
// a worker thread running network_game_client_connect_by_hostname on a heap copy of it (waiting
// for any previous such worker to finish first). An address that is neither returns failure
// (the caller, per the disassembly's other call sites, follows up with display_error).
uint8_t network_game_client_connect_to_address_async(char *address, char *password)
{
    char normalized[0x20];          // [esp+0x0c] .. [esp+0x2b]
    uint8_t is_any;
    uint32_t thread_id;
    size_t length;
    char *host_copy;

    if (address == 0 && password == 0) {
        main_globals_data.connect_address[0] = 0;
        main_globals_data.connect_password[0] = 0;
        main_globals_data.connect_pending = 0;
        return 1;
    }

    if (!network_address_string_is_valid(address)) {
        goto fail;
    }

    strncpy(main_globals_data.connect_password, password, 8);
    main_globals_data.connect_password[8] = 0;

    if (!network_address_string_normalize(address, normalized, &is_any)) {
        if (!network_address_parse_port(address, 0)) {
            goto fail;
        }
        length = strlen(address) + 1;
        host_copy = GlobalAlloc(0, length);
        strcpy(host_copy, address);

        widget_close_all();
        interface_loading_screen_reset();
        join_ui_state = 3;
        interface_loading_screen_set_text(address);

        while (connect_thread != 0) {
            Sleep(0);
        }
        connect_thread = CreateThread(0, k_main_connect_thread_stack_size,
                                      (void *)network_game_client_connect_by_hostname, host_copy, 0,
                                      &thread_id);
        return 1;
    }

    if (is_any != 0) {
        goto fail;
    }
    main_globals_data.connect_pending = 1;
    strncpy(main_globals_data.connect_address, normalized, 0x1f);
    main_globals_data.connect_address[0x1f] = 0;
    return 1;

fail:
    network_game_client_connect_to_address_async(0, 0);
    display_error(0x35, -1, 1, 0);
    return 0;
}

#if 0
Original Ghidra decompilation (0x4c8500):

int __cdecl network_game_client_connect_to_address_async(char *address,char *password)

{
  char cVar1;
  undefined4 in_EAX;
  char *pcVar2;
  char *pcVar3;
  HGLOBAL lpParameter;
  uint extraout_EAX;
  int iVar4;
  char local_1c [28];

  pcVar3 = password;
  pcVar2 = address;
  if ((address == (char *)0x0) && (password == (char *)0x0)) {
    DAT_00719a7a = 0;
    DAT_00719a9a = 0;
    DAT_00719a79 = 0;
    return CONCAT31((int3)((uint)in_EAX >> 8),1);
  }
  cVar1 = FUN_004dc730();
  if (cVar1 != '\0') {
    _strncpy(&DAT_00719a9a,pcVar3,8);
    DAT_00719aa2 = 0;
    cVar1 = network_address_string_normalize(pcVar2,local_1c,(uchar *)&address);
    if (cVar1 == '\0') {
      cVar1 = network_address_parse_port((long *)0x0);
      if (cVar1 != '\0') {
        pcVar3 = pcVar2;
        do {
          cVar1 = *pcVar3;
          pcVar3 = pcVar3 + 1;
        } while (cVar1 != '\0');
        lpParameter = GlobalAlloc(0,(SIZE_T)(pcVar3 + (1 - (int)(pcVar2 + 1))));
        iVar4 = (int)lpParameter - (int)pcVar2;
        do {
          cVar1 = *pcVar2;
          pcVar2[iVar4] = cVar1;
          pcVar2 = pcVar2 + 1;
        } while (cVar1 != '\0');
        widget_close_all();
        interface_loading_screen_reset();
        DAT_00718f8c = 3;
        FUN_004978a0();
        while (DAT_00719b64 != (HANDLE)0x0) {
          Sleep(0);
        }
        DAT_00719b64 = CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x4000,
                                    network_game_client_connect_by_hostname,lpParameter,0,
                                    (LPDWORD)&address);
        return CONCAT31((int3)((uint)DAT_00719b64 >> 8),1);
      }
    }
    else if ((char)address == '\0') {
      DAT_00719a79 = 1;
      pcVar2 = _strncpy(&DAT_00719a7a,local_1c,0x1f);
      DAT_00719a99 = 0;
      return CONCAT31((int3)((uint)pcVar2 >> 8),1);
    }
  }
  network_game_client_connect_to_address_async((char *)0x0,(char *)0x0);
  display_error(0x35,-1,'\x01','\0');
  return extraout_EAX & 0xffffff00;
}
#endif
