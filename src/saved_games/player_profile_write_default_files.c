// player_profile_write_default_files  (Ghidra: FUN_0053a610, renamed)
// address 0x53a610, size 347 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/saved_games_functions.md summary "Generates and writes the two default
// profile save files (00.sav/01.sav) to disk." For each of local_player_index 0 and 1, builds a
// fresh default profile (player_profile_initialize, merge_existing=0), computes its crc, and
// writes it as "<default_player_profiles_directory>\NN.sav" via a freshly zeroed
// file_reference_record (signature/location set directly, matching file_reference_record's
// layout). The manual "append path" dance (find end of the file_reference's path field, insert
// a backslash if it isn't empty, strncpy the built name on) is reproduced faithfully even though
// the path field is always empty at that point in this function (it was just zeroed 2 lines
// above), which is provably dead code, not "fixed" here; ditto the leading
// path_remove_last_component() call, gated on a flag byte that was also just zeroed.
// register convention: no parameters, no return value.
// UNSURE: whether the always-empty-path branches are genuinely intentional defensive code or a
// compiler artifact of a helper this was inlined from; reproduced literally either way.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char default_player_profiles_directory[0x100]; // 0x00721849

extern void player_profile_initialize(saved_player_profile *profile, int32_t local_player_index,
    uint8_t merge_existing); // 0x53a1c0
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0
extern void path_remove_last_component(char *path); // 0x555f80, this module
extern uint8_t file_reference_create(file_reference_record *ref); // this module
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode); // this module
extern uint8_t file_reference_seek(int32_t offset, file_reference_record *ref); // this module
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // this module
extern uint8_t file_reference_close(file_reference_record *ref); // this module

void player_profile_write_default_files(void)
{
    int32_t local_player_index;
    saved_player_profile_file file;
    char name[256];
    file_reference_record ref;
    char *end;

    for (local_player_index = 0; local_player_index < 2; local_player_index = local_player_index + 1) {
        player_profile_initialize(&file.profile, local_player_index, 0);
        _snprintf(name, 0xff, "%s\\%02d.sav", default_player_profiles_directory, local_player_index);

        {
            uint8_t *zero = (uint8_t *)&ref;
            int32_t i;
            for (i = sizeof(ref); i != 0; i = i - 1) {
                *zero = 0;
                zero = zero + 1;
            }
        }
        ref.signature = k_file_reference_signature;
        ref.location = _file_location_absolute;

        if ((ref.flags & 1) != 0) {
            path_remove_last_component(ref.path); /* unreachable: ref.flags was just zeroed above */
        }

        if (name[0] != 0) {
            end = ref.path;
            while (*end != 0) {
                end = end + 1;
            }
            if (end != ref.path) {
                *end = '\\';
                end = end + 1;
                *end = 0;
                end = end + 1;
            }
            strncpy(end, name, 0xff - (uint32_t)(end - (ref.path + 1)));
        }
        ref.flags |= 1;

        file.checksum = 0xffffffff;
        crc32_update(&file.checksum, (uint8_t *)&file.profile, k_saved_player_profile_size);

        if (file_reference_create(&ref) != 0 && file_reference_open(&ref, 2) != 0 &&
            file_reference_seek(0, &ref) != 0) {
            file_reference_write(&ref, &file, sizeof(file));
            file_reference_close(&ref);
        }
    }
}

#if 0
Original Ghidra decompilation (0x53a610):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_0053a610(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  char *_Dest;
  int iVar4;
  undefined4 *puVar5;
  undefined4 local_2218;
  byte local_2214;
  undefined2 local_2212;
  char local_2210 [255];
  undefined1 local_2111;
  char local_2108 [256];
  undefined1 local_2008 [8188];
  undefined4 local_c [2];

  local_c[0] = 0x53a620;
  iVar4 = 0;
  do {
    player_profile_initialize(local_2008,iVar4,0);
    __snprintf(local_2108,0xff,"%s\\%02d.sav",&DAT_00721849,iVar4);
    puVar5 = &local_2218;
    for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    local_2218 = 0x66696c6f;
    local_2212 = 2;
    if ((local_2214 & 1) != 0) {
      path_remove_last_component();
    }
    if (local_2108[0] != '\0') {
      pcVar2 = local_2210;
      do {
        _Dest = pcVar2;
        pcVar2 = _Dest + 1;
      } while (*_Dest != '\0');
      if (_Dest != local_2210) {
        *_Dest = '\\';
        *pcVar2 = '\0';
        _Dest = pcVar2;
      }
      pcVar2 = local_2210;
      do {
        cVar1 = *pcVar2;
        pcVar2 = pcVar2 + 1;
      } while (cVar1 != '\0');
      _strncpy(_Dest,local_2108,0xff - ((int)pcVar2 - (int)(local_2210 + 1)));
      local_2111 = 0;
    }
    local_2214 = local_2214 | 1;
    local_c[0] = 0xffffffff;
    crc32_update(local_c,local_2008,0x1ffc);
    cVar1 = file_reference_create();
    if (cVar1 != '\0') {
      cVar1 = file_reference_open(2);
      if (cVar1 != '\0') {
        cVar1 = file_reference_seek();
        if (cVar1 != '\0') {
          file_reference_write();
          file_reference_close();
        }
      }
    }
    iVar4 = iVar4 + 1;
  } while (iVar4 < 2);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
