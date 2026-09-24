// game_engine_player_profile_cache_sync_all  (Ghidra: FUN_00466cb0; named per
// out/phase4/game_functions.md: "Processes every active entry in the player-profile cache and
// then fires an optional game-engine callback.")
// address 0x466cb0, size 73 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x466cb0
//   --stop-address=0x466cf9), which shows two real parameters Ghidra's decompile drops
//   entirely: EBX (never assigned in this function, i.e. an incoming register argument) is
//   forwarded as game_engine_capture_player_profile's `commit` for every active slot AND as the
//   first argument to the closing callback; a stack argument (read via [esp+0x4] only after
//   this function's own esi/edi are popped back off, i.e. this function's own first stack
//   parameter) is forwarded as the callback's second argument. types/game.h player_profile_cache
//   (in_use +0x00) and game_engine_definition::profiles_updated (+0x90).
// register convention: EBX -> commit; a second, stack-passed argument forwarded only to the
//   callback.
//   // blam-cc: EBX -> commit, stack -> callback_extra_arg
// UNSURE: profiles_updated's real signature/parameter types; `callback_extra_arg`'s identity.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern player_profile player_profile_cache[16];     // 0x006b0b88
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern void game_engine_capture_player_profile(int32_t slot, int32_t commit); // 0x466ee0, this batch

// blam-cc: EBX -> commit, stack -> callback_extra_arg
// Re-captures every active player-profile-cache entry with the given `commit` flag, then
// invokes the active game engine's optional profiles_updated callback (commit, callback_extra_
// arg), if one is registered. UNSURE: see header for the callback's real signature.
void game_engine_player_profile_cache_sync_all(int32_t commit, void *callback_extra_arg)
{
    int32_t i;
    void (*callback)(int32_t, void *);

    for (i = 0; i < 16; i = i + 1) {
        if (player_profile_cache[i].in_use == 1) {
            game_engine_capture_player_profile(i, commit);
        }
    }

    callback = (void (*)(int32_t, void *))current_game_engine->profiles_updated;
    if (callback != (void (*)(int32_t, void *))0) {
        callback(commit, callback_extra_arg);
    }
}

#if 0
Original Ghidra decompilation (0x466cb0), from tools/pack.py 0x466cb0:

void FUN_00466cb0(void)

{
  char *pcVar1;

  pcVar1 = (char *)&DAT_006b0b88;
  do {
    if (*pcVar1 == '\x01') {
      FUN_00466ee0();
    }
    pcVar1 = pcVar1 + 0x30;
  } while ((int)pcVar1 < 0x6b0e88);
  if (*(code **)(DAT_006f1d20 + 0x90) != (code *)0x0) {
    (**(code **)(DAT_006f1d20 + 0x90))();
  }
  return;
}

Raw disassembly (objdump -d -M intel --start-address=0x466cb0 --stop-address=0x466cf9):

00466cb0:  push esi
00466cb1:  push edi
00466cb2:  xor edi,edi
00466cb4:  mov esi,0x6b0b88
00466cc0:  cmp byte ptr [esi],0x1
00466cc3:  jne 0x466cd0
00466cc5:  push ebx
00466cc6:  mov eax,edi
00466cc8:  call 0x466ee0             ; game_engine_capture_player_profile(EAX=slot, stack=EBX)
00466ccd:  add esp,0x4
00466cd0:  add esi,0x30
00466cd3:  inc edi
00466cd4:  cmp esi,0x6b0e88
00466cda:  jl 0x466cc0
00466cdc:  mov eax,ds:0x6f1d20
00466ce1:  mov eax,[eax+0x90]
00466ce7:  test eax,eax
00466ce9:  pop edi
00466cea:  pop esi
00466ceb:  je 0x466cf8
00466ced:  mov ecx,[esp+0x4]         ; this function's own first stack parameter
00466cf1:  push ecx
00466cf2:  push ebx
00466cf3:  call eax                  ; callback(EBX=commit, stack=ecx)
00466cf5:  add esp,0x8
00466cf8:  ret
#endif
