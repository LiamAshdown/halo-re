// sound_stop_all  (Ghidra: FUN_0054adb0, still unnamed there; named from out/phase4/sound_types_notes.md
// "0x54adb0" cross-reference under the 0x54e200 misattribution note: "stopping all sounds ...
// is the driver stop_all slot 0x546fa0 and 0x54adb0")
// address 0x54adb0, size 166 bytes
// name confidence: 0.7   rewrite confidence: 0.95
// evidence: out/phase4/sound_functions.md summary "Stops every currently playing sound and resets
// the sound datum table, used when a gain slider transitions across silence or on shutdown.";
// walks sound_data with datum_next (0x4d0630, already established in src/memory), calls
// sound_instance_stop on every live entry, then data_delete_all(looping_sound_data) and
// sound_driver->stop_all() (vtable slot 0x2c, types/sound.h sound_driver.stop_all).
// register convention: void, no parameters.
//
// Ghidra shows the loop as one real call to datum_next() followed by the callee's own body
// re-implemented inline for every later iteration (the compiler inlined the loop-continuation
// call); this rewrite calls the already-established datum_next() every time, which produces
// the identical sequence of handles.
// Phase-4 review (disassembly appended below): data_delete_all gets ESI = looping_sound_data
// (0x00724a50); the draft cleared sound_data a second time.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"
#include "fn_memory.h"

extern uint8_t sound_initialized;               // 0x00725200
extern data_array *sound_data;                   // 0x007252c0, "sounds" 0x200 x 0xb0
extern data_array *looping_sound_data;           // 0x00724a50, "looping sounds" 0x80 x 0xe4
extern uint8_t sound_stopping_all;               // 0x007252b7
extern sound_driver *current_sound_driver;       // 0x00725208, header calls this "sound_driver";
                                                  // renamed here, sound_driver is already the type
extern int32_t ai_communication_quiet_until_tick; // 0x00725204

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module

extern void sound_instance_stop(datum_index sound_handle);            // 0x54b180

// Stops every playing sound and empties the looping sound table: used when a gain slider
// crosses zero (mute), on pause and on reset.
void sound_stop_all(void)
{
    datum_index sound_handle;

    if (sound_initialized) {
        sound_stopping_all = 1;
        sound_handle = datum_next(-1, sound_data);
        while (sound_handle != k_datum_index_none) {
            sound_instance_stop(sound_handle);
            sound_handle = datum_next((int16_t)sound_handle, sound_data);
        }
        data_delete_all(looping_sound_data); // ESI = 0x00724a50 (the sounds were deleted one by one above)
        current_sound_driver->stop_all();
    }
    ai_communication_quiet_until_tick = 0;
    sound_stopping_all = 0;
}

#if 0
Original Ghidra decompilation (0x54adb0):

void FUN_0054adb0(void)

{
  uint uVar1;
  short *psVar2;
  short sVar3;
  int iVar4;

  if (DAT_00725200 != '\0') {
    DAT_007252b7 = 1;
    uVar1 = datum_next();
joined_r0x0054addc:
    if (uVar1 != 0xffffffff) {
      sound_instance_stop(uVar1);
      iVar4 = uVar1 + 1;
      uVar1 = 0xffffffff;
      sVar3 = (short)iVar4;
      if ((-1 < sVar3) && (sVar3 < *(short *)(DAT_007252c0 + 0x2e))) {
        psVar2 = (short *)((int)sVar3 * (int)*(short *)(DAT_007252c0 + 0x22) +
                          *(int *)(DAT_007252c0 + 0x34));
        do {
          if (*psVar2 != 0) {
            uVar1 = (int)*psVar2 << 0x10 | (int)(short)iVar4;
            break;
          }
          iVar4 = iVar4 + 1;
          psVar2 = (short *)((int)psVar2 + (int)*(short *)(DAT_007252c0 + 0x22));
        } while ((short)iVar4 < *(short *)(DAT_007252c0 + 0x2e));
      }
      goto joined_r0x0054addc;
    }
    data_delete_all();
    (**(code **)(DAT_00725208 + 0x2c))();
  }
  DAT_00725204 = 0;
  DAT_007252b7 = 0;
  return;
}

Disassembly (0x54adb0..0x54ae56, capstone; phase-4 review):

0x54adb0: mov al, byte ptr [0x725200]
0x54adb5: push ebx
0x54adb6: xor ebx, ebx
0x54adb8: cmp al, bl
0x54adba: je 0x54ae48
0x54adc0: push esi
0x54adc1: push edi
0x54adc2: mov edi, dword ptr [0x7252c0]
0x54adc8: or edx, 0xffffffff
0x54adcb: mov byte ptr [0x7252b7], 1
0x54add2: call 0x4d0630
0x54add7: mov esi, eax
0x54add9: cmp esi, -1
0x54addc: je 0x54ae32
0x54adde: push ebp
0x54addf: nop 
0x54ade0: push esi
0x54ade1: call 0x54b180
0x54ade6: lea ecx, [esi + 1]
0x54ade9: add esp, 4
0x54adec: or edi, 0xffffffff
0x54adef: cmp cx, bx
0x54adf2: jl 0x54ae2a
0x54adf4: mov esi, dword ptr [0x7252c0]
0x54adfa: mov bp, word ptr [esi + 0x2e]
0x54adfe: cmp cx, bp
0x54ae01: jge 0x54ae2a
0x54ae03: movsx edx, word ptr [esi + 0x22]
0x54ae07: movsx eax, cx
0x54ae0a: imul eax, edx
0x54ae0d: add eax, dword ptr [esi + 0x34]
0x54ae10: cmp word ptr [eax], bx
0x54ae13: jne 0x54ae1f
0x54ae15: inc ecx
0x54ae16: add eax, edx
0x54ae18: cmp cx, bp
0x54ae1b: jl 0x54ae10
0x54ae1d: jmp 0x54ae2a
0x54ae1f: movsx edi, word ptr [eax]
0x54ae22: movsx eax, cx
0x54ae25: shl edi, 0x10
0x54ae28: or edi, eax
0x54ae2a: cmp edi, -1
0x54ae2d: mov esi, edi
0x54ae2f: jne 0x54ade0
0x54ae31: pop ebp
0x54ae32: mov esi, dword ptr [0x724a50]
0x54ae38: call 0x4d0580
0x54ae3d: mov ecx, dword ptr [0x725208]
0x54ae43: call dword ptr [ecx + 0x2c]
0x54ae46: pop edi
0x54ae47: pop esi
0x54ae48: mov dword ptr [0x725204], ebx
0x54ae4e: mov byte ptr [0x7252b7], bl
0x54ae54: pop ebx
0x54ae55: ret 
#endif
