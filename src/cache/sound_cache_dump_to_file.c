// sound_cache_dump_to_file  (Ghidra: sound_cache_dump_to_file, already named)
// address 0x444240, size 480 bytes
// name confidence: 0.85   rewrite confidence: 0.55
// evidence: out/phase4/cache_types_notes.md names this function directly (the "one observation
// for another module" section, about its stack data_iterator) and its "misattributed" item 1
// establishes that its tail (the per-entry listing loop) was split off by Ghidra into the bogus
// FUN_00444420 / "data_file_read" -- not rewritten here, folded back into this function instead.
// FUN_004d1ca0 is cache_build_status_bitmap (src/memory/cache_build_status_bitmap.c), whose
// cache_block_status_flags bit values (1/2/4/8) fix the four counters below.
// register convention: none; plain __cdecl with no parameters.
//
// UNSURE: FUN_00624186 (0x00624186) is far outside this module's range; declared here only as
// an fopen-shaped wrapper matching its two visible arguments and FILE*-compatible return use.
// The observation in cache_types_notes.md about a fourth, self-check "iter" field on the stack
// data_iterator objects here is not reproduced -- types/memory.h's data_iterator is still 0x0c
// bytes / 3 fields, and that note is flagged for the memory module, not acted on here.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "fn_cache.h"
#include <stdint.h>

extern int32_t sound_cache_page_count;   // 0x006f17e4
extern struct cache *sound_cache;        // 0x006ac530
extern data_array *sound_cache_entries;  // 0x006ac528
extern int32_t sound_cache_size_megabytes; // 0x006869c4, read but not owned by this module
extern tag_instance *tag_instances;      // 0x0087bc14
extern char file_open_mode_w[];          // 0x0065ff44, likely "w"

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void cache_build_status_bitmap(struct cache *self, uint8_t *bitmap); // 0x4d1ca0

// UNSURE: see file header note above.

// Writes a diagnostic dump of the sound cache's page usage statistics and per-entry list to
// sound_cache_dump.txt: total sounds resident, MB used/free, and allocated/used-this-frame/
// old/locked page counts (from cache_build_status_bitmap's per-block status bits), followed by
// one line per live sound_cache_entry naming its permutation's tag path, compressed size and
// uncompressed size.
void sound_cache_dump_to_file(void)
{
    int32_t saved_page_count;
    int32_t allocated_pages;
    int32_t current_pages;
    int32_t old_pages;
    int32_t locked_pages;
    int32_t sound_count;
    uint8_t *bitmap;
    void *file;
    char line[1028];
    char *scan;
    int32_t bit;
    uint32_t page;
    float total_mb;
    float free_pages;
    data_iterator iterator;
    sound_cache_entry *entry;
    SoundPermutation *permutation;
    int32_t entry_number;

    saved_page_count = sound_cache_page_count;
    current_pages = 0;
    allocated_pages = 0;
    old_pages = 0;
    locked_pages = 0;
    sound_count = 0;

    bitmap = (uint8_t *)GlobalAlloc(0, sound_cache_page_count);
    file = fopen("sound_cache_dump.txt", file_open_mode_w);

    for (scan = line, bit = 0x100; bit != 0; bit--) {
        scan[0] = 0; scan[1] = 0; scan[2] = 0; scan[3] = 0;
        scan += 4;
    }

    if (file != (void *)0) {
        cache_build_status_bitmap(sound_cache, bitmap);

        for (bit = 0; bit < 4; bit++) {
            for (page = 0; page < (uint32_t)sound_cache_page_count; page++) {
                if ((bitmap[page] & (1 << (bit & 0x1f))) != 0) {
                    if (bit == 0) {
                        allocated_pages++;
                    } else if (bit == 1) {
                        current_pages++;
                    } else if (bit == 2) {
                        old_pages++;
                    } else {
                        locked_pages++;
                    }
                }
            }
        }

        iterator.data = sound_cache_entries;
        iterator.next_index = 0;
        iterator.index = 0;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        entry = (sound_cache_entry *)data_iterator_next(&iterator);
        while (entry != (sound_cache_entry *)0) {
            sound_count++;
            entry = (sound_cache_entry *)data_iterator_next(&iterator);
        }

        {
            float mb_total = (float)sound_cache_size_megabytes;
            float page_count_f = (float)saved_page_count;
            if (saved_page_count < 0) {
                page_count_f = page_count_f + 4.2949673e+09f;
            }
            free_pages = (mb_total / page_count_f) * (page_count_f - (float)allocated_pages);

            sprintf(line,
                "%d / 512 sounds in cache\n%.2f MB / %.2f MB used %.2f percent free\n%d / %d pages allocated\n%d / %d pages used this frame\n%d / %d pages old\n%d / %d pages locked\n\n",
                sound_count, (double)(mb_total - free_pages), (double)(int32_t)sound_cache_size_megabytes,
                (double)((free_pages / mb_total) * 100.0f),
                allocated_pages, saved_page_count, current_pages, saved_page_count,
                old_pages, saved_page_count, locked_pages, saved_page_count);
        }

        for (scan = line; *scan != '\0'; scan++) {
        }
        fwrite(line, 1, (uint32_t)(scan - (line + 1)), file);

        entry_number = 1;
        for (scan = line, bit = 0x100; bit != 0; bit--) {
            scan[0] = 0; scan[1] = 0; scan[2] = 0; scan[3] = 0;
            scan += 4;
        }
        fwrite("[sounds in cache]\n\n", 1, 0x12, file);

        iterator.data = sound_cache_entries;
        iterator.next_index = 0;
        iterator.index = 0;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        entry = (sound_cache_entry *)data_iterator_next(&iterator);
        while (entry != (sound_cache_entry *)0) {
            permutation = entry->permutation;
            if (permutation != (SoundPermutation *)0) {
                sprintf(line, "%d - %s %d c bytes %d u bytes\n", entry_number,
                    tag_instances[permutation->tag_id_1.index].path,
                    permutation->samples.size, permutation->buffer_size);
                for (scan = line; *scan != '\0'; scan++) {
                }
                fwrite(line, 1, (uint32_t)(scan - (line + 1)), file);
                entry_number++;
            }
            entry = (sound_cache_entry *)data_iterator_next(&iterator);
        }

        fclose(file);
    }

    GlobalFree(bitmap);
    return;
}

