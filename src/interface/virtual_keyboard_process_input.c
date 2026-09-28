// virtual_keyboard_process_input  (Ghidra: virtual_keyboard_process_input, already named)
// address 0x4a8be0, size 1498 bytes
// name confidence: 0.5 (existing Ghidra name)   rewrite confidence: 0.65
// evidence: types/interface.h virtual_keyboard_globals; the queued key ring (int16 read and
// write indices at 0x006b16fa / 0x006b16fc, 4 byte ui_key_event records from 0x006b16fe) is the
// one console_process_queued_input @0x4965e0 drains.
// Phase-4 review against objdump 0x4a8be0..0x4a91c0 and the two-level jump table (byte map at
// 0x4a91e8, targets at 0x4a91c4): key 0 closes, 0x1d backspace, 0x38 and 0x66 commit, 0x4f caret
// left, 0x50 caret right, 0x52 home, 0x54 delete, 0x55 end, anything else inserts its character.
// Fixed here: the commit branches and the character filter read virtual_keyboard.validation_mode
// (the dword at 0x00719410), not field_kind; widget_play_sound_effect gets its one-based id in
// EAX (1 edit, 3 commit, 4 rejected character); saved_game_name_is_available takes the destination in EAX and
// ui_variant_name_is_available in EDI; the ring indices are int16; the DirectInput scratch buffers
// are 0x6d bytes; the ".fortune" easter egg (0x0066a8b4) takes the clock modulo 10 unsigned.
// UNSURE: the character map walk (small_ui font tag +0x34 as a pointer to 12 byte records whose
// first is {count, pointer to 0x100 int16 glyph indices}, glyph records of 0x14 bytes at
// +0x80) dereferences NULL when the count is not 0x100, exactly as the binary does. The
// no-break-space test compares the zero-extended byte with 0xffffffa0 and can never match; kept.
// register convention: __cdecl, no parameters.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include <wchar.h>
#include <string.h>

extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8
extern tag_instance *tag_instances;               // 0x0087bc14
extern uint8_t controls_input_capture_flags; // 0x00712542, bit 2 set while a keyboard owns input
extern int16_t queued_key_event_read_index;       // 0x006b16fa
extern int16_t queued_key_event_write_index;      // 0x006b16fc
extern ui_key_event queued_key_events[];          // 0x006b16fe

extern void **directinput_keyboard_device;        // 0x006b1800, DirectInput device COM pointer
extern uint8_t directinput_unknown_buffer_1[0x6d]; // 0x006b1620 (0x1b dwords + 1 byte cleared)
extern uint8_t directinput_unknown_buffer_2[0x6d]; // 0x006b168d (0x1b dwords + 1 byte cleared)


extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern void display_error(int32_t string_index, int32_t player_index, uint8_t modal, uint8_t is_error); // 0x498f20
extern uint8_t virtual_keyboard_close(void);     // 0x4a9250
extern void virtual_keyboard_backspace(void);    // 0x4a96f0
extern uint8_t ui_wide_string_has_non_whitespace(const uint16_t *text); // 0x4a8b10, blam-cc: EAX
extern uint8_t ui_variant_name_is_available(const uint16_t *name); // 0x4a8b50, blam-cc: EDI
extern uint8_t virtual_keyboard_character_is_legal(int32_t validation_mode, uint8_t character); // 0x4a8b80, blam-cc: EAX mode, CL character
extern uint32_t time_query_performance_counter_ms(void);              // 0x449210, millisecond clock
extern uint8_t saved_game_name_is_available(const uint16_t *name); // 0x53d1e0, blam-cc: EAX; profile module name test
extern uint8_t saved_item_name_matches(const uint16_t *text); // 0x495e70
extern uint16_t fortune_easter_egg_text[];         // 0x0066a8b4, L".fortune"

#define WCTYPE_SPACE 0x0008

// memset of the whole destination (maximum_length bytes, zero-extended) and caret to its start.
static void vk_clear_text(void)
{
    memset(virtual_keyboard.destination, 0, (uint16_t)virtual_keyboard.maximum_length);
    virtual_keyboard.destination_end = virtual_keyboard.destination;
}

// Trims trailing whitespace. Returns 0 when nothing but whitespace was left.
static uint8_t vk_trim_trailing_whitespace(void)
{
    int32_t i = wcslen(virtual_keyboard.destination) - 1;
    while (i >= 0) {
        if (iswctype(virtual_keyboard.destination[i], WCTYPE_SPACE) == 0) {
            return 1;
        }
        virtual_keyboard.destination[i] = 0;
        i--;
    }
    return 0;
}

