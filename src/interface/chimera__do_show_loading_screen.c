// chimera__do_show_loading_screen  (Ghidra: chimera__do_show_loading_screen, already named)
// address 0x497410, size 1087 bytes; real extent 0x497410..0x49784e (Ghidra split it at 0x4974f0, see below)
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md summary: "Advances the timer-driven state machine
// for the loading/saving/connecting progress screen each frame."; the three tag paths are the
// literals at 0x669b44 (font ui\large_ui), 0x669b28 (bitmap ui\shell\bitmaps\background) and
// 0x669b0c (unicode_string_list ui\shell\strings\loading).
// register convention: no register-passed arguments.
// Review pass (phase 4): rebuilt from the disassembly. The catalogued "interface_draw_screen"
// at 0x4974f0 is not a function: 0x4974f0 is the `je 0x497533` instruction inside this body's
// cleanup test, it has no callers, and this function's epilogue at 0x497844 is shared. Its
// separate file was folded into this one. Corrections against the first rewrite:
//  - 0x68e680 is a fade-out end time in milliseconds (0x449210 is the performance-counter
//    millisecond clock). While it is set the screen draws with alpha (end - now) * 0.0013333;
//    once passed, all progress state resets and the function returns.
//  - The state cleanup (gate byte 0x7124a1) runs only when no fade-out is pending, and states
//    4, 8 and out-of-range states fall through into the normal draw instead of returning.
//  - The draw uses explicit rectangles: the background quad fills {0,0,480,640}, the state
//    message is drawn in {410,0,430,640}, then string 7 in {430,0,450,640} (every state but 8)
//    and string 9 in {460,0,480,640}. chimera__draw_16_bit_text takes the rectangle in ECX
//    and 0 in EAX; text_set_render_context takes the font in ECX and an argb color {alpha,1,1,1}
//    in EAX; text_string_list_get_string takes the tag in ECX and the index in DX;
//    string_format_wide_va formats into the EDX buffer.
//  - Loading-string indices: 2 -> 1, 3 and 5 -> 2 (with progress_screen_text), 4 -> 0 (with
//    text), 6 -> 3 (text, progress), 7 -> 4, 8 -> 6 when network_game_mode is 2 else 5 (with
//    subtext), 9 -> 8 (with subtext).
// UNSURE: the 0x719754/0x719757/0x71973c/0x719a79/0x719a7a/0x719a9a/0x71c2de bytes belong to the
// chat and network-join code and are kept under neutral names; NNCancel is foreign.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern progress_screen_state join_ui_state; // 0x00718f8c, progress_screen_state in interface.h
extern int32_t interface_loading_screen_address_b;      // 0x0068e684
extern uint32_t interface_loading_screen_address_a;  // 0x0068e680, milliseconds, -1 when none
extern datum_index interface_loading_screen_request_id;         // 0x0068e688
extern int32_t interface_loading_screen_progress;         // 0x00718f90
extern uint16_t progress_screen_text[0x20];      // 0x006b2f28
extern uint16_t progress_screen_subtext[0x20];   // 0x006b2f68
extern uint8_t chimera_loading_screen_cleanup_gate; // 0x007124a1, UNSURE
extern int16_t network_game_mode;                // 0x00719720, 2 is host

extern uint8_t chat_state_00719a7a; // 0x00719a7a, UNSURE
extern uint8_t chat_state_00719a9a; // 0x00719a9a, UNSURE
extern uint8_t chat_state_00719a79; // 0x00719a79, UNSURE
extern uint8_t network_host_handoff_requested; // 0x0071c2de, UNSURE
extern uint16_t split_screen_quit_prompt_string; // 0x00719754, word stores
extern uint8_t split_screen_quit_prompt_armed;   // 0x00719757
extern uint8_t network_join_error_reason; // 0x0071973c, byte stores only

extern uint32_t time_query_performance_counter_ms(void); // 0x449210
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550; blam-cc: group in EDI
extern int32_t bitmap_group_sequence_get_bitmap_data(datum_index bitmap, int16_t sequence,
                                                     int16_t frame); // 0x43f290; blam-cc: EAX -> bitmap, EDI -> frame, stack -> sequence

extern void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                                int16_t *clip_rect, uint32_t vertex_color); // 0x498b20; blam-cc: EAX, ECX
extern void text_set_render_context(datum_index font, ColorARGB *color, int32_t flags,
                                    int32_t justification, int32_t unused); // 0x5563b0; blam-cc: ECX -> font, EAX -> color
extern uint16_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0; blam-cc: ECX, DX
extern uint16_t *string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930; blam-cc: EDX -> dest
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0,
                                      int32_t unknown_1, const uint16_t *text); // 0x514ab0; blam-cc: EAX -> zero, ECX -> bounds
extern void chat_close(void); // 0x4aa900
extern void NNCancel(datum_index tag); // 0x614fc0, UNSURE signature

