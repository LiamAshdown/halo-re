// player_update_history_log_printf_filtered  (Ghidra: player_update_history_log_printf_filtered,
// already named)
// address 0x4e5f20, size 85 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md; player_update_history_log_write.c (this batch,
// 0x4e5ea0) and player_update_history_log_set_name_filter.c (this batch, 0x4e5f80) for the two
// globals this function bridges between.
// register convention: disassembly (objdump -d -M intel) shows EAX carries a player pointer
// (`add eax,0x4` before the wcscmp, matching player::name at +0x04) and pins the stack layout,
// which does not match Ghidra's own 2-parameter guess:
//   4e5f3b: mov edx,[esp+0x408]   ; format string           (Ghidra's "param_2")
//   4e5f42: lea ecx,[esp+0x40c]   ; &format_arg, the vsprintf va_list start
//   4e5f55: mov eax,[esp+0x410]   ; category_flags, passed on to player_update_history_log_write
// The first stack slot ([esp+0x404], where Ghidra's "param_1" would fall) is never read.
// register convention: EAX -> target_player, stack -> unused_arg, format, ...
//   // blam-cc: EAX -> target_player, stack -> unused_arg, format, ...
// UNSURE: unused_arg's purpose; it is never read by this function, so it is kept only to match
// the real stack layout for any caller elsewhere in the module.
// UNSURE (revised after cross-checking every caller in this batch): the "category_flags" read at
// [esp+0x410] assumes the format string consumes exactly one trailing vararg before it. That
// holds for none of this batch's three known call sites (all inside handle_remote_player_action_update.c,
// this module) -- each pushes only (unused_arg, format, one data word) and never a fourth value,
// and one of them (player_update_client_remote_player_total_biped_update_from_network.c) uses a
// two-specifier format string, which would consume the "category_flags" slot as its own second
// vararg instead. In every observed case the true category_flags value is therefore either
// unsupplied stack garbage or actually part of the message text. This rewrite makes the function
// properly variadic (so multi-argument format strings work correctly, matching vsprintf's real
// memory-forwarding behaviour) and always passes category_flags = 0 to
// player_update_history_log_write, which is the only value consistent with every known caller.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint16_t local_player_name_filter[0x400]; // this module, 0x0071c420

extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...); // this module, 0x4e5ea0

// Only when target_player's name matches the debug filter (player_update_history_log_set_name_filter),
// formats format (with its trailing varargs) into a scratch buffer and forwards it verbatim (as
// the format string, with no further arguments) to player_update_history_log_write under the
// "filtered" category mask. See the UNSURE note above for category_flags.
void player_update_history_log_printf_filtered(player *target_player, int32_t unused_arg,
    const char *format, ...)
    // blam-cc: EAX -> target_player, stack -> unused_arg, format, ...
{
    char buffer[0x400];
    va_list args;

    if (wcscmp((wchar_t *)target_player->name, (wchar_t *)local_player_name_filter) != 0) {
        return;
    }
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    player_update_history_log_write(0, 1, buffer);
}

#if 0
Original Ghidra decompilation (0x4e5f20), from tools/pack.py 0x4e5f20:

void player_update_history_log_printf_filtered(undefined4 param_1,char *param_2)

{
  int in_EAX;
  int iVar1;
  char local_400 [1024];

  iVar1 = _wcscmp((wchar_t *)(in_EAX + 4),&DAT_0071c420);
  if (iVar1 == 0) {
    _vsprintf(local_400,param_2,&stack0x0000000c);
    player_update_history_log_write(local_400);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