void virtual_keyboard_process_input(void)
{
    for (;;) {
        uint8_t mode_flags = controls_input_capture_flags;
        ui_key_event event;

        if (mode_flags == 1 || (mode_flags & 8) != 0 || (mode_flags & 4) == 0 ||
            queued_key_event_read_index >= queued_key_event_write_index) {
            return;
        }
        event = queued_key_events[queued_key_event_read_index];
        queued_key_event_read_index++;

        switch (event.key_code) {
        case 0x00:
            virtual_keyboard_close();
            continue;

        case 0x1d: // backspace
            if (virtual_keyboard.opened != 1) {
                virtual_keyboard_backspace();
                continue;
            }
            vk_clear_text();
            break;

        case 0x38:
        case 0x66: { // commit
            uint8_t name_ok;

            if (wcscmp((const wchar_t *)virtual_keyboard.text, (const wchar_t *)virtual_keyboard.destination) == 0) {
                goto commit_ok;
            }
            switch (virtual_keyboard.validation_mode) {
            case 1:
                if (!ui_wide_string_has_non_whitespace(virtual_keyboard.destination) ||
                    !vk_trim_trailing_whitespace()) {
                    goto invalid;
                }
                if (saved_game_name_is_available(virtual_keyboard.destination)) {
                    goto commit_ok;
                }
                name_ok = saved_item_name_matches(virtual_keyboard.destination);
                break;
            case 2:
                if (!ui_wide_string_has_non_whitespace(virtual_keyboard.destination) ||
                    !vk_trim_trailing_whitespace()) {
                    goto invalid;
                }
                if (saved_item_name_matches(virtual_keyboard.destination)) {
                    goto commit_ok;
                }
                if (!saved_game_name_is_available(virtual_keyboard.destination)) {
                    goto name_taken;
                }
                name_ok = ui_variant_name_is_available(virtual_keyboard.destination);
                break;
            case 3:
                if (virtual_keyboard.destination[0] != 0) {
                    goto commit_ok;
                }
                wcslen(virtual_keyboard.text);
                wcscpy((wchar_t *)virtual_keyboard.destination, (const wchar_t *)virtual_keyboard.text);
                virtual_keyboard_close();
                goto finish;
            default:
                goto finish;
            }
            if (name_ok) {
                goto commit_ok;
            }
name_taken:
            display_error(0x1b, -1, 1, 0);
            virtual_keyboard_close();
            goto finish;
invalid:
            display_error(0x1d, -1, 1, 0);
            virtual_keyboard_close();
            goto finish;
commit_ok:
            virtual_keyboard.committed = 1;
finish:
            widget_play_sound_effect(3);
            controls_input_capture_flags &= 0xfb;
            virtual_keyboard.active = 0;
            if (directinput_keyboard_device != 0) {
                int32_t minus_one = -1;
                void **vtable = *(void ***)directinput_keyboard_device;
                ((directinput_set_property_fn)vtable[0x28 / 4])(directinput_keyboard_device, 0x14, 0, &minus_one, 0);
                memset(directinput_unknown_buffer_2, 0, sizeof(directinput_unknown_buffer_2));
                memset(directinput_unknown_buffer_1, 0, sizeof(directinput_unknown_buffer_1));
            }
            continue;
        }

        case 0x4f: // caret left
            if (virtual_keyboard.destination_end > virtual_keyboard.destination) {
                virtual_keyboard.destination_end--;
            }
            break;

        case 0x50: // caret right
            if (*virtual_keyboard.destination_end != 0) {
                virtual_keyboard.destination_end++;
            }
            break;

        case 0x52: // home
            virtual_keyboard.destination_end = virtual_keyboard.destination;
            break;

        case 0x55: // end
            virtual_keyboard.destination_end =
                virtual_keyboard.destination + wcslen(virtual_keyboard.destination);
            break;

        case 0x54: // delete
            if (virtual_keyboard.opened == 1) {
                vk_clear_text();
                break;
            }
            if (*virtual_keyboard.destination_end != 0) {
                int32_t bytes = (int32_t)(uint16_t)virtual_keyboard.maximum_length -
                                (int32_t)((uint8_t *)virtual_keyboard.destination_end -
                                          (uint8_t *)virtual_keyboard.destination) - 1;
                if (bytes > 0) {
                    memmove(virtual_keyboard.destination_end, virtual_keyboard.destination_end + 1, bytes);
                    virtual_keyboard.destination[((uint16_t)virtual_keyboard.maximum_length >> 1) - 1] = 0;
                    widget_play_sound_effect(1);
                }
            }
            continue;

        default: { // insert a character
            uint8_t ch = event.character;
            uint8_t *font;
            int32_t *character_map;
            int16_t *glyph;

            if (ch < 0x20 || ch == 0xff) {
                continue;
            }
            font = (uint8_t *)tag_instances[virtual_keyboard.small_ui_tag & 0xffff].data;
            character_map = *(int32_t **)(font + 0x34) + (ch >> 8) * 3; // high byte is always 0
            if (character_map[0] <= 0) {
                goto rejected;
            }
            glyph = (character_map[0] == 0x100) ? (int16_t *)(uintptr_t)character_map[1] + ch : (int16_t *)0;
            if (*glyph == -1 || *(int32_t *)(font + 0x80) + *glyph * 0x14 == 0 ||
                !virtual_keyboard_character_is_legal(virtual_keyboard.validation_mode, ch)) {
                goto rejected;
            }
            if (virtual_keyboard.validation_mode == 3 &&
                virtual_keyboard.destination_end == virtual_keyboard.destination &&
                (ch == 0x20 || (uint32_t)ch == 0xffffffa0u)) {
                goto rejected;
            }
            if (virtual_keyboard.opened == 1) {
                vk_clear_text();
                virtual_keyboard.opened = 0;
            }
            if ((int32_t)(uint16_t)virtual_keyboard.maximum_length -
                    (wcslen(virtual_keyboard.destination) * 2 + 2) < 2) {
                goto rejected;
            }
            memmove(virtual_keyboard.destination_end + 1, virtual_keyboard.destination_end,
                    (int32_t)(uint16_t)virtual_keyboard.maximum_length -
                        (int32_t)((uint8_t *)virtual_keyboard.destination_end -
                                  (uint8_t *)virtual_keyboard.destination) - 2);
            *virtual_keyboard.destination_end = ch;
            virtual_keyboard.destination_end++;

            if (wcscmp((const wchar_t *)virtual_keyboard.destination, (const wchar_t *)fortune_easter_egg_text) != 0) {
                widget_play_sound_effect(1);
                continue;
            }
            {
                uint32_t pick = time_query_performance_counter_ms() % 10;
                if (pick > 9) {
                    pick = 9;
                }
                virtual_keyboard.field_kind = (int16_t)(pick + 0xb);
            }
            if (virtual_keyboard.text[0] != 0) {
                wcslen(virtual_keyboard.text);
                wcscpy((wchar_t *)virtual_keyboard.destination, (const wchar_t *)virtual_keyboard.text);
                virtual_keyboard.destination_end =
                    virtual_keyboard.destination + wcslen(virtual_keyboard.destination);
            } else {
                vk_clear_text();
            }
            widget_play_sound_effect(1);
            continue;
rejected:
            widget_play_sound_effect(4);
            continue;
        }
        }

        virtual_keyboard.opened = 0;
        widget_play_sound_effect(1);
    }
}

