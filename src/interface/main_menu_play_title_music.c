// main_menu_play_title_music  (Ghidra: main_menu_play_title_music, already named)
// address 0x4993e0, size 66 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: matches the given name exactly -- looks up the "sound\music\title1\title1" looping
// sound tag (group 'lsnd', register EDI, per tag_lookup's real signature in src/cache/
// tag_lookup.c) and starts it playing at full gain, latching a "started" flag so it only ever
// fires once. Ghidra's own decompile drops both the tag group (EDI) and the found tag index
// (EAX, the callee's in_EAX) from the two calls; both were recovered from the disassembly
// (0x4993f3..0x499417), which shows no intervening register write between the tag_lookup return
// and the sound_looping_start call, so the found tag index flows straight into it.
// register convention: no register-passed arguments.
// UNSURE: sound_looping_start (0x544090, sound module) is not otherwise named in this session; declared
// as extern with its best-recovered signature -- (in_EAX) sound tag index, param_1 an object
// index (-1 here, not attached to any object), param_2 a gain (1.0 here).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern uint8_t main_menu_music_pending; // 0x00718fc6
extern int32_t main_menu_music_datum;   // 0x00719748, UNSURE: named by inference, nonzero blocks a restart

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void sound_looping_start(datum_index sound_tag, int32_t object_index, float gain); // 0x544090, sound module, UNSURE signature
// blam-cc: EAX -> sound_tag, object_index/gain ordinary cdecl stack parameters

// Starts the main menu's title theme music if it is not already playing.
void main_menu_play_title_music(void)
{
    datum_index sound_tag;

    if (main_menu_music_pending == 0 && main_menu_music_datum == 0) {
        sound_tag = tag_lookup(0x6c736e64 /* 'lsnd' */, "sound\\music\\title1\\title1");
        if (sound_tag != (datum_index)-1) {
            sound_looping_start(sound_tag, -1, 1.0f);
            main_menu_music_pending = 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4993e0):

void main_menu_play_title_music(void)

{
  int iVar1;

  if ((DAT_00718fc6 == '\0') && (DAT_00719748 == 0)) {
    iVar1 = tag_lookup("sound\\music\\title1\\title1");
    if (iVar1 != -1) {
      FUN_00544090(0xffffffff,0x3f800000);
      DAT_00718fc6 = '\x01';
    }
  }
  return;
}

Disassembly confirming the group tag (EDI) and the dropped EAX argument to FUN_00544090:

  4993e9: mov    eax,ds:0x719748
  4993f2: push   edi
  4993f3: push   0x66a044                 ; "sound\music\title1\title1"
  4993f8: mov    edi,0x6c736e64           ; 'lsnd'
  4993fd: call   0x442550                 ; tag_lookup(group='lsnd', path)
  499405: cmp    eax,0xffffffff
  499409: je     0x499421
  49940b: push   0x3f800000               ; gain 1.0
  499410: push   0xffffffff               ; object index -1
  499412: call   0x544090                 ; FUN_00544090(eax=sound_tag, -1, 1.0)
#endif
