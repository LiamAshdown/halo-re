// hud_messaging_update  (Ghidra: hud_messaging_update, already named; the CEA hint string
// "<no button icon>" is one of its literals)
// address 0x4ae550, size 2826 bytes (to 0x4af06a)
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4ae550..0x4af06a in the phase-4 review. The first rewrite
// was an acknowledged incomplete transliteration: it read 0x00873d40 as a struct (it is a
// pointer to the HUDGlobals messaging parameters: anchor +0x00, anchor offset +0x24, fonts
// +0x54/+0x64, up and fade time +0x68/+0x6c, icon and text colors +0x70/+0x80, text spacing
// +0x90), read 0x006f187c as an array (a pointer; byte +9 suppresses the HUD messages) and
// dropped the element loop, the argument drawing and the fade math.
// Flow: the record drawn is the one of current_local_player_index (0x007c3108); the argument
// only gates the draw through game_engine_local_player_score_is_nonpositive. One message line comes first: the objective text
// (while objective_text_ticks runs, flashing with the HUDGlobals objective colors and fading
// over objective_fade_ticks), else the help text (show_hud_help_text), else the action message
// of the record (a hud_messages message, or the action_text when no message is set). A
// message is a run of HUDMessageTextElement: type 0 is a text span of data characters of the
// tag text blob; any other type is an icon or argument reference, data 0 and 1 are button
// icons 2 and 9, 0x12..0x1c map through the table at 0x00692f36 to a button icon (drawn as
// the bound key name in quotes), 0x20.. reference the record arguments (a string of the
// scenario custom_object_names or HUDGlobals alternate_icon_text list, or an icon through
// hud_draw_message_icon). Then the four item/text message slots, sorted newest first
// (qsort with hud_message_compare), are drawn one per line below it, up to 4 lines (3 in split
// screen, one less while the action line is reserved), fading out with alpha * t^1.9 after
// up_time and dropped after up_time + fade_time (seconds * 30 ticks).
// register convention: AX local player index.
//   // blam-cc: local_player_index -> AX
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "crt.h"
#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "units.h"
#include "cutscene.h"

extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c
extern player_globals *local_player_globals;   // 0x0087a478
extern HUDGlobals *hud_messaging_parameters; // 0x00873d40, pointer to the HUDGlobals messaging block
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern hud_messaging_globals *hud_messaging;   // 0x006b3a40
extern hud_globals_flags *hud_flags;           // 0x00719420
extern tag_instance *tag_instances;            // 0x0087bc14
extern game_time_globals *game_time;           // 0x006f1d6c
extern Scenario *global_scenario; // 0x00746f8c
extern int16_t current_local_player_index; // 0x007c3108
extern int32_t hud_splitscreen_message_raise;  // 0x00692fc0, 17 pixels
extern int16_t render_viewport_left;           // 0x007c3142
extern Rectangle2D screen_safe_area_right;       // 0x007c3148, UNSURE name
extern int8_t hud_message_button_icon_table[0x1d]; // 0x00692f36, element data 0x12..0x1c -> button icon
extern const uint16_t *empty_wide_string_pointer; // 0x00692d7c, points at the empty wide string 0x00660c34
extern int32_t hud_text_draw_font_tag_id;      // 0x006e472c
extern uint32_t hud_text_draw_unknown_4730;    // 0x006e4730
extern uint16_t hud_text_draw_color_or_flags;  // 0x006e4734
extern int16_t hud_text_draw_column;           // 0x006e4736
extern float hud_text_draw_color_a;            // 0x006e4738
extern float hud_text_draw_color_r;            // 0x006e473c
extern float hud_text_draw_color_g;            // 0x006e4740
extern float hud_text_draw_color_b;            // 0x006e4744
extern int16_t ui_prompt_clip_x;               // 0x006e476e
extern int16_t ui_prompt_clip_y;               // 0x006e4770

extern const uint16_t hud_text_quote[];        // 0x00669cd0, L"\""
extern const uint16_t hud_text_unbound[];      // 0x00669cc8, L"???"
extern const uint16_t hud_text_unknown[];      // 0x0066a750, L"<unknown>"
extern const uint16_t hud_text_no_button_icon[]; // 0x0066a94c, L"<no button icon>"

