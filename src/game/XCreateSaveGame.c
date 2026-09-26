// XCreateSaveGame  (Ghidra: XCreateSaveGame, already named -- CEA/PDB match)
// address 0x551710, size 653 bytes
// name confidence: 0.85 (CEA/PDB match)   rewrite confidence: 0.9 (step 1: checked against objdump -d 0x551710..0x55199d)
// evidence: out/phase4/game_types_notes.md ("save games" section) attributes this address to
//   the CEA string "XCreateSaveGame"; the strings "%s\\%s\\", "Name=%s\n", "%s\\%s" and
//   "checkpoints\\" match the four sprintf/path-building steps below. Ghidra's own local
//   variable names encode their exact stack offsets (local_8a4, local_8a0, local_820, local_718,
//   cStack_611, local_610, local_604, local_508, local_400), which is what proves the
//   local_718+cStack_611 pair and the local_610+local_604 pair are each really one contiguous
//   264-byte buffer that Ghidra split by access width; this rewrite keeps them as two named
//   264-byte arrays (`slot_path` and `checkpoint_dir`) rather than reproducing the raw
//   off-by-one pointer trick the compiler used to append "checkpoints\\" (same final bytes,
//   without the buffer-boundary hazard).
// register convention: a value gating the whole call (Ghidra's `in_EAX`, checked only for
//   non-zero, never otherwise read) in EAX; `root_path`, `mode`, `out_path` and `out_path_size`
//   are this function's own four recognized stack parameters.
//   // blam-cc: EAX -> save_game_name, stack -> root_path, mode, out_path, out_path_size
// UNSURE: `validity_token`'s real identity (a device/user handle, presumably -- checked but
//   never used again); string_convert_unicode_to_ascii/string_convert_ascii_to_unicode's exact argument lists (their destination
//   buffers are elided registers, modeled here as explicit parameters guessed from context);
//   the numeric HRESULT-shaped return constants (0x57 = ERROR_INVALID_PARAMETER,
//   0x80004005 = E_FAIL) are Win32/COM values, not reinterpreted.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern uint8_t *string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity); // 0x557950, blam-cc: ESI dest, EDI source, stack capacity
    // UNSURE: EAX -> out_name, stack -> max_length (guessed -- builds the checkpoint/save name)
extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source); // 0x557990, blam-cc: EAX dst, EDI capacity_bytes, EBX source
    // UNSURE: EAX -> text, ECX -> length (guessed -- writes the "Name=" info file)
extern int _sprintf(char *dest, const char *format, ...); // MSVC CRT
extern char *_strncpy(char *dest, const char *src, uint32_t count); // MSVC CRT
extern uint32_t GetFileAttributesA(const char *path); // Win32
extern uint32_t CreateDirectoryA(const char *path, void *security_attributes); // Win32
extern void *CreateFileA(const char *path, uint32_t access, uint32_t share_mode,
    void *security_attributes, uint32_t creation_disposition, uint32_t flags, void *template_file); // Win32
extern uint32_t WriteFile(void *file, const void *buffer, uint32_t bytes_to_write,
    uint32_t *bytes_written, void *overlapped); // Win32
extern uint32_t CloseHandle(void *handle); // Win32

// blam-cc: EAX -> save_game_name, stack -> root_path, mode, out_path, out_path_size
// FIXED (step 1, objdump -d 0x551710..0x55199d): EAX is the save game's Unicode name -- the source of the ASCII name
// every path is built from (0x551762: ESI = ascii name, EDI = name, push 0x80); the draft called it an unused token
// and converted nothing. The "Name=" line is converted into a scratch buffer (EAX dst, EBX src, EDI 0x80).
// Builds "<root_path>\<name>\" (slot_path) and "<root_path>\<name>" (its trailing-slash-free
// twin, reused as the file to create), and "<root_path>\<name>" 's info line "Name=<name>\n";
// ensures root_path exists (creating it if needed); for mode 3 (open existing), just touches
// the file; for mode 1/4, creates the slot directory (plus a "checkpoints\" subdirectory the
// first time the slot itself is created) and writes the slot file's own path string into it (0x551920: the
// "Name=" line is built and converted but never written). On
// success, copies slot_path into *out_path. Returns 0 on success, or a Win32/HRESULT-style
// error code.
uint32_t XCreateSaveGame(const uint16_t *save_game_name, const char *root_path, int32_t mode,
    char *out_path, uint32_t out_path_size)
{
    uint32_t bytes_written = 0;
    char name[128];
    char slot_path[264];       // Ghidra local_820
    char slot_dir[264];        // Ghidra local_718 + cStack_611
    char checkpoint_dir[264];  // Ghidra local_610 + local_604
    char slot_file_no_slash[264]; // Ghidra local_508
    char info_line[1024];      // Ghidra local_400
    uint32_t root_attrs;
    uint32_t slot_attrs;
    uint32_t have_slot;
    void *file;
    int32_t i;

    if (root_path == 0 || save_game_name == 0 || out_path == 0 || out_path_size == 0) {
        return 0x57;
    }

    string_convert_unicode_to_ascii((uint8_t *)name, (uint16_t *)save_game_name, 0x80);
    _sprintf(slot_dir, "%s\\%s\\", root_path, name);
    _sprintf(slot_path, "%s%s", slot_dir, name);
    _sprintf(info_line, "Name=%s\n", name);
    string_convert_ascii_to_unicode((uint16_t *)checkpoint_dir, 0x80, info_line); // result unused (buffer reused below)

    _sprintf(slot_file_no_slash, "%s\\%s", root_path, name);

    root_attrs = GetFileAttributesA(root_path);
    if (root_attrs == 0xffffffff && CreateDirectoryA(root_path, 0) == 0) {
        return 0x80004005;
    }

    slot_attrs = GetFileAttributesA(slot_file_no_slash);
    have_slot = 1;
    if (slot_attrs == 0xffffffff) {
        have_slot = bytes_written;
    }

    if (mode != 1) {
        if (mode == 3) {
            if (have_slot == 0) {
                return 0x80004005;
            }
            file = CreateFileA(slot_path, 0, 0, 0, 2, 0x80, 0);
            if (file == (void *)0xffffffff) {
                return 0x80004005;
            }
            CloseHandle(file);
            return 0;
        }
        if (mode != 4) {
            return 0x57;
        }
    }

    if (have_slot == 0) {
        if (CreateDirectoryA(slot_dir, 0) == 0) {
            return 0x80004005;
        }

        // Copy slot_dir's string into checkpoint_dir (strcpy), then append "checkpoints\".
        i = 0;
        do {
            checkpoint_dir[i] = slot_dir[i];
            i = i + 1;
        } while (slot_dir[i - 1] != '\0');
        {
            int32_t end = 0;
            while (checkpoint_dir[end] != '\0') {
                end = end + 1;
            }
            _strncpy(checkpoint_dir + end, "checkpoints\\", 0xd);
        }

        if (CreateDirectoryA(checkpoint_dir, 0) == 0) {
            return 0x80004005;
        }
    }

    file = CreateFileA(slot_path, 0xc0000000, 0, 0, 2, 0x80, 0);
    if (file != (void *)0xffffffff) {
        int32_t length = 0;
        while (slot_path[length] != '\0') {
            length = length + 1;
        }
        WriteFile(file, slot_path, length, &bytes_written, 0);
        CloseHandle(file);
        if ((uint32_t)length == bytes_written) {
            _strncpy(out_path, slot_dir, out_path_size);
            return 0;
        }
    }
    return 0x80004005;
}

