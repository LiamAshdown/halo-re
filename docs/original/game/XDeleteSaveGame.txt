// XDeleteSaveGame  (Ghidra: XDeleteSaveGame, already named -- CEA/PDB match)
// address 0x5519a0, size 528 bytes
// name confidence: 0.85 (CEA/PDB match)   rewrite confidence: 0.35
// evidence: out/phase4/game_types_notes.md ("save games" section); CEA string match on
//   "%s*.*"/"%scheckpoints"/"checkpoints"; the sibling XCreateSaveGame.c (this batch) for the
//   string_convert_unicode_to_ascii(name_buffer, max_length) call shape. Ghidra's local variable names again
//   encode exact stack offsets (local_3d0, acStack_351, local_248, local_108) that prove
//   local_3d0/acStack_351/local_248/local_108 are one contiguous stack region; this rewrite
//   keeps them as separate named buffers instead (dropping the single unused pad byte
//   `acStack_351[0]` the original left before its own string, which nothing ever reads).
// register convention: a validity token in EAX (in_EAX) and the save-game root path in ECX
//   (in_ECX); no stack parameters.
//   // blam-cc: EAX -> save_game_name, ECX -> root_path
// (the first argument is the save name -- see FIXED below; the old "validity_token" reading was wrong)
//   XCreateSaveGame.c); string_convert_unicode_to_ascii's exact argument list (guessed by analogy).

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// win32_find_dataa is types/game.h's (the Win32 WIN32_FIND_DATAA layout, 0x140 bytes).

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t *string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity); // 0x557950, ESI dest, EDI source, stack capacity

// blam-cc: EAX -> save_game_name, ECX -> root_path
// Deletes every file directly under "<root_path>\<name>\" (skipping dotfiles and an entry
// literally named "checkpoints"), then every file under its "checkpoints\" subdirectory,
// removes that subdirectory, and finally removes "<root_path>\<name>" itself (trailing slash
// trimmed). Returns 0 on success (the final RemoveDirectoryA returned 1), 1 otherwise.
// FIXED (objdump 0x5519a7..0x5519cd): the first argument (EAX, kept in EDI) is the save's wide-character name -- it is
// the source of the name conversion below -- not a validity token.
uint32_t XDeleteSaveGame(const uint16_t *save_game_name, const char *root_path)
{
    char name[128];
    char root_with_slash[266];
    char pattern[264];
    char delete_path[127];
    win32_find_dataa find_data;
    void *find_handle;
    uint32_t last_delete_ok;
    uint32_t removed_checkpoints_dir;
    int32_t result;

    if (root_path == 0 || save_game_name == 0) {
        return 0x57;
    }

    string_convert_unicode_to_ascii((uint8_t *)name, (uint16_t *)save_game_name, 0x80); // 0x5519c4: ESI = name, EDI = the argument
    sprintf(root_with_slash, "%s\\%s\\", root_path, name);

    sprintf(pattern, "%s*.*", root_with_slash);
    last_delete_ok = 0;
    find_handle = FindFirstFileA(pattern, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle != (void *)0xffffffff) {
        do {
            if (find_data.cFileName[0] != '.') {
                {
                    int32_t i = 0xc;
                    uint8_t matches_checkpoints = 1;
                    const char *a = find_data.cFileName;
                    const char *b = "checkpoints";
                    while (i != 0 && matches_checkpoints) {
                        i = i - 1;
                        matches_checkpoints = (*a == *b);
                        a = a + 1;
                        b = b + 1;
                    }
                    if (!matches_checkpoints) {
                        sprintf(delete_path, "%s%s", root_with_slash, find_data.cFileName);
                        last_delete_ok = DeleteFileA(delete_path);
                        if (last_delete_ok == 0) {
                            break;
                        }
                    }
                }
            }
        } while (FindNextFileA(find_handle, (LPWIN32_FIND_DATAA)&find_data) != 0);
        FindClose(find_handle);
    }

    sprintf(pattern, "%scheckpoints\\*.*", root_with_slash);
    result = 0;
    if (last_delete_ok != 0) {
        find_handle = FindFirstFileA(pattern, (LPWIN32_FIND_DATAA)&find_data);
        if (find_handle != (void *)0xffffffff) {
            uint32_t has_more;
            do {
                if (find_data.cFileName[0] != '.') {
                    int32_t i = 0xc;
                    uint8_t matches_checkpoints = 1;
                    const char *a = find_data.cFileName;
                    const char *b = "checkpoints";
                    while (i != 0 && matches_checkpoints) {
                        i = i - 1;
                        matches_checkpoints = (*a == *b);
                        a = a + 1;
                        b = b + 1;
                    }
                    if (!matches_checkpoints) {
                        sprintf(delete_path, "%scheckpoints\\%s", root_with_slash, find_data.cFileName);
                        DeleteFileA(delete_path);
                    }
                }
                has_more = FindNextFileA(find_handle, (LPWIN32_FIND_DATAA)&find_data);
            } while (has_more != 0);
            FindClose(find_handle);
            sprintf(pattern, "%scheckpoints", root_with_slash);
            removed_checkpoints_dir = RemoveDirectoryA(pattern);
        } else {
            // UNSURE: when the checkpoints-dir search finds nothing, `removed_checkpoints_dir`
            // is never (re)assigned here -- the original leaves BVar4 at whatever the first
            // loop's last DeleteFileA returned, which is nonzero because we are in this branch.
            removed_checkpoints_dir = last_delete_ok;
        }

        result = 0;
        if (removed_checkpoints_dir != 0) {
            int32_t end = 0;
            while (root_with_slash[end] != '\0') {
                end = end + 1;
            }
            root_with_slash[end - 1] = '\0'; // trims the trailing '\\'
            result = (int32_t)RemoveDirectoryA(root_with_slash);
        }
    }
    return result != 1;
}

