// video_options_menu_update  (Ghidra: already named)
// address 0x4bb640, size 410 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4bb640..0x4bb7d9 in the phase-4 review. The first
// rewrite swapped the two spinners, dropped the heap_reallocate register arguments and the
// EDX destination of the formatter, added bounds checks the binary does not have, walked to
// the gamma row from the wrong widget and played sound 0 instead of 4.
//   Screen layout: the first child holds the resolution spinner (second child of its first
// child); its next sibling (row_a) holds the refresh rate spinner (second child of its first
// child); the gamma control is the first child of type 2 under the seventh sibling after
// row_a. Each spinner becomes the focused child of its parent. The resolution spinner text
// (list_render_data, 0x20 bytes) is the entry name, or L"" (0x00660c34) out of range, 0xf
// characters terminated at +0x1e. The refresh spinner item count is the refresh rate count of
// the selected resolution and its selection is clamped to count - 1 (unsigned compare); its
// text is L"%d Hz" (0x0066b158) of that rate. Neither read of the resolution entry is bounds
// checked (kept from the binary). The gamma control +0x54 (-1 left, 1 right) moves the gamma
// (0x00695464) by 5, clamped to 1..0xfe with sound 4 at the limit; it is copied to 0x0071d1e0
// and applied (0x5227a0), then 0x4a66b0 runs on the screen. Returns 1.
// register convention: plain cdecl, one stack argument (the screen widget).

#include <wchar.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern heap *widget_memory_pool;                  // 0x006926c4
extern video_resolution video_resolutions[0x20]; // 0x006b6690
extern int32_t video_resolution_count;           // 0x007196cc
extern int32_t video_gamma_setting; // 0x00695464 (see video_options_menu_populate.c)
extern int32_t rasterizer_gamma; // 0x0071d1e0

extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern uint16_t *string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern void chimera__gamma(void); // 0x5227a0
extern void widget_extended_description_sync_selection(widget_instance *screen); // 0x4a66b0, cdecl

