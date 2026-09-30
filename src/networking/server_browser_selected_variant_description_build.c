// server_browser_selected_variant_description_build  (Ghidra: FUN_004b74e0, still unnamed ->
// renamed)
// address 0x4b74e0, size 207 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md summary ("builds and displays a text
// description of the selected server's game variant / fraglimit settings in the server-browser
// UI"); the four literal keys (player_flags, game_flags, gamevariant, fraglimit) match the
// four GameSpy accessor calls; multiplayer_game_variant_description_generate (0x4b8da0) is
// documented in networking_types_notes.md's "misattributed" list as UI text generation over
// game_variant, owned by types/game.h -- called here but not rewritten in this batch.
// register convention: GameSpy entry pointer in ECX (in_ECX).
// UNSURE: the exact key assigned to each of the four accessor calls is inferred from field
// role, not from a visible argument (every accessor call shows zero visible arguments):
// call 1 (string) = "gamevariant", call 2 (int) = "fraglimit", call 3 (string) = "game_flags",
// call 4 (string) = "player_flags". string_convert_ascii_to_unicode (called twice, once per flags string, each
// result discarded) is a foreign parser of unknown signature; declared here only as consuming
// one string argument by EAX pass-through, matching this module's established chained-call
// pattern.
// UNSURE: DAT_006b5e74 (the scratch buffer multiplayer_game_variant_description_generate
// writes into) and DAT_006651f8 (referenced but never shown at a call site) are not documented;
// out/phase2/networking/00.md shows server_browser_open zeroing both DAT_006b5e58 and
// DAT_006b5e74 as plain scalars, so they are declared here as opaque byte buffers rather than a
// named struct.
// UNSURE: `local_800`/`local_1000` are two adjacent stack buffers (2044 and 2048 bytes) whose
// individual purposes (likely a title/description buffer pair) were not resolved.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <wchar.h>


extern int32_t SBServerGetIntValue(void *entry, const char *key, int32_t default_value); // foreign, GameSpy int accessor // foreign, GameSpy library, int accessor
extern char *SBServerGetStringValue(void *entry, const char *key, const char *default_value); // foreign, GameSpy library, string accessor
// blam-cc: EAX -> dest, EDI -> dest capacity in BYTES, EBX -> ASCII source.
// Widens an ASCII string into dest and returns dest, or NULL when it does not fit.
extern wchar_t *string_convert_ascii_to_unicode(wchar_t *dest, int32_t dest_bytes, const char *source); // 0x557990
extern void multiplayer_game_variant_description_generate(ticker_text_buffer *ticker, int32_t fraglimit,
                                                            wchar_t *game_flags_wide, wchar_t *player_flags_wide); // 0x4b8da0,
    // owned by types/game.h, not rewritten here; its first cdecl argument is 0x006b5e74 itself
    // (push 0x6b5e74 at 0x4b7588) and the variant name string rides in EDX (mov edx,ebp)
extern ticker_text_buffer server_browser_variant_ticker; // 0x006b5e74 (EDI at 0x4b74f7)
extern void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self); // 0x4b8a60, this module

// blam-cc: GameSpy entry pointer in ECX (in_ECX)
int32_t server_browser_selected_variant_description_build(void *entry)
{
    char *variant_name;
    int32_t fraglimit;
    char *game_flags;
    char *player_flags;
    wchar_t game_flags_wide[0x400];   // the 0x800-byte stack scratch at [esp+0x814]
    wchar_t player_flags_wide[0x400]; // the 0x800-byte stack scratch at [esp+0x14]

    ticker_text_buffer_append(0, 1, &server_browser_variant_ticker);
    if (entry != 0) {
        variant_name = SBServerGetStringValue(entry, "gamevariant", "");
        fraglimit = SBServerGetIntValue(entry, "fraglimit", 0);
        game_flags = SBServerGetStringValue(entry, "game_flags", "");
        player_flags = SBServerGetStringValue(entry, "player_flags", "");
        if (variant_name != 0 && fraglimit != 0) {
            // FIXED in the review pass: 0x4b755a/0x4b756b set EAX to the two stack scratch
            // buffers and EDI to 0x800 before each call; an earlier draft passed only the
            // source string, and passed the ticker as an opaque byte array.
            string_convert_ascii_to_unicode(game_flags_wide, 0x800, game_flags);
            string_convert_ascii_to_unicode(player_flags_wide, 0x800, player_flags);
            multiplayer_game_variant_description_generate(&server_browser_variant_ticker,
                fraglimit, game_flags_wide, player_flags_wide);
        }
        return 1;
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
