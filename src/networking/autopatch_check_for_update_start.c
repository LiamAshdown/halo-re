// autopatch_check_for_update_start  (Ghidra: autopatch_check_for_update_start, already named)
// address 0x577240, size 205 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: string "currentupdate.cfg"; shares the file_reference reset preamble (0x43 dwords
// zeroed, then "file" + 0xffff stamped into the head, matching types/hs.h's own comment on
// file_reference) with the savegame_index_* family in src/game; path_append_component and
// path_remove_last_component are the same two helpers that family already pins.
// register convention: no register-passed arguments.
// UNSURE: the exact path this file_reference resolves to (an implicit global path component,
// like saved_game_root_path is for the savegame family, not independently identified here).

// FIXED (objdump): path_append_component takes (destination = the file reference's path buffer at +8, in ESI;
//   component, in EBX); the draft passed them swapped, and read the component array as a pointer.
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t autopatch_update_check_state; // 0x0069fe04, -1 not started, 0 failed, 1 running

extern uint8_t *autopatch_update_cfg_directory; // UNSURE: implicit path component this resolves against
extern int32_t security_check_write_access(void); // 0x542840, this module
extern char file_reference_exists(file_reference *reference); // 0x555720
extern uint8_t file_reference_delete(void); // 0x555670, UNSURE args elided
extern void path_append_component(char *destination, const char *component); // 0x555ec0
extern void path_remove_last_component(uint8_t *path); // 0x555f80
extern uint32_t autopatch_version_check_request(void); // 0x5771e0, this module

// One-time entry point that kicks off the background thread which checks bungie.net for a game
// update: if the update-config directory is writable, deletes any stale "currentupdate.cfg"
// first, then starts the version-check thread.
int32_t autopatch_check_for_update_start(void)
{
    if (autopatch_update_check_state == -1) {
        file_reference reference;
        uint8_t *flags = (uint8_t *)&reference + 8;
        void *thread;
        uint32_t thread_id;

        if (security_check_write_access() != 0) {
            uint32_t *raw = (uint32_t *)&reference;
            int32_t i;
            for (i = 0; i < 0x43; i++) {
                raw[i] = 0;
            }
            *(int32_t *)((uint8_t *)&reference + 4) = 0x66696c6f; // "ofil" (little-endian "file")
            *(uint16_t *)((uint8_t *)&reference + 6) = 0xffff;
            if ((*flags & 1) != 0) {
                path_remove_last_component((uint8_t *)&reference + 8);
            }
            path_append_component((char *)&reference + 8, "currentupdate.cfg"); // 0x67219c
            *flags = *flags | 1;
            if (file_reference_exists(&reference) != 0) {
                file_reference_delete();
            }
        }

        thread = CreateThread(0, 0x10400, (LPTHREAD_START_ROUTINE)autopatch_version_check_request, 0, 0, (LPDWORD)&thread_id);
        if (thread != (void *)0xffffffff) {
            autopatch_update_check_state = 1;
            CloseHandle(thread);
            return autopatch_update_check_state;
        }
        autopatch_update_check_state = 0;
    }
    return autopatch_update_check_state;
}

#if 0
Original Ghidra decompilation (0x577240):

int autopatch_check_for_update_start(void)

{
  char cVar1;
  int iVar2;
  HANDLE hObject;
  DWORD *pDVar3;
  DWORD local_11c [2];
  byte local_114;
  undefined2 local_112;

  if (DAT_0069fe04 == -1) {
    iVar2 = security_check_write_access();
    if (iVar2 != 0) {
      pDVar3 = local_11c;
      for (iVar2 = 0x43; pDVar3 = pDVar3 + 1, iVar2 != 0; iVar2 = iVar2 + -1) {
        *pDVar3 = 0;
      }
      local_11c[1] = 0x66696c6f;
      local_112 = 0xffff;
      if ((local_114 & 1) != 0) {
        path_remove_last_component();
      }
      path_append_component();
      local_114 = local_114 | 1;
      cVar1 = file_reference_exists();
      if (cVar1 != '\0') {
        file_reference_delete();
      }
    }
    hObject = CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x10400,autopatch_version_check_request,
                           (LPVOID)0x0,0,local_11c);
    if (hObject != (HANDLE)0xffffffff) {
      DAT_0069fe04 = 1;
      CloseHandle(hObject);
      return DAT_0069fe04;
    }
    DAT_0069fe04 = 0;
  }
  return DAT_0069fe04;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