extern int32_t __ftol(double x); // 0x006391b4, MSVC 7.1 CRT float-to-int truncation
extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow
extern uint8_t game_engine_local_player_score_is_nonpositive(datum_index player_index); // 0x466340, blam-cc: EAX player_index; UNSURE: a per player HUD visibility test
extern void color_argb_int_to_real(ColorARGB *out, uint32_t packed); // 0x43f5a0; blam-cc: EAX -> out, ECX -> packed
extern uint32_t color_pack_argb_from_real(ColorARGB *color); // 0x497900
extern void hud_anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale,
                                                 const int16_t *offset, int16_t *out, int32_t selector); // 0x4ab690, blam-cc: AL has_scale, EDX offset, ECX child placement (selector)
extern uint32_t hud_meter_flash_color_blend(const hud_flash_parameters *flash, int32_t start_time); // 0x4ab980, blam-cc: ESI flash, EDI start_time
extern void hud_draw_message_text_span(Rectangle2D *cursor, Rectangle2D *origin, const uint16_t *text,
                                       uint8_t allow_button_prompts); // 0x4ad8e0, blam-cc: EAX cursor, ECX origin
extern void hud_draw_message_icon(const hud_messaging_information *information, Rectangle2D *cursor,
                                  uint32_t color); // 0x4ad970, blam-cc: ESI information
extern int32_t hud_message_compare(const void *a, const void *b); // 0x4ae500
extern uint8_t input_get_last_used_binding(int16_t key, uint8_t *out_binding); // 0x48bde0; blam-cc: EAX -> key, stack -> 12 byte out
extern void input_get_binding_display_name(uint8_t *binding, uint16_t *out_name); // 0x48c7f0; blam-cc: EAX -> binding, ECX -> out_name
extern void text_measure_string_extents(Rectangle2D *origin, Rectangle2D *cursor, Rectangle2D *out_bounds,
                         const uint16_t *text); // 0x5562d0, measure a span, blam-cc: EBX origin, ESI cursor, EDI out_bounds
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0,
                                      int32_t unknown_1, const uint16_t *text); // 0x514ab0; blam-cc: EAX clip, ECX bounds
extern uint16_t *text_string_list_get_string(datum_index string_list_tag, int16_t index); // 0x5578c0; blam-cc: ECX -> tag, DX -> index
extern wchar_t *string_format_wide_va(wchar_t *dest, const wchar_t *format, ...); // 0x557930, blam-cc: EDX dest

static void hud_messaging_set_text_state(datum_index font, const ColorARGB *color)
{
    hud_text_draw_font_tag_id = (int32_t)font;
    hud_text_draw_color_a = color->alpha;
    hud_text_draw_color_r = color->red;
    hud_text_draw_color_g = color->green;
    hud_text_draw_color_b = color->blue;
    hud_text_draw_color_or_flags = 0xffff;
    hud_text_draw_column = 0;
    hud_text_draw_unknown_4730 = 0;
}

// One button icon element: the key bound to the button, in quotes.
static void hud_messaging_draw_button_icon(int16_t button_icon, Rectangle2D *cursor, Rectangle2D *line)
{
    uint8_t binding[12];
    uint16_t name[0x40]; // UNSURE size: the binary keeps it at esp+0x80 of a 0x2f4 byte frame

    if ((int32_t)button_icon >= (int32_t)hud_globals_tag_data->button_icons.count) {
        hud_draw_message_text_span(cursor, line, hud_text_no_button_icon, 0);
        return;
    }
    if (input_get_last_used_binding(button_icon, binding) == 0) {
        hud_draw_message_text_span(cursor, line, hud_text_unbound, 0);
        return;
    }
    input_get_binding_display_name(binding, name);
    hud_draw_message_text_span(cursor, line, hud_text_quote, 0);
    hud_draw_message_text_span(cursor, line, name, 0);
    hud_draw_message_text_span(cursor, line, hud_text_quote, 0);
}

