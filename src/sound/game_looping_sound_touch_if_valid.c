// game_looping_sound_touch_if_valid  (Ghidra: FUN_00544290)
// address 0x544290, size 160 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Checks whether a looping-sound datum's owning
//   object is still valid and its cluster/zone permits the sound, stopping it if not." (the
//   auto-summary's "stopping if not" is superseded below: the only call this function makes is
//   to sound_looping_datum_touch, whose own summary is "stamps it with the current double-buffer
//   flag" -- i.e. a keep-alive, not a stop); reads game_looping_sound.last_update (0x14),
//   game_sound_globals.update_count (0x007461a4), function_index (0x18) and flags bits 0x01/0x02
//   (types/sound.h); object.function_valid_flags (0x123, types/objects.h) for the function-value
//   liveness test.
// register convention: game_looping_sound datum handle in ESI (unaff_ESI).
// blam-cc: ESI -> looping_sound_index
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "sound.h"

extern data_array *game_looping_sound_data;         // 0x007461a0
extern game_sound_globals *game_sound_globals_ptr;  // 0x007461a4
extern data_array *object_data;                     // 0x008603b0

extern void sound_looping_datum_touch(int32_t owner); // 0x549f50

// blam-cc: ESI -> looping_sound_index
// Re-stamps a game_looping_sound datum as still alive (via sound_looping_datum_touch) when either
// its bound object function output is valid (or no function is bound / script gain is not being
// stopped), or the datum has gone stale since the last full update pass while not already in the
// loop-stopping state.
void game_looping_sound_touch_if_valid(datum_index looping_sound_index)
{
    game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)looping_sound_index];
    uint8_t stale = (self->last_update == -1) || (self->last_update == game_sound_globals_ptr->update_count - 1);
    uint8_t function_live;

    if ((self->flags & _game_looping_sound_script_gain_bit) == 0) {
        if (self->function_index == -1) {
            function_live = 1;
        } else {
            object_header *header = &((object_header *)object_data->data)[self->object_index & 0xffff];
            object *obj = (object *)header->data;
            function_live = (obj->function_valid_flags & (1 << (self->function_index & 0x1f))) != 0;
        }
    } else {
        function_live = (~(self->flags >> 1)) & 1;
    }

    if (function_live != 0 || (self->state != _game_looping_sound_stopped && stale)) {
        sound_looping_datum_touch((int32_t)looping_sound_index);
    }
}

#if 0
Original Ghidra decompilation (0x544290):

void FUN_00544290(void)

{
  int iVar1;
  bool bVar2;
  int iVar3;
  uint unaff_ESI;
  bool bVar4;

  iVar3 = (unaff_ESI & 0xffff) * 0x34;
  iVar1 = *(int *)(iVar3 + 0x14 + *(int *)(DAT_007461a0 + 0x34));
  iVar3 = iVar3 + *(int *)(DAT_007461a0 + 0x34);
  if ((iVar1 == -1) || (bVar2 = false, iVar1 == *DAT_007461a4 + -1)) {
    bVar2 = true;
  }
  if ((*(uint *)(iVar3 + 4) & 1) == 0) {
    if (*(short *)(iVar3 + 0x18) == -1) {
      bVar4 = true;
    }
    else {
      bVar4 = (*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                 (*(uint *)(iVar3 + 0x10) & 0xffff) * 0xc) + 0x123) &
              (byte)(1 << ((byte)*(short *)(iVar3 + 0x18) & 0x1f))) != 0;
    }
  }
  else {
    bVar4 = (bool)(~(byte)(*(uint *)(iVar3 + 4) >> 1) & 1);
  }
  if ((bVar4 != false) || ((*(short *)(iVar3 + 2) != 2 && (bVar2)))) {
    FUN_00549f50();
  }
  return;
}

Disassembly (0x544290..0x544330, capstone; phase-4 review):

0x544290: push ecx
0x544291: mov ecx, dword ptr [0x7461a0]
0x544297: mov eax, esi
0x544299: and eax, 0xffff
0x54429e: imul eax, eax, 0x34
0x5442a1: push edi
0x5442a2: mov edi, dword ptr [ecx + 0x34]
0x5442a5: mov ecx, dword ptr [eax + edi + 0x14]
0x5442a9: add eax, edi
0x5442ab: cmp ecx, -1
0x5442ae: je 0x5442c2
0x5442b0: mov edx, dword ptr [0x7461a4]
0x5442b6: mov edx, dword ptr [edx]
0x5442b8: dec edx
0x5442b9: cmp ecx, edx
0x5442bb: mov byte ptr [esp + 7], 0
0x5442c0: jne 0x5442c7
0x5442c2: mov byte ptr [esp + 7], 1
0x5442c7: mov ecx, dword ptr [eax + 4]
0x5442ca: test cl, 1
0x5442cd: jne 0x54430a
0x5442cf: mov cx, word ptr [eax + 0x18]
0x5442d3: cmp cx, -1
0x5442d7: jne 0x5442dd
0x5442d9: mov cl, 1
0x5442db: jmp 0x544311
0x5442dd: mov edx, dword ptr [eax + 0x10]
0x5442e0: mov edi, dword ptr [0x8603b0]
0x5442e6: mov edi, dword ptr [edi + 0x34]
0x5442e9: push ebx
0x5442ea: and edx, 0xffff
0x5442f0: mov ebx, 1
0x5442f5: shl ebx, cl
0x5442f7: lea edx, [edx + edx*2]
0x5442fa: mov edx, dword ptr [edi + edx*4 + 8]
0x5442fe: test byte ptr [edx + 0x123], bl
0x544304: setne cl
0x544307: pop ebx
0x544308: jmp 0x544311
0x54430a: shr ecx, 1
0x54430c: not cl
0x54430e: and cl, 1
0x544311: test cl, cl
0x544313: pop edi
0x544314: jne 0x544325
0x544316: cmp word ptr [eax + 2], 2
0x54431b: je 0x54432e
0x54431d: mov al, byte ptr [esp + 3]
0x544321: test al, al
0x544323: je 0x54432e
0x544325: push esi
0x544326: call 0x549f50
0x54432b: add esp, 4
0x54432e: pop ecx
0x54432f: ret 
#endif
