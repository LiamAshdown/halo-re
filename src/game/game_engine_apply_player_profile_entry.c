// game_engine_apply_player_profile_entry  (Ghidra: FUN_00466d00; named per
// out/phase4/game_functions.md: "Applies a cached player-profile entry's fields onto a
// newly-created datum, with a King-variant-specific time-field rescale.")
// address 0x466d00, size 344 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x466d00
//   --stop-address=0x466e60), which corrects two things Ghidra's decompile gets wrong and fills
//   in two calls it drops all arguments for:
//     - the branch test is `**(int **)event == 0`, not `*(int *)*event == 0` -- the real code
//       dereferences one level deeper (ecx = [edi], edx = [ecx]) than Ghidra shows.
//     - game_engine_player_profile_cache_find (0x466e80, this batch) is called with ESI built
//       from `*(*(int ***)event)[0x44/4] ...`: eax = [[event+0x44]], and if that is nonzero,
//       ESI = a lookup through the global table at 0x00687558 (`*(int*)(table+0x28))[eax]`)
//       -- otherwise ESI stays -1. This lookup table is not attributed to any module here.
//     - the `datum_get` call resolves EDX = player_profile_cache[slot].player (its stored
//       handle) against ESI = player_data (0x0087a480), exactly matching datum_get's own
//       (handle, array) register convention (src/memory/datum_get.c).
//     - message_delta_decode_compound_field is called (EAX = event, ECX = &cache[slot].kills); FUN_004ec600 is called
//       (EAX = event, EDX = &cache[slot].kills, ECX = a local stack scratch address one dword
//       into the same buffer the swap below fills, stack: 0). Both are outside this batch and
//       their real purpose is unresolved; the local-buffer offset-by-one-dword relationship
//       between the write and the read-back is reproduced literally without a guess at why.
// register convention: `event` is this function's own recognized stack parameter.
//   // blam-cc: stack -> event
// UNSURE: the shape of `event` beyond one dereference chain to +0x44; the 0x00687558 lookup
//   table; and message_delta_decode_compound_field/FUN_004ec600/message_delta_decode_compound_field_staged's real signatures and behavior (kept as
//   raw externs with the register wiring the disassembly shows, nothing more).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern player_profile player_profile_cache[16]; // 0x006b0b88
extern game_variant game_engine_variant;        // 0x006f1c88
extern data_array *player_data;                 // 0x0087a480

extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX, ESI

extern int32_t game_engine_player_profile_cache_find(datum_index player_handle); // 0x466e80, blam-cc: ESI

extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590, blam-cc:
    // EAX -> event, ECX -> out_values; UNSURE identity (a network message-delta decode)
extern uint8_t message_delta_decode_compound_field_forced(void *event, uint32_t *cache_tail, uint32_t *scratch, int32_t zero);
    // 0x4ec600, blam-cc: EAX, EDX, ECX, stack -> zero; UNSURE
extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670, blam-cc: EAX -> event; UNSURE identity

extern int32_t *machine_table; // 0x00687558+0x28, UNSURE: owning module/identity