// blam-cc: local_player_index -> AX
void hud_messaging_update(int16_t local_player_index)
{
    HUDGlobals *parameters;
    hud_player_messaging_state *record;
    datum_index player_index;
    datum_index font;
    Font *font_tag;
    Point2DInt origin;
    Rectangle2D cursor;
    Rectangle2D line;
    Rectangle2D bounds;
    ColorARGB color;
    uint32_t packed_color;
    int32_t line_height;
    int16_t y;
    int16_t max_lines;
    int16_t i;
    uint8_t split_screen;
    uint8_t objective_shown;
    uint8_t help_shown;
    uint8_t action_shown;
    int32_t now;

    if (cinematic_globals_ptr->in_progress != 0 || local_player_index == -1) {
        return;
    }
    player_index = local_player_index < 1 ? local_player_globals->local_players[local_player_index]
                                          : (datum_index)-1;
    if (game_engine_local_player_score_is_nonpositive(player_index) == 0) {
        return;
    }

    parameters = hud_messaging_parameters;
    split_screen = local_player_globals->local_player_count > 1;
    font = *(datum_index *)&parameters->fullscreen_font.tag_id;
    if (local_player_globals->local_player_count > 1 &&
        *(datum_index *)&parameters->splitscreen_font.tag_id != (datum_index)-1) {
        font = *(datum_index *)&parameters->splitscreen_font.tag_id;
    }
    hud_anchor_offset_to_screen_position((uint16_t *)&parameters->anchor, split_screen, 0.0f,
                                         &parameters->anchor_offset.x, &origin.x, 0);
    font_tag = (Font *)tag_instances[font & 0xffff].data;
    y = origin.y;
    if (split_screen) {
        line_height = (uint16_t)(font_tag->leading_height + font_tag->ascending_height);
        y = (int16_t)(y - hud_splitscreen_message_raise);
    } else {
        line_height = (uint16_t)(font_tag->leading_height + font_tag->descending_height + font_tag->ascending_height);
    }

    record = &hud_messaging->players[current_local_player_index];
    max_lines = (int16_t)(4 - (local_player_globals->local_player_count > 1));
    objective_shown = hud_messaging->objective_text != 0 && hud_messaging->objective_text_ticks != 0;
    help_shown = hud_flags->help_text_shown != 0 && hud_messaging->help_text != 0;
    action_shown = record->message_shown != 0 && (record->message != 0 || record->action_text[0] != 0);

    if (objective_shown || help_shown || action_shown) {
        HUDMessageText *messages_tag = 0;
        HUDMessageTextMessage *message = 0;
        uint16_t text_offset;
        int32_t element_index;

        // the line color
        if (objective_shown) {
            HUDGlobals *globals = hud_globals_tag_data;
            float fraction;

            now = game_time->game_time;
            packed_color = hud_meter_flash_color_blend(
                (const hud_flash_parameters *)&globals->objective_default_color,
                hud_messaging->objective_text_ticks - globals->objective_uptime_ticks - globals->objective_fade_ticks + now);
            color_argb_int_to_real(&color, packed_color);
            fraction = (float)hud_messaging->objective_text_ticks / (float)globals->objective_fade_ticks;
            if (fraction > 1.0f) {
                fraction = 1.0f;
            }
            color.alpha = fraction * color.alpha;
            packed_color = color_pack_argb_from_real(&color);
        } else if (help_shown) {
            if (hud_messaging->help_text_flashing != 0) {
                packed_color = hud_meter_flash_color_blend(
                    (const hud_flash_parameters *)&hud_globals_tag_data->hud_help_default_color,
                    hud_messaging->help_text_flash_start_time);
            } else if ((*(uint8_t *)&hud_globals_tag_data->hud_help_flash_flags & 1) != 0) {
                packed_color = *(uint32_t *)&hud_globals_tag_data->hud_help_flashing_color;
            } else {
                packed_color = *(uint32_t *)&hud_globals_tag_data->hud_help_default_color;
            }
            color_argb_int_to_real(&color, packed_color);
        } else {
            color = parameters->icon_color;
            packed_color = color_pack_argb_from_real(&color);
        }

        line.top = y;
        line.left = origin.x;
        line.bottom = (int16_t)(y + line_height * 5);
        line.right = (int16_t)(screen_safe_area_right.right - render_viewport_left);
        cursor = line;
        hud_messaging_set_text_state(font, &color);

        if (objective_shown) {
            int32_t remaining = hud_messaging->objective_text_ticks - game_time->ticks_this_frame;
            messages_tag = (HUDMessageText *)tag_instances[*(datum_index *)&global_scenario->hud_messages.tag_id & 0xffff].data;
            message = hud_messaging->objective_text;
            hud_messaging->objective_text_ticks = (int16_t)(remaining > 0 ? remaining : 0);
        } else if (help_shown) {
            messages_tag = (HUDMessageText *)tag_instances[*(datum_index *)&global_scenario->hud_messages.tag_id & 0xffff].data;
            message = hud_messaging->help_text;
        } else if (record->message != 0) {
            messages_tag = (HUDMessageText *)tag_instances[*(datum_index *)&hud_globals_tag_data->hud_messages.tag_id & 0xffff].data;
            message = record->message;
        } else if (record->action_text[0] != 0) {
            hud_draw_message_text_span(&cursor, &line, record->action_text, 1);
        }

        if (message != 0 && message->panel_count != 0) {
            text_offset = message->start_index_into_text_blob;
            for (element_index = 0; (int16_t)element_index < (int32_t)message->panel_count; element_index++) {
                // panel_count of types/tags.h is the element count of the message
                HUDMessageTextElement *element = (HUDMessageTextElement *)messages_tag->message_elements.pointer +
                                                 (message->start_index_of_message_block + element_index);
                uint8_t data = element->data;

                if (element->type == 0) { // text span
                    const uint16_t *text = (const uint16_t *)messages_tag->text_data.pointer + text_offset;

                    ui_prompt_clip_x = (int16_t)(cursor.left - line.left);
                    ui_prompt_clip_y = 0;
                    text_measure_string_extents(&line, &cursor, &bounds, text);
                    cursor.left = (int16_t)(cursor.left - 3);
                    bounds.left = line.left;
                    chimera__draw_16_bit_text(0, &bounds, 0, 0, text);
                    line.top = cursor.top;
                    text_offset = (uint16_t)(text_offset + data);
                } else if (data <= 0x11) {
                    if (data == 0) {
                        hud_messaging_draw_button_icon(2, &cursor, &line);
                    } else if (data == 1) {
                        hud_messaging_draw_button_icon(9, &cursor, &line);
                    }
                } else if (data <= 0x1f) {
                    if (data <= 0x1c && hud_message_button_icon_table[data] != -1) {
                        hud_messaging_draw_button_icon(hud_message_button_icon_table[data], &cursor, &line);
                    }
                } else if (hud_flags->help_text_shown == 0) { // argument reference
                    int16_t argument = (int16_t)(data - 0x20);

                    if ((record->argument_is_string & (uint8_t)(1 << argument)) != 0) {
                        uint8_t *reference = (uint8_t *)&record->arguments[argument];
                        uint16_t string_index = *(uint16_t *)reference;

                        if (string_index == 0xffff) {
                            hud_draw_message_text_span(&cursor, &line, hud_text_unknown, 0);
                        } else if (reference[2] != 0) {
                            hud_draw_message_text_span(&cursor, &line,
                                text_string_list_get_string(*(datum_index *)&global_scenario->custom_object_names.tag_id,
                                                            (int16_t)string_index), 0);
                        } else {
                            hud_draw_message_text_span(&cursor, &line,
                                text_string_list_get_string(*(datum_index *)&hud_globals_tag_data->alternate_icon_text.tag_id,
                                                            (int16_t)string_index), 0);
                        }
                    } else if (record->arguments[argument] != 0) {
                        hud_draw_message_icon((const hud_messaging_information *)record->arguments[argument], &cursor,
                                              packed_color);
                    }
                }
            }
        }
        ui_prompt_clip_x = 0;
        ui_prompt_clip_y = 0;
        y = cursor.bottom;
    }

    if (!objective_shown && !help_shown && (record->message_shown != 0 || record->prompt_changed != 0)) {
        // one line stays reserved for the action message
        if (split_screen) {
            y = (int16_t)__ftol((double)((float)(origin.y - hud_splitscreen_message_raise) +
                                         (float)line_height * parameters->text_spacing));
        } else {
            y = (int16_t)__ftol((double)((float)line_height * parameters->text_spacing + (float)origin.y));
        }
        max_lines--;
    }

    qsort(record->messages, 4, sizeof(hud_message_slot), hud_message_compare);
    for (i = 0; i < max_lines; i++) {
        hud_message_slot *slot = &record->messages[i];
        float elapsed;
        float up_ticks;

        if (slot->active == 0) {
            return;
        }
        now = game_time->game_time;
        color = parameters->text_color;
        elapsed = (float)(now - slot->timestamp);
        up_ticks = parameters->up_time * 30.0f;
        if (elapsed > up_ticks) {
            float t = 1.0f - (elapsed - up_ticks) / (parameters->fade_time * 30.0f);
            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
            color.alpha = (float)pow((double)t, 1.899999976158142) * color.alpha;
        }

        cursor = screen_safe_area_right;
        cursor.right = (int16_t)(screen_safe_area_right.right - render_viewport_left);
        cursor.left = origin.x;
        cursor.top = y;
        cursor.bottom = (int16_t)(line_height + y);
        y = (int16_t)__ftol((double)((float)line_height * parameters->text_spacing + (float)y));
        hud_messaging_set_text_state(font, &color);

        if (slot->source == -1) {
            chimera__draw_16_bit_text(0, &cursor, 0, 0, slot->text);
        } else {
            Item *item;
            const uint16_t *text;
            uint8_t plural;
            int32_t string_index;
            datum_index strings = *(datum_index *)&hud_globals_tag_data->item_message_text.tag_id;

            if (slot->source_kind == 0xff) {
                plural = slot->count > 1;
            } else {
                plural = slot->source_kind;
            }
            item = (Item *)tag_instances[slot->source & 0xffff].data;
            string_index = (int16_t)((int8_t)plural + item->pickup_text_index);
            text = empty_wide_string_pointer;
            if (strings != (datum_index)-1) {
                int32_t *string_list = (int32_t *)tag_instances[strings & 0xffff].data;
                if (string_list != 0 && string_index >= 0 && string_index < string_list[0]) {
                    text = text_string_list_get_string(strings, (int16_t)string_index);
                }
            }
            if ((slot->source_kind == 0xff && plural != 0) || slot->count != 0) {
                wchar_t formatted[0x80]; // UNSURE size: esp+0x100 of the 0x2f4 byte frame
                int32_t value_scale = (int16_t)item->hud_message_value_scale;
                if (value_scale <= 1) {
                    value_scale = 1;
                }
                string_format_wide_va(formatted, (const wchar_t *)text, slot->count / value_scale);
                chimera__draw_16_bit_text(0, &cursor, 0, 0, (const uint16_t *)formatted);
            } else {
                chimera__draw_16_bit_text(0, &cursor, 0, 0, text);
            }
        }

        slot->active = (parameters->fade_time + parameters->up_time) * 30.0f > (float)(now - slot->timestamp);
        if (slot->active == 0) {
            slot->timestamp = -1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ae550):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void hud_messaging_update(void)

{
  char *pcVar1;
  byte bVar2;
  int *piVar3;
  float fVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  int iVar8;
  char cVar9;
  short in_AX;
  short sVar10;
  short sVar11;
  undefined4 uVar12;
  undefined2 extraout_var;
  int iVar13;
  int iVar14;
  int *piVar15;
  ushort uVar16;
  int iVar17;
  int *piVar18;
  float10 fVar19;
  undefined4 uVar20;
  char local_2fa;
  short local_2f4;
  short sStack_2f2;
  short local_2f0;
  short sStack_2ee;
  char local_2e9;
  short local_2e8;
  short sStack_2e6;
  short local_2e4;
  short sStack_2e2;
  int local_2e0;
  int local_2dc;
  float local_2d8;
  uint local_2d4;
  uint local_2d0;
  int local_2cc;
  float local_2c8;
  undefined4 local_2c4;
  undefined4 local_2c0;
  undefined4 local_2bc;
  short *local_2b8;
  int local_2b4;
  uint local_2b0;
  void *local_2ac;
  short local_2a8;
  int local_2a6;
  int local_2a0;
  short local_29a;
  undefined1 local_294 [12];
  undefined1 local_288 [128];
  undefined1 local_208 [516];

  iVar14 = DAT_0087a478;
  if (*(char *)(DAT_006f187c + 9) != '\0') {
    return;
  }
  if (in_AX == -1) {
    return;
  }
  cVar9 = FUN_00466340();
  if (cVar9 == '\0') {
    return;
  }
  sVar11 = *(short *)(iVar14 + 0xc);
  if ((sVar11 < 2) || (local_2b0 = *(uint *)(DAT_00873d40 + 100), local_2b0 == 0xffffffff)) {
    local_2b0 = *(uint *)(DAT_00873d40 + 0x54);
  }
  local_2e9 = 1 < sVar11;
  FUN_004ab690(DAT_00873d40,0,&local_2a8);
  iVar8 = DAT_0071941c;
  iVar17 = DAT_006b3a40;
  iVar13 = *(int *)((local_2b0 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (local_2e9 == '\0') {
    uVar16 = *(short *)(iVar13 + 8) + *(short *)(iVar13 + 6) + *(short *)(iVar13 + 4);
    iVar13 = local_2a6;
  }
  else {
    uVar16 = *(short *)(iVar13 + 8) + *(short *)(iVar13 + 4);
    iVar13 = local_2a6 - DAT_00692fc0;
  }
  local_2d0 = (uint)uVar16;
  local_2ac = (void *)(DAT_007c3108 * 0x460 + DAT_006b3a40);
  local_2b4 = 4 - (uint)(1 < *(short *)(iVar14 + 0xc));
  if ((*(int *)(DAT_006b3a40 + 0x470) == 0) || (bVar5 = true, *(short *)(DAT_006b3a40 + 0x474) == 0)
     ) {
    bVar5 = false;
  }
  if ((*(char *)(DAT_00719420 + 1) == '\0') || (bVar7 = true, *(int *)(DAT_006b3a40 + 0x46c) == 0))
  {
    bVar7 = false;
  }
  if ((*(char *)((int)local_2ac + 0x458) == '\0') ||
     ((*(int *)((int)local_2ac + 0x454) == 0 && (*(short *)((int)local_2ac + 0x230) == 0)))) {
    bVar6 = false;
  }
  else {
    bVar6 = true;
  }
  if (((bVar5) || (bVar7)) || (bVar6)) {
    local_2b8 = (short *)((int)local_2ac + 0x230);
    if (bVar5) {
      FUN_004ab980();
      color_argb_int_to_real();
      local_2dc = (int)*(short *)(iVar8 + 0x11e);
      fVar4 = (float)(int)*(short *)(DAT_006b3a40 + 0x474) / (float)local_2dc;
      if (1.0 < fVar4) {
        fVar4 = 1.0;
      }
      local_2c8 = fVar4 * local_2c8;
      local_2d4 = color_pack_argb_from_real(&local_2c8);
      iVar17 = DAT_006b3a40;
    }
    else if (bVar7) {
      if (*(char *)(DAT_006b3a40 + 0x464) == '\0') {
        if ((*(byte *)(DAT_0071941c + 0xe2) & 1) == 0) {
          local_2d4 = *(uint *)(DAT_0071941c + 0xd0);
          color_argb_int_to_real();
        }
        else {
          local_2d4 = *(uint *)(DAT_0071941c + 0xd4);
          color_argb_int_to_real();
        }
      }
      else {
        local_2d4 = FUN_004ab980();
        iVar17 = DAT_006b3a40;
        color_argb_int_to_real();
      }
    }
    else {
      local_2c8 = *(float *)(DAT_00873d40 + 0x70);
      local_2c4 = *(undefined4 *)(DAT_00873d40 + 0x74);
      local_2c0 = *(undefined4 *)(DAT_00873d40 + 0x78);
      local_2bc = *(undefined4 *)(DAT_00873d40 + 0x7c);
      local_2d4 = color_pack_argb_from_real(&local_2c8);
    }
    iVar8 = DAT_0087bc14;
    iVar14 = global_scenario;
    sStack_2ee = DAT_007c314c._2_2_ - DAT_007c3140._2_2_;
    local_2f4 = (short)iVar13;
    local_2f0 = local_2f4 + (short)local_2d0 * 5;
    sStack_2e6 = local_2a8;
    sStack_2f2 = local_2a8;
    DAT_006e473c = local_2c4;
    DAT_006e472c = local_2b0;
    DAT_006e4738 = local_2c8;
    DAT_006e4740 = local_2c0;
    DAT_006e4744 = local_2bc;
    DAT_006e4734._0_2_ = 0xffff;
    DAT_006e4734._2_2_ = 0;
    _DAT_006e4730 = 0;
    local_2e8 = local_2f4;
    local_2e4 = local_2f0;
    sStack_2e2 = sStack_2ee;
    if (bVar5) {
      sVar11 = *(short *)(DAT_006f1d6c + 0x10);
      sVar10 = *(short *)(iVar17 + 0x474);
      if ((int)sVar10 == (int)sVar11 || (int)sVar10 - (int)sVar11 < 0) {
        sVar10 = 0;
      }
      else {
        sVar10 = sVar10 - sVar11;
      }
      *(short *)(iVar17 + 0x474) = sVar10;
      local_2e0 = *(int *)((*(uint *)(iVar14 + 0x5a0) & 0xffff) * 0x20 + 0x14 + iVar8);
      local_2cc = *(int *)(iVar17 + 0x470);
LAB_004ae9bd:
      local_2d8 = (float)CONCAT22(local_2d8._2_2_,*(undefined2 *)(local_2cc + 0x20));
      local_2dc = 0;
      if (*(char *)(local_2cc + 0x24) != '\0') {
        iVar13 = 0;
        iVar14 = local_2cc;
        do {
          pcVar1 = (char *)(*(int *)(local_2e0 + 0x18) +
                           ((uint)*(ushort *)(iVar14 + 0x22) + iVar13) * 2);
          if (*pcVar1 == '\0') {
            local_2a0 = *(int *)(local_2e0 + 0xc) + ((uint)local_2d8 & 0xffff) * 2;
            DAT_006e476e = sStack_2f2 - sStack_2e6;
            DAT_006e4770 = 0;
            FUN_005562d0(local_2a0);
            sStack_2f2 = sStack_2f2 + -3;
            local_29a = sStack_2e6;
            chimera__draw_16_bit_text(0,0,local_2a0);
            local_2e8 = local_2f4;
            local_2d8 = (float)((int)local_2d8 + CONCAT22(extraout_var,(ushort)(byte)pcVar1[1]));
            iVar14 = local_2cc;
          }
          else {
            bVar2 = pcVar1[1];
            if (bVar2 < 0x12) {
              if (bVar2 == 0) {
                sVar11 = 2;
              }
              else {
                if (bVar2 != 1) goto LAB_004aec6e;
                sVar11 = 9;
              }
LAB_004aea2f:
              if ((int)sVar11 < *(int *)(DAT_0071941c + 0xc4)) {
                cVar9 = FUN_0048bde0(local_294);
                if (cVar9 == '\0') {
                  FUN_004ad8e0(&DAT_00669cc8,0);
                }
                else {
                  FUN_0048c7f0();
                  FUN_004ad8e0(&DAT_00669cd0,0);
                  FUN_004ad8e0(local_288,0);
                  FUN_004ad8e0(&DAT_00669cd0,0);
                }
              }
              else {
                FUN_004ad8e0(L"<no button icon>",0);
              }
            }
            else if (bVar2 < 0x20) {
              if ((bVar2 < 0x1d) && (sVar11 = (short)(char)(&DAT_00692f36)[bVar2], sVar11 != -1))
              goto LAB_004aea2f;
            }
            else if (*(char *)(DAT_00719420 + 1) == '\0') {
              iVar13 = (int)(short)(bVar2 - 0x20);
              if (((byte)(1 << ((byte)(bVar2 - 0x20) & 0x1f)) & *(byte *)((int)local_2b8 + 0x229))
                  == 0) {
                if (*(int *)(local_2b8 + iVar13 * 2 + 0x102) != 0) {
                  FUN_004ad970(&local_2f4,local_2d4);
                }
              }
              else {
                uVar20 = 0;
                if (local_2b8[iVar13 * 2 + 0x102] == -1) {
                  FUN_004ad8e0(&PTR_DAT_0066a750,0);
                }
                else if ((char)local_2b8[iVar13 * 2 + 0x103] == '\0') {
                  uVar20 = text_string_list_get_string();
                  FUN_004ad8e0(uVar20);
                }
                else {
                  uVar12 = text_string_list_get_string();
                  FUN_004ad8e0(uVar12,uVar20);
                }
              }
            }
          }
LAB_004aec6e:
          local_2dc = local_2dc + 1;
          iVar13 = (int)(short)local_2dc;
        } while (iVar13 < (int)(uint)*(byte *)(iVar14 + 0x24));
      }
    }
    else {
      if (bVar7) {
        local_2e0 = *(int *)((*(uint *)(global_scenario + 0x5a0) & 0xffff) * 0x20 + 0x14 +
                            DAT_0087bc14);
        local_2cc = *(int *)(iVar17 + 0x46c);
        goto LAB_004ae9bd;
      }
      if (*(int *)((int)local_2ac + 0x454) != 0) {
        local_2e0 = *(int *)((*(uint *)(DAT_0071941c + 0xfc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
        ;
        local_2cc = *(int *)((int)local_2ac + 0x454);
        goto LAB_004ae9bd;
      }
      if (*local_2b8 != 0) {
        FUN_004ad8e0(local_2b8,1);
      }
    }
    DAT_006e476e = 0;
    DAT_006e4770 = 0;
    iVar13 = CONCAT22(sStack_2ee,local_2f0);
    if ((bVar5) || (bVar7)) goto LAB_004aed31;
  }
  if ((*(char *)((int)local_2ac + 0x458) != '\0') || (*(char *)((int)local_2ac + 0x45e) != '\0')) {
    iVar13 = __ftol();
    local_2b4 = local_2b4 + -1;
  }
LAB_004aed31:
  _qsort(local_2ac,4,0x8c,FUN_004ae500);
  local_2e0 = 0;
  iVar14 = DAT_00873d40;
  if (0 < (short)local_2b4) {
    do {
      piVar18 = (int *)((short)local_2e0 * 0x8c + (int)local_2ac);
      if (*(char *)((short)local_2e0 * 0x8c + 0x82 + (int)local_2ac) == '\0') {
        return;
      }
      local_2dc = *(int *)(DAT_006f1d6c + 0xc);
      local_2c8 = *(float *)(iVar14 + 0x80);
      local_2c4 = *(undefined4 *)(iVar14 + 0x84);
      local_2c0 = *(undefined4 *)(iVar14 + 0x88);
      local_2bc = *(undefined4 *)(iVar14 + 0x8c);
      fVar4 = *(float *)(iVar14 + 0x68) * 30.0;
      if (fVar4 < (float)(local_2dc - *piVar18)) {
        local_2d8 = 1.0 - ((float)(local_2dc - *piVar18) - fVar4) /
                          (*(float *)(iVar14 + 0x6c) * 30.0);
        if (0.0 <= local_2d8) {
          if (1.0 < local_2d8) {
            local_2d8 = 1.0;
          }
        }
        else {
          local_2d8 = 0.0;
        }
        fVar19 = (float10)FUN_006283c0();
        local_2c8 = (float)(fVar19 * (float10)local_2c8);
      }
      sStack_2ee = DAT_007c314c._2_2_ - DAT_007c3140._2_2_;
      sStack_2f2 = local_2a8;
      local_2f4 = (short)iVar13;
      local_2f0 = (short)local_2d0 + local_2f4;
      iVar13 = __ftol();
      DAT_006e472c = local_2b0;
      DAT_006e4738 = local_2c8;
      DAT_006e473c = local_2c4;
      DAT_006e4740 = local_2c0;
      DAT_006e4744 = local_2bc;
      DAT_006e4734._0_2_ = 0xffff;
      DAT_006e4734._2_2_ = 0;
      _DAT_006e4730 = 0;
      if (piVar18[0x21] == 0xffffffff) {
        piVar15 = piVar18 + 1;
LAB_004aeffb:
        chimera__draw_16_bit_text(0,0,piVar15);
      }
      else {
        local_2fa = *(char *)((int)piVar18 + 0x8a);
        if (local_2fa == -1) {
          local_2fa = 1 < (short)piVar18[0x22];
        }
        local_2a0 = *(int *)((piVar18[0x21] & 0xffffU) * 0x20 + 0x14 + DAT_0087bc14);
        iVar14 = (int)(short)((short)local_2fa + *(short *)(local_2a0 + 0x180));
        piVar15 = (int *)PTR_DAT_00692d7c;
        if ((((*(uint *)(DAT_0071941c + 0xa0) != 0xffffffff) &&
             (piVar3 = *(int **)((*(uint *)(DAT_0071941c + 0xa0) & 0xffff) * 0x20 + 0x14 +
                                DAT_0087bc14), piVar3 != (int *)0x0)) && (-1 < iVar14)) &&
           (iVar14 < *piVar3)) {
          piVar15 = (int *)text_string_list_get_string();
        }
        if (((*(char *)((int)piVar18 + 0x8a) != -1) || (local_2fa == '\0')) &&
           ((short)piVar18[0x22] == 0)) goto LAB_004aeffb;
        sVar11 = *(short *)(local_2a0 + 0x188);
        if (sVar11 < 2) {
          sVar11 = 1;
        }
        string_format_wide_va(piVar15,(int)(short)piVar18[0x22] / (int)sVar11);
        chimera__draw_16_bit_text(0,0,local_208);
      }
      iVar14 = DAT_00873d40;
      bVar5 = (float)(local_2dc - *piVar18) <
              (*(float *)(DAT_00873d40 + 0x6c) + *(float *)(DAT_00873d40 + 0x68)) * 30.0;
      *(bool *)((int)piVar18 + 0x82) = bVar5;
      if (!bVar5) {
        *piVar18 = -1;
      }
      local_2e0 = local_2e0 + 1;
    } while ((short)local_2e0 < (short)local_2b4);
  }
  return;
}
#endif
