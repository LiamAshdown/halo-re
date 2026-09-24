// cache_file_switch_map_by_path  (Ghidra: FUN_0045aea0; renamed per symbols/review_queue.txt)
// address 0x45aea0, size 260 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: symbols/review_queue.txt 0x45aea0 "extracts the filename from a path with strrchr,
//   looks it up with cache_file_find_slot_by_name, and on a miss/mismatch triggers cache reload
//   helpers and an error".
// register convention: map path in EAX (in_EAX); a caller-supplied "apply the switch now" flag
//   in BL (unaff_BL).
//   // blam-cc: EAX -> path, EBX -> apply_state
//
// UNSURE: every callee here (cache_file_find_slot_by_name, cache_file_open_by_name,
// cache_file_download_matches/_stop/_finish, FUN_004c8900, FUN_0053d080,
// saved_game_last_profile_clear) is called with zero visible arguments in the decompile; none
// are in this batch. Reconstructed to take `path` where that is the obvious operand (the name
// lookups) and left argument-less where no evidence supports a specific signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern uint8_t download_in_progress;   // 0x006ac470
extern char *download_error_path;      // 0x00722bbc
extern uint8_t *cache_file_slot_table; // 0x006b0b80, TYPES-GAP (byte 0=active, byte 3=flag, dword+4=float)
extern uint32_t unknown_00719979; // TYPES-GAP
extern uint32_t unknown_00719774; // TYPES-GAP
extern int32_t unknown_006894b8;  // TYPES-GAP
extern int32_t saved_game_profile_index;    // 0x00714dd4
extern int32_t saved_game_profile_previous; // 0x0068e66c
extern uint8_t unknown_00718e80;  // TYPES-GAP

extern char *strrchr(const char *s, int32_t c); // CRT
extern int16_t cache_file_find_slot_by_name(char *path); // 0x443770, UNSURE args
extern uint8_t cache_file_open_by_name(char *name, uint8_t report_fatal_error); // 0x443360,
    // blam-cc: EAX -> name, stack -> report_fatal_error (matches src/cache/cache_file_open_by_name.c)
extern uint8_t cache_file_download_matches(char *path);   // 0x4432f0, UNSURE args
extern void cache_file_download_stop(void);   // 0x443510
extern void cache_file_download_finish(void); // 0x443540
extern void FUN_004c8900(void); // UNSURE module
extern void FUN_0053d080(void); // UNSURE module
extern void saved_game_last_profile_clear(void); // 0x53d220
extern void shell_display_fatal_error_dialog(uint32_t a, uint32_t b, uint32_t c); // 0x57ea70

// Resolves `path`'s map cache slot and switches or queues loading of it if it is not already the
// active cache file; only actually applies the state changes when `apply_state` is set.
void cache_file_switch_map_by_path(char *path, uint8_t apply_state)
    // blam-cc: EAX -> path, EBX -> apply_state
{
    int32_t slot_table;
    int16_t slot;

    strrchr(path, 0x5c);
    slot = cache_file_find_slot_by_name(path); // UNSURE args
    if (slot == -1) {
        if (download_in_progress == 0) {
        open_by_name:
            if (cache_file_open_by_name(path, apply_state) == 0) { // EBX/BL is forwarded as report_fatal_error
                if (apply_state == 0) {
                    return;
                }
                download_error_path = path;
                shell_display_fatal_error_dialog(0x89, 0x7e, 1);
            }
        } else {
            if (cache_file_download_matches(path) == 0) { // UNSURE args
                if (apply_state == 0) {
                    cache_file_download_stop();
                    FUN_004c8900();
                } else {
                    cache_file_download_finish();
                }
            }
            if (download_in_progress == 0) {
                goto open_by_name;
            }
        }
        slot_table = (int32_t)cache_file_slot_table;
        if (apply_state == 0) {
            return;
        }
        cache_file_slot_table[3] = 0;
        *(uint32_t *)(slot_table + 4) = 0x3f800000;
    }

    if (apply_state != 0) {
        unknown_00719979 = 0;
        unknown_00719774 = 0;
        if (download_in_progress != 0) {
            cache_file_download_finish();
        }
        if (unknown_006894b8 == 1) {
            if (saved_game_profile_previous != saved_game_profile_index) {
                if (saved_game_profile_index != -1) {
                    FUN_0053d080();
                }
                saved_game_profile_previous = saved_game_profile_index;
            }
            if (unknown_00718e80 != 0) {
                saved_game_last_profile_clear();
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x45aea0), from tools/pack.py 0x45aea0:

void FUN_0045aea0(void)

{
  int iVar1;
  char cVar2;
  short sVar3;
  char *in_EAX;
  char unaff_BL;

  _strrchr(in_EAX,0x5c);
  sVar3 = cache_file_find_slot_by_name();
  if (sVar3 != -1) goto LAB_0045af3c;
  if (DAT_006ac470 == '\0') {
LAB_0045aefa:
    cVar2 = cache_file_open_by_name();
    if (cVar2 == '\0') {
      if (unaff_BL == '\0') {
        return;
      }
      DAT_00722bbc = in_EAX;
      shell_display_fatal_error_dialog(0x89,0x7e,1);
    }
  }
  else {
    cVar2 = cache_file_download_matches();
    if (cVar2 == '\0') {
      if (unaff_BL == '\0') {
        cache_file_download_stop();
        FUN_004c8900();
      }
      else {
        cache_file_download_finish();
      }
    }
    if (DAT_006ac470 == '\0') goto LAB_0045aefa;
  }
  iVar1 = DAT_006b0b80;
  if (unaff_BL == '\0') {
    return;
  }
  *(undefined1 *)(DAT_006b0b80 + 3) = 0;
  *(undefined4 *)(iVar1 + 4) = 0x3f800000;
LAB_0045af3c:
  if (unaff_BL != '\0') {
    DAT_00719979 = 0;
    DAT_00719774 = 0;
    if (DAT_006ac470 != '\0') {
      cache_file_download_finish();
    }
    if (DAT_006894b8 == 1) {
      if (DAT_0068e66c != DAT_00714dd4) {
        if (DAT_00714dd4 != -1) {
          FUN_0053d080();
        }
        DAT_0068e66c = DAT_00714dd4;
      }
      if (DAT_00718e80 != '\0') {
        saved_game_last_profile_clear();
      }
    }
  }
  return;
}
#endif