// UNSURE: applies a found player-profile-cache entry's stat tail onto a player datum. `event`'s
// shape beyond one `[[event+0x44]]` dereference, and the two foreign FUN_004ec5xx/4ec6xx calls'
// real purpose, are unresolved -- see header.
void game_engine_apply_player_profile_entry(void *event)
{
    int32_t lookup_index;
    datum_index search_handle;
    int32_t slot;
    player_profile *profile;
    uint32_t *tail; // &profile->kills, the 10-dword tail this function copies in/out
    uint8_t committed;
    player *p;

    lookup_index = **(int32_t **)((uint8_t *)event + 0x44);
    search_handle = (datum_index)0xffffffff;
    if (lookup_index != 0) {
        search_handle = (datum_index)machine_table[lookup_index];
    }

    slot = game_engine_player_profile_cache_find(search_handle);
    if (slot == -1) {
        message_delta_decode_compound_field_staged(event);
        return;
    }

    profile = &player_profile_cache[slot];
    tail = (uint32_t *)&profile->kills; // profile+0x08, 10 dwords to the struct's end

    if (**(int32_t **)event == 0) {
        committed = message_delta_decode_compound_field(event, tail);
    } else {
        uint32_t scratch[11];
        uint32_t i;
        for (i = 0; i < 10; i = i + 1) {
            scratch[i] = tail[i];
        }
        committed = message_delta_decode_compound_field_forced(event, tail, scratch + 1, 0);
        for (i = 0; i < 10; i = i + 1) {
            tail[i] = scratch[i + 1];
        }
    }

    if (committed != 1) {
        return;
    }

    p = (player *)datum_get(profile->player, player_data);
    if (p == (player *)0) {
        return;
    }

    p->kills = profile->kills;
    p->unknown_9e = profile->unknown_0a;
    p->unknown_a0 = profile->unknown_0c;
    p->assists = profile->assists;
    p->unknown_a6 = profile->unknown_12;
    p->unknown_a8 = profile->unknown_14;
    p->betrayals = profile->betrayals;
    p->deaths = profile->deaths;
    p->suicides = profile->suicides;
    p->objective_time = profile->objective_time;
    p->objective_score = profile->unknown_22;
    p->slayer_target = profile->unknown_24;
    p->odd_man_out = profile->odd_man_out;
    p->speed = profile->speed;

    if (game_engine_variant.game_engine_index == _game_engine_king) {
        *(int16_t *)&p->objective_time = (int16_t)profile->objective_time * 0x1e;
    }
}

#if 0
Original Ghidra decompilation (0x466d00), from tools/pack.py 0x466d00:

void FUN_00466d00(undefined4 *param_1)