#if 0
Original Ghidra decompilation (0x5519a0), from tools/pack.py 0x5519a0:

undefined1 XDeleteSaveGame(void)

{
  int in_EAX;
  HANDLE pvVar1;
  BOOL BVar2;
  int in_ECX;
  int iVar3;
  BOOL BVar4;
  CHAR *pCVar5;
  char *pcVar6;
  bool bVar7;
  char local_3d0 [127];
  char acStack_351 [265];
  _WIN32_FIND_DATAA local_248;
  char local_108 [264];

  if ((in_ECX == 0) || (in_EAX == 0)) {
    return 0x57;
  }
  FUN_00557950(0x80);
  _sprintf(acStack_351 + 1,"%s\\%s\\");
  _sprintf(local_108,"%s*.*",acStack_351 + 1);
  BVar4 = 0;
  pvVar1 = FindFirstFileA(local_108,&local_248);
  if (pvVar1 != (HANDLE)0xffffffff) {
    do {
      if (local_248.cFileName[0] != '.') {
        iVar3 = 0xc;
        bVar7 = true;
        pCVar5 = local_248.cFileName;
        pcVar6 = "checkpoints";
        do {
          if (iVar3 == 0) break;
          iVar3 = iVar3 + -1;
          bVar7 = *pCVar5 == *pcVar6;
          pCVar5 = pCVar5 + 1;
          pcVar6 = pcVar6 + 1;
        } while (bVar7);
        if (!bVar7) {
          _sprintf(local_3d0,"%s%s",acStack_351 + 1,local_248.cFileName);
          BVar4 = DeleteFileA(local_3d0);
          if (BVar4 == 0) break;
        }
      }
      BVar2 = FindNextFileA(pvVar1,&local_248);
    } while (BVar2 != 0);
    FindClose(pvVar1);
  }
  _sprintf(local_108,"%scheckpoints\\*.*",acStack_351 + 1);
  iVar3 = 0;
  if (BVar4 != 0) {
    pvVar1 = FindFirstFileA(local_108,&local_248);
    if (pvVar1 != (HANDLE)0xffffffff) {
      do {
        if (local_248.cFileName[0] != '.') {
          iVar3 = 0xc;
          bVar7 = true;
          pCVar5 = local_248.cFileName;
          pcVar6 = "checkpoints";
          do {
            if (iVar3 == 0) break;
            iVar3 = iVar3 + -1;
            bVar7 = *pCVar5 == *pcVar6;
            pCVar5 = pCVar5 + 1;
            pcVar6 = pcVar6 + 1;
          } while (bVar7);
          if (!bVar7) {
            _sprintf(local_3d0,"%scheckpoints\\%s",acStack_351 + 1,local_248.cFileName);
            DeleteFileA(local_3d0);
          }
        }
        BVar4 = FindNextFileA(pvVar1,&local_248);
      } while (BVar4 != 0);
      FindClose(pvVar1);
      _sprintf(local_108,"%scheckpoints",acStack_351 + 1);
      BVar4 = RemoveDirectoryA(local_108);
    }
    iVar3 = 0;
    if (BVar4 != 0) {
      pcVar6 = acStack_351;
      do {
        pcVar6 = pcVar6 + 1;
      } while (*pcVar6 != '\0');
      pcVar6[-1] = '\0';
      iVar3 = RemoveDirectoryA(acStack_351 + 1);
    }
  }
  return iVar3 != 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