#if 0
Original Ghidra decompilation (0x4a8be0):

void __cdecl virtual_keyboard_process_input(void)

{
  size_t _Size;
  int *piVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  short *psVar5;
  uint uVar6;
  uint uVar7;
  short sVar8;
  byte bVar9;
  undefined4 *puVar10;
  wchar_t *pwVar11;
  undefined4 local_4;

LAB_004a8bf0:
  if ((((DAT_00712542 == 1) || ((DAT_00712542 & 8) != 0)) || ((DAT_00712542 & 4) == 0)) ||
     (DAT_006b16fc <= DAT_006b16fa)) {
    return;
  }
  iVar4 = (int)DAT_006b16fa;
  DAT_006b16fa = DAT_006b16fa + 1;
  switch((int)(&DAT_006b16fe)[iVar4] >> 0x10) {
  case 0:
    virtual_keyboard_close();
    goto LAB_004a8bf0;
  default:
    bVar9 = (byte)((uint)(&DAT_006b16fe)[iVar4] >> 8);
    if ((0x1f < bVar9) && (bVar9 != 0xff)) {
      iVar4 = *(int *)((DAT_00719418 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      piVar1 = *(int **)(iVar4 + 0x34);
      iVar3 = *piVar1;
      if (iVar3 < 1) {
LAB_004a91aa:
        widget_play_sound_effect();
      }
      else {
        if (iVar3 == 0x100) {
          psVar5 = (short *)(piVar1[1] + (uint)bVar9 * 2);
        }
        else {
          psVar5 = (short *)0x0;
        }
        if ((((*psVar5 == -1) || (*(int *)(iVar4 + 0x80) + *psVar5 * 0x14 == 0)) ||
            (cVar2 = FUN_004a8b80(), cVar2 == '\0')) ||
           (((DAT_00719410 == 3 && (DAT_007193c4 == DAT_007193c0)) &&
            ((bVar9 == 0x20 || (bVar9 == 0xffffffa0)))))) goto LAB_004a91aa;
        if (DAT_007193bc._3_1_ == '\x01') {
          uVar6 = (uint)DAT_007193b4;
          pwVar11 = DAT_007193c0;
          for (uVar7 = (uint)(DAT_007193b4 >> 2); uVar7 != 0; uVar7 = uVar7 - 1) {
            pwVar11[0] = L'\0';
            pwVar11[1] = L'\0';
            pwVar11 = pwVar11 + 2;
          }
          for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined1 *)pwVar11 = 0;
            pwVar11 = (wchar_t *)((int)pwVar11 + 1);
          }
          DAT_007193c4 = DAT_007193c0;
          DAT_007193bc._3_1_ = '\0';
        }
        iVar4 = FUN_00625b7a(DAT_007193c0);
        if ((int)((uint)DAT_007193b4 - (iVar4 * 2 + 2)) < 2) goto LAB_004a91aa;
        _memmove(DAT_007193c4 + 1,DAT_007193c4,
                 (size_t)(((uint)DAT_007193b4 - (int)DAT_007193c4) + -2 + (int)DAT_007193c0));
        *DAT_007193c4 = (ushort)bVar9;
        DAT_007193c4 = DAT_007193c4 + 1;
        iVar4 = _wcscmp(DAT_007193c0,(wchar_t *)&PTR_PTR_0066a8b4);
        if (iVar4 != 0) goto LAB_004a8e82;
        uVar7 = FUN_00449210();
        pwVar11 = DAT_007193c0;
        sVar8 = (short)(uVar7 % 10);
        if (9 < uVar7 % 10) {
          sVar8 = 9;
        }
        DAT_007193bc._0_2_ = sVar8 + 0xb;
        if (DAT_007193d0 == 0) {
          uVar6 = (uint)DAT_007193b4;
          for (uVar7 = (uint)(DAT_007193b4 >> 2); uVar7 != 0; uVar7 = uVar7 - 1) {
            pwVar11[0] = L'\0';
            pwVar11[1] = L'\0';
            pwVar11 = pwVar11 + 2;
          }
          for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined1 *)pwVar11 = 0;
            pwVar11 = (wchar_t *)((int)pwVar11 + 1);
          }
          DAT_007193c4 = DAT_007193c0;
          widget_play_sound_effect();
        }
        else {
          FUN_00625b7a(&DAT_007193d0);
          _wcscpy(pwVar11,&DAT_007193d0);
          iVar4 = FUN_00625b7a(DAT_007193c0);
          DAT_007193c4 = DAT_007193c0 + iVar4;
          widget_play_sound_effect();
        }
      }
    }
    goto LAB_004a8bf0;
  case 0x1d:
    if (DAT_007193bc._3_1_ != '\x01') {
      virtual_keyboard_backspace();
      goto LAB_004a8bf0;
    }
    uVar6 = (uint)DAT_007193b4;
    pwVar11 = DAT_007193c0;
    for (uVar7 = (uint)(DAT_007193b4 >> 2); uVar7 != 0; uVar7 = uVar7 - 1) {
      pwVar11[0] = L'\0';
      pwVar11[1] = L'\0';
      pwVar11 = pwVar11 + 2;
    }
    for (uVar6 = uVar6 & 3; DAT_007193c4 = DAT_007193c0, uVar6 != 0; uVar6 = uVar6 - 1) {
      *(undefined1 *)pwVar11 = 0;
      pwVar11 = (wchar_t *)((int)pwVar11 + 1);
    }
    break;
  case 0x38:
  case 0x66:
    iVar4 = _wcscmp(&DAT_007193d0,DAT_007193c0);
    pwVar11 = DAT_007193c0;
    if (iVar4 == 0) {
LAB_004a8dec:
      DAT_007193bc._2_1_ = 1;
    }
    else {
      if (DAT_00719410 == 1) {
        cVar2 = FUN_004a8b10();
        if (cVar2 != '\0') {
          iVar4 = FUN_00625b7a(DAT_007193c0);
          iVar4 = iVar4 + -1;
          if (-1 < iVar4) {
LAB_004a8ca0:
            iVar3 = _iswctype(DAT_007193c0[iVar4],8);
            if (iVar3 != 0) goto code_r0x004a8cbb;
            if (-1 < iVar4) {
              cVar2 = FUN_0053d1e0();
              if (cVar2 == '\0') {
                cVar2 = FUN_00495e70(DAT_007193c0);
LAB_004a8da1:
                if (cVar2 != '\0') goto LAB_004a8dec;
LAB_004a8da5:
                display_error(0x1b,-1,'\x01','\0');
                virtual_keyboard_close();
                goto LAB_004a8df3;
              }
              goto LAB_004a8dec;
            }
          }
        }
      }
      else {
        if (DAT_00719410 != 2) {
          if (DAT_00719410 == 3) {
            if (*DAT_007193c0 != L'\0') goto LAB_004a8dec;
            FUN_00625b7a(&DAT_007193d0);
            _wcscpy(pwVar11,&DAT_007193d0);
            virtual_keyboard_close();
          }
          goto LAB_004a8df3;
        }
        cVar2 = FUN_004a8b10();
        if (cVar2 != '\0') {
          iVar4 = FUN_00625b7a(DAT_007193c0);
          while (iVar4 = iVar4 + -1, -1 < iVar4) {
            iVar3 = _iswctype(DAT_007193c0[iVar4],8);
            if (iVar3 == 0) {
              if (-1 < iVar4) {
                cVar2 = FUN_00495e70(DAT_007193c0);
                if (cVar2 != '\0') goto LAB_004a8dec;
                cVar2 = FUN_0053d1e0();
                if (cVar2 == '\0') goto LAB_004a8da5;
                cVar2 = FUN_004a8b50();
                goto LAB_004a8da1;
              }
              break;
            }
            DAT_007193c0[iVar4] = L'\0';
          }
        }
      }
LAB_004a8cca:
      display_error(0x1d,-1,'\x01','\0');
      virtual_keyboard_close();
    }
