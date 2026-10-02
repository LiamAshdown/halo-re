// server_list_menu_update  (Ghidra: server_list_menu_update, already named)
// address 0x4a5040, size 1724 bytes, the largest function in this session's range
// name confidence: 0.7   rewrite confidence: 0.55 (lowest-confidence tier this session, alongside
// FUN_004a2ad0.c/FUN_004a2cb0.c/player_profile_1wide_list_update.c)
// evidence: matches the given name; functions.md: "Rebuilds the multiplayer server-browser list,
// matching each discovered game's map/variant and populating the on-screen list widgets."
// register convention: cdecl, the one recognized stack parameter (widget).
// UNSURE (significant, whole file): this is a best-effort, time-boxed translation of the largest
// and most intricate function in this session's range. Every field offset into the network_client
// LAN-discovery table (+4, stride 0x130) and the favorites/history table (+0x1c, stride 0x4c*4)
// is preserved as a raw offset rather than cross-referenced against types/networking.h's
// server_browser_entry (0x220 bytes, since renamed controls_gamepad_record: it is a gamepad record) or network_client_globals, since neither obviously lines up
// with these strides in the time available. The eleven-deep chained `strstr(name, "map")`
// if/else ladder (a strcmp-shaped call, given its two string arguments) is preserved as written
// rather than turned into a lookup table, to avoid asserting an ordering not directly evidenced.
// Phase-4 review against objdump 0x4a5040..0x4a56fc: strstr is strstr (the binary tests
// the result for non-zero, so a substring match picks the map index; the rewrite had every test
// inverted); the tenth widget of the details pane is r9->next_sibling (r10), not
// r3->next_sibling; the discovery age test is signed (jg 0x1770), the fade test unsigned; the
// "%d" format is the literal at 0x006607a0 (not a pointer read), the player count at +0x124 is
// zero-extended; heap_reallocate gets the old text in EAX and widget_memory_pool in ESI.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *server_list_entries_006b380c[9]; // 0x006b380c, TYPES-GAP
extern network_client_globals *network_client; // 0x0071c2d8
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern heap *widget_memory_pool; // 0x006926c4
extern uint16_t missing_string_text[]; // 0x00671fac
extern tag_instance *tag_instances; // 0x0087bc14
extern uint16_t chat_local_prompt_string[]; // 0x006607a0, L"%d"

extern uint8_t network_game_search_entry_is_fresh(const uint8_t *entry); // 0x4da770, blam-cc: ESI entry; reads entry+0x12d and a QPC age
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void string_format_wide_va_bounded(uint32_t count, wchar_t *dest, const wchar_t *format, ...); // 0x557910, blam-cc: EDX count