// Per-frame progress-screen driver.
void chimera__do_show_loading_screen(void)
{
    float alpha;
    datum_index font, background, strings;
    int32_t bitmap_data;
    uint32_t packed_color;
    ColorARGB text_color;
    Rectangle2D bounds;
    uint16_t text_buffer[0x200];

    if (join_ui_state == 0) {
        return;
    }
    alpha = 1.0f;
    if (interface_loading_screen_address_b == -1) {
        interface_loading_screen_address_b = (int32_t)time_query_performance_counter_ms();
    }

    if (interface_loading_screen_address_a != 0xffffffffu) {
        uint32_t now = time_query_performance_counter_ms();
        if (now >= interface_loading_screen_address_a) {
            interface_loading_screen_address_a = 0xffffffffu;
            interface_loading_screen_address_b = -1;
            interface_loading_screen_request_id = (datum_index)-1;
            join_ui_state = 0;
            interface_loading_screen_progress = 0;
            progress_screen_text[0] = 0;
            progress_screen_subtext[0] = 0;
            return;
        }
        alpha = (float)(interface_loading_screen_address_a - now) * 0.0013333333f; // unsigned to float
        if (alpha < 0.0f) {
            alpha = 0.0f;
        } else if (alpha > 1.0f) {
            alpha = 1.0f;
        }
    } else if (chimera_loading_screen_cleanup_gate != 0) {
        switch (join_ui_state) {
        case 2: case 5: case 6: case 7: case 9:
            chat_state_00719a7a = 0;
            chat_state_00719a9a = 0;
            chat_state_00719a79 = 0;
            network_host_handoff_requested = 1;
            chat_close();
            return;
        case 3:
            split_screen_quit_prompt_string = 0xffff;
            chat_state_00719a7a = 0;
            chat_state_00719a9a = 0;
            chat_state_00719a79 = 0;
            network_join_error_reason = 0;
            split_screen_quit_prompt_armed = 1;
            return;
        case 4:
            if (interface_loading_screen_request_id != (datum_index)-1) {
                NNCancel(interface_loading_screen_request_id);
                interface_loading_screen_request_id = (datum_index)-1;
                split_screen_quit_prompt_string = 0xffff;
                network_join_error_reason = 0;
                split_screen_quit_prompt_armed = 1;
            }
            break;
        default: // 8 and out-of-range states draw
            break;
        }
    }

    font = tag_lookup(0x666f6e74, "ui\\large_ui");                       // 'font'
    background = tag_lookup(0x6269746d, "ui\\shell\\bitmaps\\background");  // 'bitm'
    strings = tag_lookup(0x75737472, "ui\\shell\\strings\\loading");      // 'ustr'
    if (font == (datum_index)-1 || background == (datum_index)-1 || strings == (datum_index)-1) {
        return;
    }

    bitmap_data = bitmap_group_sequence_get_bitmap_data(background, 0, 0);
    packed_color = color_argb_scale_alpha(0xffffffff, alpha);
    text_color.alpha = alpha;
    text_color.red = 1.0f;
    text_color.green = 1.0f;
    text_color.blue = 1.0f;
    bounds.top = 0;
    bounds.left = 0;
    bounds.bottom = 0x1e0;
    bounds.right = 0x280;
    if (bitmap_data != 0) {
        ui_draw_screen_quad((int16_t *)&bounds, (int16_t *)&bounds, bitmap_data, (int16_t *)0,
                            packed_color);
    }
    text_set_render_context(font, &text_color, -1, 2, 0);

    bounds.left = 0;
    bounds.top = 0x19a;
    bounds.right = 0x280;
    bounds.bottom = 0x1ae;
    switch (join_ui_state) {
    case 2:
        string_format_wide_va(text_buffer, text_string_list_get_string(strings, 1));
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text_buffer);
        break;
    case 3:
    case 5:
        string_format_wide_va(text_buffer, text_string_list_get_string(strings, 2), progress_screen_text);
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text_buffer);
        break;
    case 4:
        string_format_wide_va(text_buffer, text_string_list_get_string(strings, 0), progress_screen_text);
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text_buffer);
        break;
    case 6:
        string_format_wide_va(text_buffer, text_string_list_get_string(strings, 3), progress_screen_text,
                              interface_loading_screen_progress);
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text_buffer);
        break;
    case 7:
        string_format_wide_va(text_buffer, text_string_list_get_string(strings, 4));
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text_buffer);
        break;
    case 8:
        string_format_wide_va(text_buffer,
                              text_string_list_get_string(strings, (network_game_mode == 2) ? 6 : 5),
                              progress_screen_subtext);
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text_buffer);
        break;
    case 9:
        string_format_wide_va(text_buffer, text_string_list_get_string(strings, 8), progress_screen_subtext);
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text_buffer);
        break;
    default:
        break;
    }

    bounds.top = 0x1ae;
    bounds.bottom = 0x1c2;
    switch (join_ui_state) {
    case 2: case 3: case 4: case 5: case 6: case 7: case 9:
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text_string_list_get_string(strings, 7));
        // fall through
    case 8:
        bounds.top = 0x1cc;
        bounds.bottom = 0x1e0;
        chimera__draw_16_bit_text(0, &bounds, 0, 0, text_string_list_get_string(strings, 9));
        break;
    default:
        break;
    }
}