#if 0
Original Ghidra decompilation (0x444240):

void sound_cache_dump_to_file(void)

{
  char cVar1;
  float fVar2;
  float fVar3;
  SIZE_T SVar4;
  HGLOBAL hMem;
  FILE *_File;
  uint uVar5;
  char *pcVar6;
  int iVar7;
  int iVar8;
  int local_438;
  int local_434;
  int local_42c;
  int local_428;
  int local_424;
  char local_408 [1028];

  SVar4 = DAT_006f17e4;
  local_434 = 0;
  local_438 = 0;
  local_428 = 0;
  local_424 = 0;
  local_42c = 0;
  hMem = GlobalAlloc(0,DAT_006f17e4);
  _File = (FILE *)FUN_00624186("sound_cache_dump.txt",&DAT_0065ff44);
  pcVar6 = local_408;
  for (iVar7 = 0x100; iVar7 != 0; iVar7 = iVar7 + -1) {
    pcVar6[0] = '\0';
    pcVar6[1] = '\0';
    pcVar6[2] = '\0';
    pcVar6[3] = '\0';
    pcVar6 = pcVar6 + 4;
  }
  if (_File != (FILE *)0x0) {
    FUN_004d1ca0(DAT_006ac530,hMem);
    iVar7 = 0;
    do {
      uVar5 = 0;
      if (DAT_006f17e4 != 0) {
        do {
          if ((*(byte *)((int)(short)uVar5 + (int)hMem) & (byte)(1 << ((byte)iVar7 & 0x1f))) != 0) {
            if (iVar7 == 0) {
              local_438 = local_438 + 1;
            }
            else if (iVar7 == 1) {
              local_428 = local_428 + 1;
            }
            else if (iVar7 == 2) {
              local_424 = local_424 + 1;
            }
            else {
              local_42c = local_42c + 1;
            }
          }
          uVar5 = uVar5 + 1;
        } while (uVar5 < DAT_006f17e4);
      }
      iVar7 = iVar7 + 1;
    } while (iVar7 < 4);
    iVar7 = data_iterator_next();
    while (iVar7 != 0) {
      local_434 = local_434 + 1;
      iVar7 = data_iterator_next();
    }
    fVar3 = (float)(int)DAT_006869c4;
    fVar2 = (float)(int)SVar4;
    if ((int)SVar4 < 0) {
      fVar2 = fVar2 + 4.2949673e+09;
    }
    fVar2 = (fVar3 / fVar2) * (fVar2 - (float)local_438);
    _sprintf(local_408,
             "%d / 512 sounds in cache\n%.2f MB / %.2f MB used %.2f percent free\n%d / %d pages allocated\n%d / %d pages used this frame\n%d / %d pages old\n%d / %d pages locked\n\n"
             ,local_434,(double)(fVar3 - fVar2),(double)(int)DAT_006869c4,
             (double)((fVar2 / fVar3) * 100.0),local_438,SVar4,local_428,SVar4,local_424,SVar4,
             local_42c,SVar4);
    pcVar6 = local_408;
    do {
      cVar1 = *pcVar6;
      pcVar6 = pcVar6 + 1;
    } while (cVar1 != '\0');
    _fwrite(local_408,1,(int)pcVar6 - (int)(local_408 + 1),_File);
    iVar7 = 1;
    pcVar6 = local_408;
    for (iVar8 = 0x100; iVar8 != 0; iVar8 = iVar8 + -1) {
      pcVar6[0] = '\0';
      pcVar6[1] = '\0';
      pcVar6[2] = '\0';
      pcVar6[3] = '\0';
      pcVar6 = pcVar6 + 4;
    }
    _fwrite("[sounds in cache]\n\n",1,0x12,_File);
    iVar8 = data_iterator_next();
    while (iVar8 != 0) {
      iVar8 = *(int *)(iVar8 + 0xc);
      if (iVar8 != 0) {
        _sprintf(local_408,"%d - %s %d c bytes %d u bytes\n",iVar7,
                 *(undefined4 *)(*(short *)(iVar8 + 0x3c) * 0x20 + 0x10 + DAT_0087bc14),
                 *(undefined4 *)(iVar8 + 0x40),*(undefined4 *)(iVar8 + 0x38));
        pcVar6 = local_408;
        do {
          cVar1 = *pcVar6;
          pcVar6 = pcVar6 + 1;
        } while (cVar1 != '\0');
        _fwrite(local_408,1,(int)pcVar6 - (int)(local_408 + 1),_File);
        iVar7 = iVar7 + 1;
      }
      iVar8 = data_iterator_next();
    }
    _fclose(_File);
  }
  GlobalFree(hMem);
  return;
}
#endif