void server_list_menu_update(widget_instance *widget)
{
    uint8_t *client = (uint8_t *)network_client;
    int32_t count = 0;
    int32_t i;
    widget_instance *row;

    server_list_entries_006b380c[0] = (void *)0;
    server_list_entries_006b380c[1] = (void *)0;
    server_list_entries_006b380c[2] = (void *)0;
    server_list_entries_006b380c[3] = (void *)0;
    server_list_entries_006b380c[4] = (void *)0;
    server_list_entries_006b380c[5] = (void *)0;
    server_list_entries_006b380c[6] = (void *)0;
    server_list_entries_006b380c[7] = (void *)0;
    server_list_entries_006b380c[8] = (void *)0;

    if (client == (uint8_t *)0) {
        return;
    }

    {
        uint8_t *entry = client + 4;

        for (i = 0; i < 9; i++) {
            if (network_game_search_entry_is_fresh(entry) != 0 && *(int16_t *)(entry + 0x12a) == 1 && entry[300] != 0) {
                server_list_entries_006b380c[count] = entry;
                count++;
            }
            entry += 0x130;
        }
    }

    {
        int32_t *entry = (int32_t *)(client + 0x1c);

        for (i = 0; i < 9; i++) {
            if (*((int8_t *)entry + 0x115) != 0) {
                large_integer counter;
                int32_t now_ms;

                QueryPerformanceCounter((LARGE_INTEGER *)&counter);
                now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
                if (now_ms - *entry <= 0x1770 && *(int16_t *)((uint8_t *)entry + 0x112) == 1 &&
                    *((int8_t *)entry + 0x45 * 4) == 0) {
                    server_list_entries_006b380c[count] = (uint8_t *)entry - 0x18; // -6 dwords
                    count++;
                }
            }
            entry += 0x4c;
        }
    }

    widget->list_items = server_list_entries_006b380c;
    widget->item_count = (uint16_t)count;
    {
        int16_t bound = (count - 1 < widget->selection_index) ? (int16_t)(count - 1) : widget->selection_index;

        widget->selection_index = bound;
    }

    row = widget->first_child;
    for (i = 0; row != (widget_instance *)0 && i < count; i++) {
        uint16_t *buf = (uint16_t *)heap_reallocate(row->text, 0x20, widget_memory_pool);
        uint8_t *entry = (uint8_t *)server_list_entries_006b380c[i];

        row->text = buf;
        if (buf != (uint16_t *)0) {
            if (entry[300] == 1) {
                wcsncpy((wchar_t *)buf, (const wchar_t *)((const uint16_t *)(entry + 0x1c)), 0xf);
                ((uint16_t *)row->text)[0xf] = 0;
            } else {
                datum_index tag = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text");
                uint16_t *source = missing_string_text; // L"<missing string>"

                if (tag != (datum_index)-1) {
                    UnicodeStringList *list = (UnicodeStringList *)tag_instances[tag & 0xffff].data;

                    if (list->strings.count > 0x13) {
                        UnicodeStringListString *strings = (UnicodeStringListString *)list->strings.pointer;
                        uint32_t size = strings[0x13].string.size;

                        if ((int32_t)size > 0) {
                            source = (uint16_t *)strings[0x13].string.pointer;
                            *(uint16_t *)((uint8_t *)source + ((size & 0xfffffffe) - 2)) = 0;
                        }
                    }
                }
                string_format_wide_va_bounded(0xf, (wchar_t *)buf, L"%s %s", source, entry + 0x1c); // EDX 0xf (FIXED: count passed)
                ((uint16_t *)row->text)[0xf] = 0;
            }
        }
        row = row->next_sibling;
    }

    if (count > 0 && widget->selection_index < 0) {
        widget->selection_index = 0;
    }

    {
        large_integer counter;
        int32_t now_ms;
        widget_instance *r1 = widget->extended_description->first_child;
        widget_instance *r2 = r1->next_sibling;
        widget_instance *r3 = r2->next_sibling->next_sibling;
        widget_instance *r4 = r2->next_sibling->first_child;
        widget_instance *r5 = r4->next_sibling;
        widget_instance *r6 = r5->next_sibling;
        widget_instance *r7 = r6->next_sibling;
        widget_instance *r8 = r7->next_sibling;
        widget_instance *r9 = r8->next_sibling;
        widget_instance *r10 = r9->next_sibling;

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

        if (widget->selection_index < 0) {
            r1->background_bitmap_frame = 5;
            r2->background_bitmap_frame = 0x13;
            r4->selection_index = 1;
            r5->selection_index = 0x14;
            r6->selection_index = 1;
            r7->selection_index = 1;
            {
                uint16_t *b = (uint16_t *)heap_reallocate(r8->text, 8, widget_memory_pool);

                r8->text = b;
                if (b != (uint16_t *)0) b[0] = 0;
            }
            {
                uint16_t *b = (uint16_t *)heap_reallocate(r9->text, 8, widget_memory_pool);

                r9->text = b;
                if (b != (uint16_t *)0) b[0] = 0;
            }
            r10->selection_index = 1;
            r3->selection_index = (uint32_t)(now_ms - widget->creation_time) > 999;
            r3->hidden = 1;
        } else {
            uint8_t *sel = (uint8_t *)server_list_entries_006b380c[widget->selection_index];
            int16_t kind = *(int16_t *)(sel + 0x120);
            const char *map_name = (const char *)(sel + 0xa0);
            int16_t map_index;

            switch (kind) {
            case 1: r1->background_bitmap_frame = 0; break;
            case 2: r1->background_bitmap_frame = 2; break;
            case 3: r1->background_bitmap_frame = 3; break;
            case 4: r1->background_bitmap_frame = 1; break;
            case 5: r1->background_bitmap_frame = 4; break;
            default: r1->background_bitmap_frame = 5; break;
            }

            if (strstr(map_name, "beavercreek") != 0) map_index = 0;
            else if (strstr(map_name, "sidewinder") != 0) map_index = 1;
            else if (strstr(map_name, "damnation") != 0) map_index = 2;
            else if (strstr(map_name, "ratrace") != 0) map_index = 3;
            else if (strstr(map_name, "prisoner") != 0) map_index = 4;
            else if (strstr(map_name, "hangemhigh") != 0) map_index = 5;
            else if (strstr(map_name, "chillout") != 0) map_index = 6;
            else if (strstr(map_name, "carousel") != 0) map_index = 7;
            else if (strstr(map_name, "boardingaction") != 0) map_index = 8;
            else if (strstr(map_name, "bloodgulch") != 0) map_index = 9;
            else if (strstr(map_name, "wizard") != 0) map_index = 10;
            else if (strstr(map_name, "putput") != 0) map_index = 11;
            else if (strstr(map_name, "longest") != 0) map_index = 0xc;
            else map_index = 0x13;
            r2->background_bitmap_frame = map_index;

            r4->selection_index = (sel[300] != 1) + 0x14;
            r5->selection_index = r2->background_bitmap_frame;

            switch (kind) {
            case 1: r6->selection_index = 3; break;
            case 2: r6->selection_index = 4; break;
            case 3: r6->selection_index = 5; break;
            case 4: r6->selection_index = 6; break;
            case 5: r6->selection_index = 7; break;
            default: r6->selection_index = 8; break;
            }
            r7->selection_index = (sel[0x12e] != 1) + 0xc;

            {
                uint16_t *b = (uint16_t *)heap_reallocate(r8->text, 8, widget_memory_pool);

                r8->text = b;
                if (b != (uint16_t *)0) {
                    string_format_wide_va_bounded(3, (wchar_t *)b, (const wchar_t *)chat_local_prompt_string, // EDX 3 (FIXED: count passed)
                                                   (int32_t)*(uint16_t *)(sel + 0x124));
                    ((uint16_t *)r8->text)[3] = 0;
                }
            }
            {
                uint16_t *b = (uint16_t *)heap_reallocate(r9->text, 8, widget_memory_pool);

                r9->text = b;
                if (b != (uint16_t *)0) {
                    string_format_wide_va_bounded(3, (wchar_t *)b, (const wchar_t *)chat_local_prompt_string, // EDX 3 (FIXED: count passed)
                                                   (int32_t)*(int16_t *)(sel + 0x128));
                    ((uint16_t *)r9->text)[3] = 0;
                }
            }

            switch (kind) {
            case 1: r10->selection_index = 0x16; break;
            case 2: r10->selection_index = 0x18; break;
            case 3: r10->selection_index = 0x18 - (sel[0x12f] != 1); break;
            case 4: r10->selection_index = 0x17; break;
            case 5: r10->selection_index = 0x19; break;
            default: r10->selection_index = 1; break;
            }
            r3->selection_index = 2;
            r3->hidden = 0;

            if (widget->focused_child == (widget_instance *)0) {
                widget->selection_index = 0;
                widget->focused_child = widget->first_child;
                return;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4a5040):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void server_list_menu_update(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  char cVar7;
  wchar_t *_Dest;
  uint uVar8;
  int iVar9;
  undefined2 *puVar10;
  undefined **ppuVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  int *piVar15;
  int iVar16;
  undefined8 uVar17;
  LARGE_INTEGER local_10;
  LARGE_INTEGER local_8;

  iVar13 = DAT_0071c2d8;
  DAT_006b380c = 0;
  DAT_006b3810 = 0;
  DAT_006b3814 = 0;
  _DAT_006b3818 = 0;
  _DAT_006b381c = 0;
  _DAT_006b3820 = 0;
  _DAT_006b3824 = 0;
  DAT_006b3828 = 0;
  DAT_006b382c = 0;
  iVar12 = 0;
  if (DAT_0071c2d8 != 0) {
    iVar14 = DAT_0071c2d8 + 4;
    iVar16 = 9;
    do {
      cVar7 = FUN_004da770();
      if (((cVar7 != '\0') && (*(short *)(iVar14 + 0x12a) == 1)) &&
         (*(char *)(iVar14 + 300) != '\0')) {
        (&DAT_006b380c)[iVar12] = iVar14;
        iVar12 = iVar12 + 1;
      }
      iVar14 = iVar14 + 0x130;
      iVar16 = iVar16 + -1;
    } while (iVar16 != 0);
    piVar15 = (int *)(iVar13 + 0x1c);
    iVar13 = 9;
    do {
      if (*(char *)((int)piVar15 + 0x115) != '\0') {
        QueryPerformanceCounter(&local_10);
        uVar17 = __allmul(local_10.s.LowPart,local_10.s.HighPart,1000,0);
        iVar14 = __alldiv(uVar17,DAT_006ac8f8,DAT_006ac8fc);
        if (((iVar14 - *piVar15 < 0x1771) && (*(short *)((int)piVar15 + 0x112) == 1)) &&
           ((char)piVar15[0x45] == '\0')) {
          (&DAT_006b380c)[iVar12] = piVar15 + -6;
          iVar12 = iVar12 + 1;
        }
      }
      piVar15 = piVar15 + 0x4c;
      iVar13 = iVar13 + -1;
    } while (iVar13 != 0);
    *(undefined4 **)(param_1 + 0x44) = &DAT_006b380c;
    *(short *)(param_1 + 0x48) = (short)iVar12;
    iVar13 = (int)*(short *)(param_1 + 0x40);
    if (iVar12 + -1 < (int)*(short *)(param_1 + 0x40)) {
      iVar13 = iVar12 + -1;
    }
    iVar14 = *(int *)(param_1 + 0x34);
    *(short *)(param_1 + 0x40) = (short)iVar13;
    for (iVar13 = 0; (iVar14 != 0 && (iVar13 < iVar12)); iVar13 = iVar13 + 1) {
      _Dest = (wchar_t *)heap_reallocate(0x20);
      *(wchar_t **)(iVar14 + 0x3c) = _Dest;
      if (_Dest != (wchar_t *)0x0) {
        if (*(char *)((&DAT_006b380c)[iVar13] + 300) == '\x01') {
          _wcsncpy(_Dest,(wchar_t *)((&DAT_006b380c)[iVar13] + 0x1c),0xf);
          *(undefined2 *)(*(int *)(iVar14 + 0x3c) + 0x1e) = 0;
        }
        else {
          uVar8 = tag_lookup("ui\\multiplayer_game_text");
          ppuVar11 = &PTR_DAT_00671fac;
          if ((uVar8 != 0xffffffff) &&
             (piVar15 = *(int **)((uVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 0x13 < *piVar15))
          {
            iVar16 = piVar15[1];
            uVar8 = *(uint *)(iVar16 + 0x17c);
            if (0 < (int)uVar8) {
              ppuVar11 = *(undefined ***)(iVar16 + 0x188);
              *(undefined2 *)((int)ppuVar11 + ((uVar8 & 0xfffffffe) - 2)) = 0;
            }
          }
          string_format_wide_va_bounded
                    (*(undefined4 *)(iVar14 + 0x3c),L"%s %s",ppuVar11,(&DAT_006b380c)[iVar13] + 0x1c
                    );
          *(undefined2 *)(*(int *)(iVar14 + 0x3c) + 0x1e) = 0;
        }
      }
      iVar14 = *(int *)(iVar14 + 0x2c);
    }
    if ((0 < iVar12) && (*(short *)(param_1 + 0x40) < 0)) {
      *(undefined2 *)(param_1 + 0x40) = 0;
    }
    QueryPerformanceCounter(&local_8);
    uVar17 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar9 = __alldiv(uVar17,DAT_006ac8f8,DAT_006ac8fc);
    iVar13 = *(int *)(param_1 + 0x18);
    iVar12 = *(int *)(*(int *)(param_1 + 0x4c) + 0x34);
    iVar14 = *(int *)(iVar12 + 0x2c);
    iVar16 = *(int *)(*(int *)(iVar14 + 0x2c) + 0x2c);
    iVar1 = *(int *)(*(int *)(iVar14 + 0x2c) + 0x34);
    local_10.s.LowPart = *(DWORD *)(iVar1 + 0x2c);
    iVar2 = *(int *)(local_10.s.LowPart + 0x2c);
    iVar3 = *(int *)(iVar2 + 0x2c);
    iVar4 = *(int *)(iVar3 + 0x2c);
    iVar5 = *(int *)(iVar4 + 0x2c);
    iVar6 = *(int *)(iVar5 + 0x2c);
    if (*(short *)(param_1 + 0x40) < 0) {
      *(undefined2 *)(iVar12 + 0x58) = 5;
      *(undefined2 *)(iVar14 + 0x58) = 0x13;
      *(undefined2 *)(iVar1 + 0x40) = 1;
      *(undefined2 *)(local_10.s.LowPart + 0x40) = 0x14;
      *(undefined2 *)(iVar2 + 0x40) = 1;
      *(undefined2 *)(iVar3 + 0x40) = 1;
      puVar10 = (undefined2 *)heap_reallocate(8);
      *(undefined2 **)(iVar4 + 0x3c) = puVar10;
      if (puVar10 != (undefined2 *)0x0) {
        *puVar10 = 0;
      }
      puVar10 = (undefined2 *)heap_reallocate(8);
      *(undefined2 **)(iVar5 + 0x3c) = puVar10;
      if (puVar10 != (undefined2 *)0x0) {
        *puVar10 = 0;
      }
      *(undefined2 *)(iVar6 + 0x40) = 1;
      *(ushort *)(iVar16 + 0x40) = (ushort)(999 < (uint)(iVar9 - iVar13));
      *(undefined1 *)(iVar16 + 0x10) = 1;
    }
    else {
      iVar13 = (&DAT_006b380c)[*(short *)(param_1 + 0x40)];
      switch(*(undefined2 *)(iVar13 + 0x120)) {
      case 1:
        *(undefined2 *)(iVar12 + 0x58) = 0;
        break;
      case 2:
        *(undefined2 *)(iVar12 + 0x58) = 2;
        break;
      case 3:
        *(undefined2 *)(iVar12 + 0x58) = 3;
        break;
      case 4:
        *(undefined2 *)(iVar12 + 0x58) = 1;
        break;
      case 5:
        *(undefined2 *)(iVar12 + 0x58) = 4;
        break;
      default:
        *(undefined2 *)(iVar12 + 0x58) = 5;
      }
      iVar12 = iVar13 + 0xa0;
      iVar9 = FUN_00625430(iVar12,"beavercreek");
      if (iVar9 == 0) {
        iVar9 = FUN_00625430(iVar12,"sidewinder");
        if (iVar9 == 0) {
          iVar9 = FUN_00625430(iVar12,"damnation");
          if (iVar9 == 0) {
            iVar9 = FUN_00625430(iVar12,"ratrace");
            if (iVar9 == 0) {
              iVar9 = FUN_00625430(iVar12,"prisoner");
              if (iVar9 == 0) {
                iVar9 = FUN_00625430(iVar12,"hangemhigh");
                if (iVar9 == 0) {
                  iVar9 = FUN_00625430(iVar12,"chillout");
                  if (iVar9 == 0) {
                    iVar9 = FUN_00625430(iVar12,"carousel");
                    if (iVar9 == 0) {
                      iVar9 = FUN_00625430(iVar12,"boardingaction");
                      if (iVar9 == 0) {
                        iVar9 = FUN_00625430(iVar12,"bloodgulch");
                        if (iVar9 == 0) {
                          iVar9 = FUN_00625430(iVar12,"wizard");
                          if (iVar9 == 0) {
                            iVar9 = FUN_00625430(iVar12,"putput");
                            if (iVar9 == 0) {
                              iVar12 = FUN_00625430(iVar12,"longest");
                              if (iVar12 == 0) {
                                *(undefined2 *)(iVar14 + 0x58) = 0x13;
                              }
                              else {
                                *(undefined2 *)(iVar14 + 0x58) = 0xc;
                              }
                            }
                            else {
                              *(undefined2 *)(iVar14 + 0x58) = 0xb;
                            }
                          }
                          else {
                            *(undefined2 *)(iVar14 + 0x58) = 10;
                          }
                        }
                        else {
                          *(undefined2 *)(iVar14 + 0x58) = 9;
                        }
                      }
                      else {
                        *(undefined2 *)(iVar14 + 0x58) = 8;
                      }
                    }
                    else {
                      *(undefined2 *)(iVar14 + 0x58) = 7;
                    }
                  }
                  else {
                    *(undefined2 *)(iVar14 + 0x58) = 6;
                  }
                }
                else {
                  *(undefined2 *)(iVar14 + 0x58) = 5;
                }
              }
              else {
                *(undefined2 *)(iVar14 + 0x58) = 4;
              }
            }
            else {
              *(undefined2 *)(iVar14 + 0x58) = 3;
            }
          }
          else {
            *(undefined2 *)(iVar14 + 0x58) = 2;
          }
        }
        else {
          *(undefined2 *)(iVar14 + 0x58) = 1;
        }
      }
      else {
        *(undefined2 *)(iVar14 + 0x58) = 0;
      }
      *(ushort *)(iVar1 + 0x40) = (*(char *)(iVar13 + 300) != '\x01') + 0x14;
      *(undefined2 *)(local_10.s.LowPart + 0x40) = *(undefined2 *)(iVar14 + 0x58);
      switch(*(undefined2 *)(iVar13 + 0x120)) {
      case 1:
        *(undefined2 *)(iVar2 + 0x40) = 3;
        break;
      case 2:
        *(undefined2 *)(iVar2 + 0x40) = 4;
        break;
      case 3:
        *(undefined2 *)(iVar2 + 0x40) = 5;
        break;
      case 4:
        *(undefined2 *)(iVar2 + 0x40) = 6;
        break;
      case 5:
        *(undefined2 *)(iVar2 + 0x40) = 7;
        break;
      default:
        *(undefined2 *)(iVar2 + 0x40) = 8;
      }
      *(ushort *)(iVar3 + 0x40) = (*(char *)(iVar13 + 0x12e) != '\x01') + 0xc;
      iVar12 = heap_reallocate(8);
      *(int *)(iVar4 + 0x3c) = iVar12;
      if (iVar12 != 0) {
        string_format_wide_va_bounded
                  (iVar12,&PTR_s_parameter_handles_0063fff0_0x35_006607a0,
                   *(undefined2 *)(iVar13 + 0x124));
        *(undefined2 *)(*(int *)(iVar4 + 0x3c) + 6) = 0;
      }
      iVar12 = heap_reallocate(8);
      *(int *)(iVar5 + 0x3c) = iVar12;
      if (iVar12 != 0) {
        string_format_wide_va_bounded
                  (iVar12,&PTR_s_parameter_handles_0063fff0_0x35_006607a0,
                   (int)*(short *)(iVar13 + 0x128));
        *(undefined2 *)(*(int *)(iVar5 + 0x3c) + 6) = 0;
      }
      switch(*(undefined2 *)(iVar13 + 0x120)) {
      case 1:
        *(undefined2 *)(iVar6 + 0x40) = 0x16;
        break;
      case 2:
        *(undefined2 *)(iVar6 + 0x40) = 0x18;
        break;
      case 3:
        *(ushort *)(iVar6 + 0x40) = 0x18 - (ushort)(*(char *)(iVar13 + 0x12f) != '\x01');
        break;
      case 4:
        *(undefined2 *)(iVar6 + 0x40) = 0x17;
        break;
      case 5:
        *(undefined2 *)(iVar6 + 0x40) = 0x19;
        break;
      default:
        *(undefined2 *)(iVar6 + 0x40) = 1;
      }
      *(undefined2 *)(iVar16 + 0x40) = 2;
      *(undefined1 *)(iVar16 + 0x10) = 0;
      if (*(int *)(param_1 + 0x38) == 0) {
        *(undefined2 *)(param_1 + 0x40) = 0;
        *(undefined4 *)(param_1 + 0x38) = *(undefined4 *)(param_1 + 0x34);
        return;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