LAB_004a8df3:
    widget_play_sound_effect();
    DAT_00712542 = DAT_00712542 & 0xfb;
    DAT_007193a8 = 0;
    if (DAT_006b1800 != (int *)0x0) {
      local_4 = 0xffffffff;
      (**(code **)(*DAT_006b1800 + 0x28))(DAT_006b1800,0x14,0,&local_4,0);
      puVar10 = &DAT_006b168d;
      for (iVar4 = 0x1b; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar10 = 0;
        puVar10 = puVar10 + 1;
      }
      *(undefined1 *)puVar10 = 0;
      puVar10 = (undefined4 *)&DAT_006b1620;
      for (iVar4 = 0x1b; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar10 = 0;
        puVar10 = puVar10 + 1;
      }
      *(undefined1 *)puVar10 = 0;
    }
    goto LAB_004a8bf0;
  case 0x4f:
    if (DAT_007193c0 < DAT_007193c4) {
      DAT_007193c4 = DAT_007193c4 + -1;
    }
    break;
  case 0x50:
    if (*DAT_007193c4 != L'\0') {
      DAT_007193c4 = DAT_007193c4 + 1;
    }
    break;
  case 0x52:
    DAT_007193c4 = DAT_007193c0;
    break;
  case 0x54:
    if (DAT_007193bc._3_1_ == '\x01') {
      uVar6 = (uint)DAT_007193b4;
      pwVar11 = DAT_007193c0;
      for (uVar7 = (uint)(DAT_007193b4 >> 2); uVar7 != 0; uVar7 = uVar7 - 1) {
        pwVar11[0] = L'\0';
        pwVar11[1] = L'\0';
        pwVar11 = pwVar11 + 2;
      }
      for (uVar6 = uVar6 & 3; DAT_007193c4 = DAT_007193c0, uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined1 *)pwVar11 = 0;
        pwVar11 = (wchar_t *)((int)pwVar11 + 1);
      }
      break;
    }
    if ((*DAT_007193c4 != L'\0') &&
       (_Size = ((uint)DAT_007193b4 - (int)DAT_007193c4) + -1 + (int)DAT_007193c0, 0 < (int)_Size))
    {
      _memmove(DAT_007193c4,DAT_007193c4 + 1,_Size);
      *(undefined2 *)((int)DAT_007193c0 + ((DAT_007193b4 & 0xfffffffe) - 2)) = 0;
      widget_play_sound_effect();
    }
    goto LAB_004a8bf0;
  case 0x55:
    iVar4 = FUN_00625b7a(DAT_007193c0);
    DAT_007193c4 = DAT_007193c0 + iVar4;
  }
  DAT_007193bc._3_1_ = '\0';
LAB_004a8e82:
  widget_play_sound_effect();
  goto LAB_004a8bf0;
code_r0x004a8cbb:
  iVar3 = iVar4 + -1;
  DAT_007193c0[iVar4] = L'\0';
  iVar4 = iVar3;
  if (iVar3 < 0) goto LAB_004a8cca;
  goto LAB_004a8ca0;
}
#endif
