// weapon_get_zoom_fov_resolved  (Ghidra: FUN_0046fe70; renamed, no established name)
// address 0x46fe70, size 136 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump 0x46fe70..0x46fef7)
// evidence: out/phase4/game_functions.md ("Resolves and validates a zoom-table index before
// delegating to the zoom FOV lookup, applying a bitmask/substitution check first");
// types/game.h current_game_engine (0x006f1d20), team_pair_globals (0x006b0b84, enemy_bits
// +0xa4); weapon_get_zoom_fov.c (this batch, 0x46fe10), which this function always tails into.
//
// Fully reconstructed from
//   objdump -d -M intel --start-address=0x46fe70 --stop-address=0x46fef8 bin/halo.exe
// because Ghidra's decompile packs three different register-passed inputs into two guessed
// variables (`in_AX`, `in_ECX`) and one bogus CONCAT22 expression that does not correspond to any
// real value the disassembly computes; see the block below for the transcription this is based on.
// register convention: ECX carries the zoom-table index (`mov edi,ecx` is the first instruction);
// AX carries a second, 0..9-range index used only by the single-player substitution check.
//   // blam-cc: ECX -> zoom_table_index, AX -> substitution_check_index
// UNSURE: DAT_006b0b80 (dereferenced, +0xe read as int16) is not attested in any header; modeled
// here as a raw untyped pointer. UNSURE: the enemy_bits row this reads is fixed at team 1
// (index (substitution_check_index + 10) always lands in team_pair_globals::enemy_bits[0] for the
// 0..9 range this function accepts, i.e. it asks "is team `substitution_check_index` an enemy of
// team 1" using the exact same bitmap game.h documents for team relationships) -- what that has to
// do with a weapon zoom substitution is not recoverable from this function alone; it is
// transcribed literally rather than renamed to something more specific.

// FIXED (objdump + in game): the original returns the callee's float on the x87 stack; every caller
//   consumes it (fmul / fstp st(0) right after the call at 0x40da4a, 0x40dcf8, 0x40eae8, 0x40ebbf). The
//   draft returned void, so callers scaled AI values by FPU junk (in game: NPCs teleporting).
//   NAMING NOTE: the table at Globals+0x11c holds four floats per row, one per difficulty level, and
//   0x6b0b80+0xe is the current difficulty; this is a difficulty-scaled AI parameter lookup (ECX = the
//   parameter row, AX = a team whose enemy bit swaps the row via 0x657470), not a weapon zoom.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern team_pair_globals *team_pair_data;         // 0x006b0b84
extern void *main_game_globals;                        // 0x006b0b80, UNSURE: see header
extern int16_t weapon_zoom_index_substitutions[];     // 0x00657470, indexed by zoom_table_index

extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification); // 0x46fe10, this batch

// blam-cc: ECX -> zoom_table_index, AX -> substitution_check_index
// In multiplayer (current_game_engine != NULL), forces magnification to 1 and looks up
// weapon_get_zoom_fov(zoom_table_index, 1) directly. Single-player: if
// substitution_check_index is in 0..9 and team_pair_globals marks team `substitution_check_index`
// as an enemy of team 1, first substitutes zoom_table_index through
// weapon_zoom_index_substitutions (falling back to the multiplayer-style call if the substitute is
// -1); either way, the magnification used is *(int16 *)(main_game_globals + 0xe).
real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index)
{
    int16_t magnification = *(int16_t *)((uint8_t *)main_game_globals + 0xe);

    if (current_game_engine != 0) {
        return weapon_get_zoom_fov(zoom_table_index, 1);
    }

    if (substitution_check_index >= 0 && substitution_check_index < 10) {
        int32_t bit_index = substitution_check_index + 10;
        uint32_t bit = 1u << (bit_index & 0x1f);
        uint8_t is_enemy = (team_pair_data->enemy_bits[bit_index >> 5] & bit) != 0;

        if (is_enemy) {
            int16_t substitute = weapon_zoom_index_substitutions[(uint16_t)zoom_table_index];

            if (substitute == -1) {
                return weapon_get_zoom_fov(zoom_table_index, 1);
            }
            zoom_table_index = substitute;
        }
    }
    return weapon_get_zoom_fov(zoom_table_index, magnification);
}

#if 0
Original Ghidra decompilation (0x46fe70), from tools/pack.py 0x46fe70 -- see the header comment;
the CONCAT22 expression and both in_* variables do not correspond 1:1 to the real registers.

void FUN_0046fe70(void)

{
  short in_AX;
  int iVar1;
  undefined4 in_ECX;
  short sVar2;

  if ((DAT_006f1d20 == 0) &&
     ((((in_AX < 0 || (9 < in_AX)) ||
       (iVar1 = in_AX + 10,
       (1 << ((byte)iVar1 & 0x1f) & *(uint *)(DAT_006b0b84 + 0xa4 + (iVar1 >> 5) * 4)) == 0)) ||
      (sVar2 = (short)in_ECX,
      in_ECX = CONCAT22((short)(iVar1 >> 0x15),*(short *)(&DAT_00657470 + sVar2 * 2)),
      *(short *)(&DAT_00657470 + sVar2 * 2) != -1)))) {
    FUN_0046fe10(in_ECX);
    return;
  }
  FUN_0046fe10();
  return;
}

Reconstruction from objdump -d -M intel --start-address=0x46fe70 --stop-address=0x46fef8
bin/halo.exe:

  46fe70: push ecx ; push esi ; push edi
  46fe73: mov edi,ecx                     ; edi = zoom_table_index (ECX arg)
  46fe75: mov ecx,[0x6b0b80]
  46fe7b: mov si,[ecx+0xe]                ; si = magnification (kept for the fallback call)
  46fe7f: mov ecx,[0x6f1d20] ; test ecx,ecx ; je 0x46fe9d
  46fe89: mov esi,1 ; push edi ; mov ecx,esi ; call 0x46fe10   ; weapon_get_zoom_fov(edi, 1)
  46fe96..46fe9c: epilogue, ret
  46fe9d: test ax,ax ; jl 0x46fee9                              ; substitution_check_index < 0
  46fea2: cmp ax,0xa ; jge 0x46fee9                              ; >= 10
  46fea8: movsx eax,ax ; add eax,0xa                             ; eax = index + 10
  46feae: mov ecx,eax ; and ecx,0x1f ; mov edx,1 ; shl edx,cl     ; edx = 1 << ((index+10)&0x1f)
  46feba: mov ecx,[0x6b0b84] ; sar eax,5                          ; eax = (index+10) >> 5
  46fec3: and edx,[ecx+eax*4+0xa4]                                ; edx &= enemy_bits[eax]
  46feca: neg edx ; sbb dl,dl ; inc dl                             ; dl = (edx==0) ? 1 : 0
  46fed0: mov [esp+0xb],dl ; jne 0x46fee9                          ; dl==1 (not an enemy) -> skip
  46fed6: movsx edx,di ; mov ax,[edx*2+0x657470]                   ; ax = substitutes[zoom_table_index]
  46fee1: cmp ax,0xffff ; je 0x46fe89                              ; -1 -> take the magnification=1 path
  46fee7: mov edi,eax                                              ; edi = substituted index
  46fee9: push edi ; mov ecx,esi ; call 0x46fe10                   ; weapon_get_zoom_fov(edi, esi)
  46fef4..46fef7: epilogue, ret
#endif