#if 0
Original Ghidra decompilation (0x551710), from tools/pack.py 0x551710:

undefined4 XCreateSaveGame(LPCSTR param_1,int param_2,char *param_3,size_t param_4)

{
  char cVar1;
  int in_EAX;
  DWORD DVar2;
  BOOL BVar3;
  DWORD DVar4;
  int iVar5;
  HANDLE pvVar6;
  char *pcVar7;
  char *pcVar8;
  DWORD local_8a4;
  undefined1 local_8a0 [128];
  char local_820 [264];
  char local_718 [263];
  char cStack_611;
  CHAR local_610 [12];
  char local_604 [252];
  char local_508 [264];
  char local_400 [1024];

  local_8a4 = 0;
  if ((((param_1 == (LPCSTR)0x0) || (in_EAX == 0)) || (param_3 == (char *)0x0)) || (param_4 == 0)) {
    return 0x57;
  }
  FUN_00557950(0x80);
  _sprintf(local_718,"%s\\%s\\",param_1,local_8a0);
  _sprintf(local_820,"%s%s",local_718,local_8a0);
  _sprintf(local_400,"Name=%s\n",local_8a0);
  FUN_00557990();
  _sprintf(local_508,"%s\\%s",param_1,local_8a0);
  DVar2 = GetFileAttributesA(param_1);
  if ((DVar2 == 0xffffffff) &&
     (BVar3 = CreateDirectoryA(param_1,(LPSECURITY_ATTRIBUTES)0x0), BVar3 == 0)) {
    return 0x80004005;
  }
  DVar4 = GetFileAttributesA(local_508);
  DVar2 = 1;
  if (DVar4 == 0xffffffff) {
    DVar2 = local_8a4;
  }
  if (param_2 != 1) {
    if (param_2 == 3) {
      if (DVar2 == 0) {
        return 0x80004005;
      }
      pvVar6 = CreateFileA(local_820,0,0,(LPSECURITY_ATTRIBUTES)0x0,2,0x80,(HANDLE)0x0);
      if (pvVar6 == (HANDLE)0xffffffff) {
        return 0x80004005;
      }
      CloseHandle(pvVar6);
      return 0;
    }
    if (param_2 != 4) {
      return 0x57;
    }
  }
  if (DVar2 == 0) {
    BVar3 = CreateDirectoryA(local_718,(LPSECURITY_ATTRIBUTES)0x0);
    if (BVar3 == 0) {
      return 0x80004005;
    }
    iVar5 = 0;
    do {
      pcVar7 = local_718 + iVar5;
      local_610[iVar5] = *pcVar7;
      iVar5 = iVar5 + 1;
    } while (*pcVar7 != '\0');
    pcVar7 = &cStack_611;
    do {
      pcVar8 = pcVar7;
      pcVar7 = pcVar8 + 1;
    } while (pcVar8[1] != '\0');
    builtin_strncpy(pcVar8 + 1,"checkpoints\\",0xd);
    BVar3 = CreateDirectoryA(local_610,(LPSECURITY_ATTRIBUTES)0x0);
    if (BVar3 == 0) {
      return 0x80004005;
    }
  }
  pvVar6 = CreateFileA(local_820,0xc0000000,0,(LPSECURITY_ATTRIBUTES)0x0,2,0x80,(HANDLE)0x0);
  if (pvVar6 != (HANDLE)0xffffffff) {
    pcVar7 = local_820;
    do {
      cVar1 = *pcVar7;
      pcVar7 = pcVar7 + 1;
    } while (cVar1 != '\0');
    WriteFile(pvVar6,local_820,(int)pcVar7 - (int)(local_820 + 1),&local_8a4,(LPOVERLAPPED)0x0);
    CloseHandle(pvVar6);
    if ((int)pcVar7 - (int)(local_820 + 1) == local_8a4) {
      _strncpy(param_3,local_718,param_4);
      return 0;
    }
  }
  return 0x80004005;
}
#endif