uint8_t video_options_menu_update(widget_instance *screen)
{
    static const uint16_t empty_text[1] = {0};                             // 0x00660c34
    static const uint16_t hz_format[6] = {'%', 'd', ' ', 'H', 'z', 0};   // 0x0066b158
    widget_instance *row_a = screen->first_child->next_sibling;
    widget_instance *resolution = screen->first_child->first_child->next_sibling;
    widget_instance *refresh = row_a->first_child->next_sibling;
    widget_instance *gamma;
    int32_t resolution_index;
    int32_t refresh_index;
    uint16_t *text;
    int32_t i;

    resolution->parent->focused_child = resolution;
    refresh->parent->focused_child = refresh;

    resolution_index = resolution->selection_index;
    text = (uint16_t *)heap_reallocate(resolution->list_render_data, 0x20, widget_memory_pool);
    resolution->list_render_data = text;
    if (text != 0) {
        wcsncpy((wchar_t *)text,
                (const wchar_t *)(resolution_index >= 0 && resolution_index < video_resolution_count
                                      ? video_resolutions[resolution_index].name
                                      : empty_text),
                0xf);
        ((uint16_t *)resolution->list_render_data)[0xf] = 0;
    }

    refresh_index = refresh->selection_index;
    refresh->item_count = (uint16_t)video_resolutions[resolution_index].refresh_rate_count;
    if ((uint32_t)refresh_index >= video_resolutions[resolution_index].refresh_rate_count) {
        refresh->selection_index = (int16_t)(video_resolutions[resolution_index].refresh_rate_count - 1);
        refresh_index = refresh->selection_index;
    }
    text = (uint16_t *)heap_reallocate(refresh->list_render_data, 0x20, widget_memory_pool);
    refresh->list_render_data = text;
    if (text != 0) {
        string_format_wide_va(text, hz_format, video_resolutions[resolution_index].refresh_rates[refresh_index]);
        ((uint16_t *)refresh->list_render_data)[0xf] = 0;
    }

    gamma = row_a;
    for (i = 0; i < 7; i++) {
        gamma = gamma->next_sibling;
    }
    gamma = gamma->first_child;
    while (gamma != 0 && gamma->widget_type != 2) {
        gamma = gamma->next_sibling;
    }
    if (gamma->unknown_54 == -1) {
        video_gamma_setting -= 5;
        if (video_gamma_setting < 1) {
            widget_play_sound_effect(4);
            video_gamma_setting = 1;
        }
    } else if (gamma->unknown_54 == 1) {
        video_gamma_setting += 5;
        if (video_gamma_setting > 0xfe) {
            widget_play_sound_effect(4);
            video_gamma_setting = 0xfe;
        }
    }
    rasterizer_gamma = video_gamma_setting;
    chimera__gamma();
    widget_extended_description_sync_selection(screen);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4bb640):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 video_options_menu_update(int param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;
  wchar_t *_Dest;
  int iVar4;
  wchar_t *_Source;
  int iVar5;
  uint uVar6;

  iVar1 = *(int *)(*(int *)(param_1 + 0x34) + 0x2c);
  iVar4 = *(int *)(*(int *)(*(int *)(param_1 + 0x34) + 0x34) + 0x2c);
  iVar2 = *(int *)(iVar1 + 0x34);
  *(int *)(*(int *)(iVar4 + 0x30) + 0x38) = iVar4;
  iVar2 = *(int *)(iVar2 + 0x2c);
  *(int *)(*(int *)(iVar2 + 0x30) + 0x38) = iVar2;
  iVar5 = (int)*(short *)(iVar4 + 0x40);
  _Dest = (wchar_t *)heap_reallocate(0x20);
  *(wchar_t **)(iVar4 + 0x50) = _Dest;
  if (_Dest != (wchar_t *)0x0) {
    if ((iVar5 < 0) || (DAT_007196cc <= iVar5)) {
      _Source = L"";
    }
    else {
      _Source = (wchar_t *)(&DAT_006b6698 + iVar5 * 0x4c);
    }
    _wcsncpy(_Dest,_Source,0xf);
    *(undefined2 *)(*(int *)(iVar4 + 0x50) + 0x1e) = 0;
  }
  uVar6 = (uint)*(short *)(iVar2 + 0x40);
  *(undefined2 *)(iVar2 + 0x48) = *(undefined2 *)(&DAT_006b66b8 + iVar5 * 0x13);
  if ((uint)(&DAT_006b66b8)[iVar5 * 0x13] <= uVar6) {
    sVar3 = (short)(&DAT_006b66b8)[iVar5 * 0x13] + -1;
    *(short *)(iVar2 + 0x40) = sVar3;
    uVar6 = (uint)sVar3;
  }
  iVar4 = heap_reallocate(0x20);
  *(int *)(iVar2 + 0x50) = iVar4;
  if (iVar4 != 0) {
    string_format_wide_va(L"%d Hz",*(undefined4 *)(&DAT_006b66bc + (iVar5 * 0x13 + uVar6) * 2));
    *(undefined2 *)(*(int *)(iVar2 + 0x50) + 0x1e) = 0;
  }
  for (iVar1 = *(int *)(*(int *)(*(int *)(*(int *)(*(int *)(*(int *)(*(int *)(*(int *)(iVar1 + 0x2c)
                                                                             + 0x2c) + 0x2c) + 0x2c)
                                                  + 0x2c) + 0x2c) + 0x2c) + 0x34);
      (iVar1 != 0 && (*(short *)(iVar1 + 0xe) != 2)); iVar1 = *(int *)(iVar1 + 0x2c)) {
  }
  if (*(short *)(iVar1 + 0x54) == -1) {
    _DAT_00695464 = _DAT_00695464 + -5;
    if (_DAT_00695464 < 1) {
      widget_play_sound_effect();
      _DAT_00695464 = 1;
    }
  }
  else if ((*(short *)(iVar1 + 0x54) == 1) &&
          (_DAT_00695464 = _DAT_00695464 + 5, 0xfe < _DAT_00695464)) {
    widget_play_sound_effect();
    _DAT_00695464 = 0xfe;
  }
  _DAT_0071d1e0 = _DAT_00695464;
  chimera__gamma();
  FUN_004a66b0(param_1);
  return 1;
}
#endif