#if 0
Original Ghidra decompilation (0x497410):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void chimera__do_show_loading_screen(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  undefined *puVar6;
  float local_418;
  undefined1 auStack_400 [1024];

  if (DAT_00718f8c == 0) {
    return;
  }
  local_418 = 1.0;
  if (DAT_0068e684 == -1) {
    DAT_0068e684 = FUN_00449210();
  }
  if (DAT_0068e680 == 0xffffffff) {
LAB_004974e9:
    if (DAT_007124a1 != '\0') {
      switch(DAT_00718f8c) {
      case 2:
      case 5:
      case 6:
      case 7:
      case 9:
        DAT_00719a7a = 0;
        DAT_00719a9a = 0;
        DAT_00719a79 = 0;
        DAT_0071c2de = 1;
        chat_close();
        return;
      case 3:
        DAT_00719754._0_2_ = 0xffff;
        DAT_00719a7a = 0;
        DAT_00719a9a = 0;
        DAT_00719a79 = 0;
        DAT_0071973c = 0;
        DAT_00719754._3_1_ = 1;
        return;
      case 4:
        if (DAT_0068e688 != -1) {
          FUN_00614fc0(DAT_0068e688);
          DAT_0068e688 = -1;
          DAT_00719754._0_2_ = 0xffff;
          DAT_0071973c = 0;
          DAT_00719754._3_1_ = 1;
        }
      }
    }
  }
  else {
    uVar1 = FUN_00449210();
    if (DAT_0068e680 <= uVar1) {
      DAT_0068e680 = 0xffffffff;
      DAT_0068e684 = 0xffffffff;
      DAT_0068e688 = 0xffffffff;
      DAT_00718f8c = 0;
      DAT_00718f90 = 0;
      _DAT_006b2f28 = 0;
      _DAT_006b2f68 = 0;
      return;
    }
    local_418 = (float)(int)(DAT_0068e680 - uVar1);
    if ((int)(DAT_0068e680 - uVar1) < 0) {
      local_418 = local_418 + 4.2949673e+09;
    }
    local_418 = local_418 * 0.0013333333;
    if (0.0 <= local_418) {
      if (1.0 < local_418) {
        local_418 = 1.0;
      }
    }
    else {
      local_418 = 0.0;
    }
    if (DAT_0068e680 == 0xffffffff) goto LAB_004974e9;
  }
  iVar2 = tag_lookup("ui\\large_ui");
  iVar3 = tag_lookup("ui\\shell\\bitmaps\\background");
  iVar4 = tag_lookup("ui\\shell\\strings\\loading");
  if (iVar2 == -1) {
    return;
  }
  if (iVar3 == -1) {
    return;
  }
  if (iVar4 == -1) {
    return;
  }
  iVar2 = bitmap_group_sequence_get_bitmap_data(0);
  uVar5 = color_argb_scale_alpha(local_418);
  if (iVar2 != 0) {
    FUN_00498b20(iVar2,0,uVar5);
  }
  text_set_render_context(0xffffffff,2,0);
  switch(DAT_00718f8c) {
  case 2:
    uVar5 = text_string_list_get_string();
    string_format_wide_va(uVar5);
    chimera__draw_16_bit_text(0,0,auStack_400);
    break;
  case 3:
  case 5:
    uVar5 = text_string_list_get_string(&DAT_006b2f28);
    string_format_wide_va(uVar5);
    goto LAB_004977c5;
  case 4:
    puVar6 = &DAT_006b2f28;
    goto LAB_004977af;
  case 6:
    uVar5 = text_string_list_get_string(&DAT_006b2f28,DAT_00718f90);
    string_format_wide_va(uVar5);
    chimera__draw_16_bit_text(0,0,auStack_400);
    break;
  case 7:
    uVar5 = text_string_list_get_string();
    string_format_wide_va(uVar5);
    chimera__draw_16_bit_text(0,0,auStack_400);
    break;
  case 8:
    uVar5 = text_string_list_get_string(&DAT_006b2f68);
    string_format_wide_va(uVar5);
    chimera__draw_16_bit_text(0,0,auStack_400);
    break;
  case 9:
    puVar6 = &DAT_006b2f68;
LAB_004977af:
    uVar5 = text_string_list_get_string(puVar6);
    string_format_wide_va(uVar5);
LAB_004977c5:
    chimera__draw_16_bit_text(0,0,auStack_400);
  }
  switch(DAT_00718f8c) {
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 9:
    uVar5 = text_string_list_get_string();
    chimera__draw_16_bit_text(0,0,uVar5);
  case 8:
    uVar5 = text_string_list_get_string();
    chimera__draw_16_bit_text(0,0,uVar5);
  default:
    return;
  }
}
#endif