{
  undefined4 *puVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 local_28 [10];

  iVar3 = FUN_00466e80();
  if (iVar3 != -1) {
    iVar3 = iVar3 * 0x30;
    puVar1 = (undefined4 *)(&DAT_006b0b90 + iVar3);
    if (*(int *)*param_1 == 0) {
      cVar2 = FUN_004ec590();
    }
    else {
      puVar5 = puVar1;
      puVar6 = local_28;
      for (iVar4 = 10; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      cVar2 = FUN_004ec600(0);
      puVar5 = local_28;
      puVar6 = puVar1;
      for (iVar4 = 10; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
    }
    if ((cVar2 == '\x01') && (iVar4 = datum_get(), iVar4 != 0)) {
      *(undefined4 *)(iVar4 + 0x9c) = *puVar1;
      *(undefined4 *)(iVar4 + 0xa0) = *(undefined4 *)(&DAT_006b0b94 + iVar3);
      *(undefined4 *)(iVar4 + 0xa4) = *(undefined4 *)(&DAT_006b0b98 + iVar3);
      *(undefined4 *)(iVar4 + 0xa8) = *(undefined4 *)(&DAT_006b0b9c + iVar3);
      *(undefined2 *)(iVar4 + 0xac) = *(undefined2 *)(&DAT_006b0ba0 + iVar3);
      *(undefined2 *)(iVar4 + 0xae) = *(undefined2 *)(&DAT_006b0ba2 + iVar3);
      *(undefined2 *)(iVar4 + 0xb0) = *(undefined2 *)(&DAT_006b0ba4 + iVar3);
      *(undefined4 *)(iVar4 + 0xc4) = *(undefined4 *)(&DAT_006b0ba6 + iVar3);
      *(undefined2 *)(iVar4 + 200) = *(undefined2 *)(&DAT_006b0baa + iVar3);
      *(undefined4 *)(iVar4 + 0x88) = *(undefined4 *)(&DAT_006b0bac + iVar3);
      *(undefined *)(iVar4 + 0x8c) = (&DAT_006b0bb0)[iVar3];
      *(undefined4 *)(iVar4 + 0x6c) = *(undefined4 *)(&DAT_006b0bb4 + iVar3);
      if (DAT_006f1cb8 == 4) {
        *(short *)(iVar4 + 0xc4) = *(short *)(&DAT_006b0ba6 + iVar3) * 0x1e;
      }
    }
    return;
  }
  FUN_004ec670();
  return;
}

Raw disassembly (objdump -d -M intel --start-address=0x466d00 --stop-address=0x466e60), the
part this rewrite relied on for the two corrections and the two calls argument registers:

00466d00:  sub esp,0x28
00466d03:  push esi
00466d04:  push edi
00466d05:  mov edi,[esp+0x34]        ; edi = event (the only stack parameter)
00466d09:  mov eax,[edi+0x44]
00466d0c:  mov eax,[eax]             ; eax = [[event+0x44]]
00466d0e:  or esi,0xffffffff         ; esi = -1 (default search handle)
00466d11:  test eax,eax
00466d13:  je 0x466d21
00466d15:  mov ecx,ds:0x687558
00466d1b:  mov edx,[ecx+0x28]
00466d1e:  mov esi,[edx+eax*4]       ; esi = table[eax]
00466d21:  call 0x466e80             ; game_engine_player_profile_cache_find(ESI)
00466d26:  cmp eax,0xffffffff
00466d29:  je 0x466e4b
00466d2f:  mov ecx,[edi]
00466d31:  mov edx,[ecx]             ; edx = **event
00466d34:  lea ebx,[eax+eax*2]
00466d37:  shl ebx,0x4               ; ebx = slot * 0x30
00466d3a:  test edx,edx
00466d3d:  lea ebp,[ebx+0x6b0b90]    ; ebp = &cache[slot].kills
00466d43:  jne 0x466d50
00466d45:  mov eax,edi
00466d47:  mov ecx,ebp
00466d49:  call 0x4ec590             ; FUN_004ec590(EAX=event, ECX=&cache[slot].kills)
00466d4e:  jmp 0x466d7e
00466d50:  mov eax,[esp+0x3c]        ; eax = event, reloaded (same slot [esp+0x34] before the
                                     ; two extra pushes below shift it to +0x3c)
00466d54:  mov ecx,0xa
00466d59:  mov esi,ebp
00466d5b:  lea edi,[esp+0x10]
00466d5f:  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]   ; scratch[0..9] = cache tail
00466d61:  push 0x0
00466d63:  mov edx,ebp
00466d65:  lea ecx,[esp+0x14]        ; one dword INTO the scratch buffer
00466d69:  call 0x4ec600             ; FUN_004ec600(EAX=event, EDX=&cache[slot].kills,
                                     ;              ECX=&scratch[1], stack=0)
00466d6e:  mov ecx,0xa
00466d73:  lea esi,[esp+0x14]        ; read back starting one dword IN, not from [0]
00466d77:  mov edi,ebp
00466d7c:  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]   ; cache tail = scratch[1..10]
00466d7e:  cmp al,0x1
00466d80:  jne 0x466e43
00466d86:  mov edx,[ebx+0x6b0b8c]    ; edx = cache[slot].player
00466d8c:  mov esi,ds:0x87a480      ; esi = player_data
00466d92:  call 0x4d0680             ; datum_get(EDX=handle, ESI=array)
00466d97:  test eax,eax
00466d99:  je 0x466e43
... (field copies as in the Ghidra listing above, all confirmed against ebx/ebp-relative
     addressing matching player_profile_cache[slot] fields one for one)
00466e30:  cmp DWORD PTR ds:0x6f1cb8,0x4
00466e37:  jne 0x466e43
00466e39:  mov ax,[esi]              ; esi still == &cache[slot].objective_time here
00466e3c:  imul ax,ax,0x1e
00466e40:  mov [edi],ax              ; edi still == &player.objective_time here
00466e43:  ret (epilogue)
00466e4b:  mov eax,edi
00466e4d:  call 0x4ec670             ; FUN_004ec670(EAX=event)
#endif
