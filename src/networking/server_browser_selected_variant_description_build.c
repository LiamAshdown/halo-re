// server_browser_selected_variant_description_build  (Ghidra: FUN_004b74e0, still unnamed ->
// renamed)
// address 0x4b74e0, size 207 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md summary ("builds and displays a text
// description of the selected server's game variant / fraglimit settings in the server-browser
// UI"); the four literal keys (player_flags, game_flags, gamevariant, fraglimit) match the
// four GameSpy accessor calls; multiplayer_game_variant_description_generate (0x4b8da0) is
// documented in networking_types_notes.md's "misattributed" list as UI text generation over
// game_variant, owned by types/game.h -- called here, rewritten in src/game/.
// register convention: GameSpy entry pointer in ECX (in_ECX).
// The four accessor calls (disassembly 0x4b750e..0x4b7546) read, in order: "player_flags" (string,
// default "") -> EDX of the description generator; "game_flags" (int) -> its `fraglimit`
// (packed engine/flags) parameter; "gamevariant" (string) and "fraglimit" (string, default "0")
// -> each widened into a 0x800-byte stack buffer and passed as its last two arguments. The
// generator's own parameter names (variant_name / fraglimit / game_flags_wide / player_flags_wide)
// are misnomers for what the browser keys actually carry.
// Return value is AL = 1 on every path.

// VERIFIED against disassembly 0x4b74e0..0x4b75af (2026-09-30): FIXED: the four key/role assignments were wrong (player_flags string -> EDX, game_flags int -> fraglimit arg, gamevariant and fraglimit strings widened into the last two args) and the generator extern lacked its EDX variant_name argument
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>


#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t SBServerGetIntValue(void *entry, const char *key, int32_t default_value); // foreign, GameSpy int accessor // foreign, GameSpy library, int accessor
extern char *SBServerGetStringValue(void *entry, const char *key, const char *default_value); // foreign, GameSpy library, string accessor
// blam-cc: EAX -> dest, EDI -> dest capacity in BYTES, EBX -> ASCII source.
// Widens an ASCII string into dest and returns dest, or NULL when it does not fit.
extern wchar_t *string_convert_ascii_to_unicode(wchar_t *dest, int32_t dest_bytes, const char *source); // 0x557990
extern void multiplayer_game_variant_description_generate(char *variant_name, ticker_text_buffer *ticker,
    int32_t fraglimit, wchar_t *game_flags_wide, wchar_t *player_flags_wide); // 0x4b8da0; blam-cc: EDX -> variant_name, stack -> ticker, fraglimit, game_flags_wide, player_flags_wide
extern ticker_text_buffer server_browser_variant_ticker; // 0x006b5e74 (EDI at 0x4b74f7)
extern void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self); // 0x4b8a60, this module

// blam-cc: ECX -> entry
int32_t server_browser_selected_variant_description_build(void *entry)
{
    char *player_flags;
    int32_t game_flags;
    char *gamevariant;
    char *fraglimit;
    wchar_t gamevariant_wide[0x400];  // the 0x800-byte stack scratch at [esp+0x814]
    wchar_t fraglimit_wide[0x400];    // the 0x800-byte stack scratch at [esp+0x14]

    ticker_text_buffer_append(0, 1, &server_browser_variant_ticker);
    if (entry != 0) {
        player_flags = SBServerGetStringValue(entry, "player_flags", "");
        game_flags = SBServerGetIntValue(entry, "game_flags", 0);
        gamevariant = SBServerGetStringValue(entry, "gamevariant", 0);
        fraglimit = SBServerGetStringValue(entry, "fraglimit", "0");
        if (player_flags != 0 && game_flags != 0) {
            string_convert_ascii_to_unicode(gamevariant_wide, 0x800, gamevariant);
            string_convert_ascii_to_unicode(fraglimit_wide, 0x800, fraglimit);
            multiplayer_game_variant_description_generate(player_flags, &server_browser_variant_ticker,
                game_flags, gamevariant_wide, fraglimit_wide);
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4b74e0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 FUN_004b74e0(void)

{
  int iVar1;
  int iVar2;
  int in_ECX;
  undefined1 local_1000 [2048];
  undefined1 local_800 [2044];
  undefined4 uStack_4;

  uStack_4 = 0x4b74ea;
  ticker_text_buffer_append(0,1);
  if (in_ECX != 0) {
    iVar1 = FUN_00617490();
    iVar2 = FUN_00617c10();
    FUN_00617490();
    FUN_00617490();
    if ((iVar1 != 0) && (iVar2 != 0)) {
      FUN_00557990();
      FUN_00557990();
      multiplayer_game_variant_description_generate(&DAT_006b5e74,iVar2,local_800,local_1000);
    }
    return 1;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
