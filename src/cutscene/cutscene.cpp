#include "halo/cutscene/cutscene.hpp"
#include "halo/core/datum.hpp"

extern "C" {
extern void sound_set_music_gain(float gain);
extern void game_engine_cleanup_stray_projectiles(void);
extern float sound_music_gain;
extern float cinematic_saved_music_gain;
extern cinematic_globals *cinematic_globals_ptr;
extern player_globals *local_player_globals;
extern ai_globals *ai_globals_ptr;
extern game_time_globals *game_time;
extern void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error);
extern cinematic_screen_effect_globals *cinematic_screen_effect_state;
extern ColorARGB *rasterizer_model_ambient_reflection_tint;
extern ui_pending_error ui_pending_errors[4];
extern int32_t ROUND(float x);
extern float fabsf(float x);
extern Scenario *global_scenario;
extern tag_instance *tag_instances;
extern HUDGlobals *hud_globals_tag_data;
extern uint8_t widget_memory_pool_valid;
extern widget_instance *ui_root_widget[1];
extern Rectangle2D render_viewport_top;
extern uint32_t text_shadow_color_argb;
extern void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect);
extern void color_argb_int_to_real(ColorARGB *out, uint32_t packed);
extern void text_set_render_context(datum_index font, ColorARGB *color, int16_t style, int16_t justification, uint32_t flags);
extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index);
extern void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text);
void cutscene_start(void);
void cutscene_stop(void);
void cutscene_title_queue(int16_t title_index, float delay_seconds);
void chimera__letterbox(void);
}

namespace halo::cutscene {

namespace {
constexpr uint32_t k_rgb_mask = 0x00ffffff;
constexpr uint32_t k_alpha_shift = 24;
}

/**
 * Begins a cutscene: saves the current music gain (once, if nothing is already saved), forces
 * music to full volume, marks the local player and ai communication state for cinematic mode,
 * seeds the letterbox fade clock from the current game tick and marks it and the cinematic as
 * active, then tail-calls the stray-projectile cleanup the hs cinematic layer also runs on
 * start.
 *
 * Register convention in the original: __cdecl, no parameters; every access is through named
 * globals.
 *
 * @address 0x449720
 */
void CutsceneDirector::start()
{
    if (cinematic_saved_music_gain == -1.0f) {
        cinematic_saved_music_gain = sound_music_gain;
    }
    sound_set_music_gain(1.0f);

    local_player_globals->input_disabled = 1;
    ai_globals_ptr->dialogue_triggers_enabled = 0;

    cinematic_globals_ptr->show_letterbox = 1;
    cinematic_globals_ptr->letterbox_last_tick = game_time->game_time;
    cinematic_globals_ptr->in_progress = 1;

    game_engine_cleanup_stray_projectiles();
}

/**
 * Ends a cutscene: restores the previous music gain, clears the cinematic/letterbox state,
 * resets the screen-effect and model-tint output blocks, and surfaces any pending cutscene
 * error for local player 0 before clearing it.
 *
 * Register convention in the original: __cdecl, no parameters; every access is through named
 * globals.
 *
 * @address 0x449eb0
 */
void CutsceneDirector::stop()
{
    cinematic_screen_effect_globals *effects;
    ColorARGB *tint;
    int32_t i;

    if (cinematic_saved_music_gain != -1.0f) {
        sound_set_music_gain(cinematic_saved_music_gain);
    }

    cinematic_globals_ptr->show_letterbox = 0;
    local_player_globals->input_disabled = 0;
    ai_globals_ptr->dialogue_triggers_enabled = 1;

    effects = cinematic_screen_effect_state;
    cinematic_saved_music_gain = -1.0f;
    cinematic_globals_ptr->in_progress = 0;

    if (effects != 0) {
        
        for (i = 0; i < (int32_t)(sizeof(cinematic_screen_effect_globals) / 4); i += 1) {
            ((uint32_t *)effects)[i] = 0;
        }
        effects->script_values[0] = 1.0f;
        effects->script_values[1] = 1.0f;
        effects->script_values[2] = 1.0f;
        effects->script_values[3] = 1.0f;
    }

    tint = rasterizer_model_ambient_reflection_tint;
    if (rasterizer_model_ambient_reflection_tint != 0) {
        tint->alpha = 0.0f;
        tint->red = 0.0f;
        tint->green = 0.0f;
        tint->blue = 0.0f;
    }

    if (effects != 0) {
        effects->near_clip_distance = 0.0f;
    }

    if (-1 < ui_pending_errors[0].error_string_index && ui_pending_errors[0].error_string_index < 0x3c) {
        display_error(ui_pending_errors[0].error_string_index, 0,
            ui_pending_errors[0].modal, ui_pending_errors[0].is_error);
    }
    ui_pending_errors[0].error_string_index = (int16_t)halo::k_word_none;
}

/**
 * Queues a cutscene title for display in the first free slot (of up to
 * k_cinematic_title_slot_count concurrent slots), scheduling it to start fading in after
 * delay_seconds. Does nothing if every slot is already occupied.
 *
 * Register convention in the original: __cdecl, both parameters recognized by Ghidra on the
 * stack (int16 title.
 *
 * @address 0x449960
 */
void CutsceneDirector::title_queue(int16_t title_index, float delay_seconds)
{
    int16_t i;

    for (i = 0; i < k_cinematic_title_slot_count; i += 1) {
        if (cinematic_globals_ptr->titles[i].title_index == k_cinematic_title_none) {
            cinematic_globals_ptr->titles[i].title_index = title_index;
            cinematic_globals_ptr->titles[i].ticks =
                -(int16_t)ROUND(delay_seconds * k_cinematic_ticks_per_second);
            return;
        }
    }
}

/**
 * Per-frame cinematic update: steps the letterbox fade towards show_letterbox (1 second full
 * travel) and draws both bars while the fade is above 0, then advances and draws every queued
 * cutscene title. The fade/draw half is skipped entirely while a ui widget is open (the
 * letterbox clock is simply not advanced that frame); the title loop always runs.
 *
 * Register convention in the original: __cdecl, no parameters; every access is through named
 * globals.
 *
 * @address 0x4499c0
 */
void CutsceneDirector::letterbox()
{
    int16_t i;

    if (cinematic_globals_ptr->show_letterbox != 0 || cinematic_globals_ptr->letterbox_scale > 0.0f) {
        
        
        uint8_t widget_open = widget_memory_pool_valid != 0 && ui_root_widget[0] != (widget_instance *)0;

        if (!widget_open) {
            int32_t old_tick = cinematic_globals_ptr->letterbox_last_tick;
            float delta = (float)(game_time->game_time - old_tick) * (1.0f / k_cinematic_ticks_per_second);
            cinematic_globals_ptr->letterbox_last_tick = game_time->game_time;

            if (cinematic_globals_ptr->show_letterbox == 0) {
                float scale = cinematic_globals_ptr->letterbox_scale - delta;
                if (scale <= 0.0f) { 
                    scale = 0.0f;
                }
                cinematic_globals_ptr->letterbox_scale = scale;
            } else {
                float scale = cinematic_globals_ptr->letterbox_scale + delta;
                if (scale > 1.0f) {
                    scale = 1.0f;
                }
                cinematic_globals_ptr->letterbox_scale = scale;
            }

            if (cinematic_globals_ptr->letterbox_scale > 0.0f) {
                
                
                
                float bar_height = cinematic_globals_ptr->letterbox_scale * 0.125f *
                    (float)k_cinematic_letterbox_screen_height;
                Rectangle2D bar;

                bar.left = (int16_t)ROUND((float)render_viewport_top.left);
                bar.right = (int16_t)ROUND((float)k_cinematic_letterbox_screen_width);
                bar.top = (int16_t)ROUND((float)render_viewport_top.top);
                bar.bottom = (int16_t)ROUND((float)render_viewport_top.top + bar_height);
                ui_draw_filled_rectangle((uint32_t)k_cinematic_letterbox_color, &bar);

                bar.left = (int16_t)ROUND((float)render_viewport_top.left);
                bar.right = (int16_t)ROUND((float)k_cinematic_letterbox_screen_width);
                bar.top = (int16_t)ROUND((float)k_cinematic_letterbox_screen_height - bar_height);
                bar.bottom = (int16_t)k_cinematic_letterbox_bottom_edge;
                ui_draw_filled_rectangle((uint32_t)k_cinematic_letterbox_color, &bar);
            }
        }
    }

    for (i = 0; i < k_cinematic_title_slot_count; i += 1) {
        cinematic_title_slot *slot = &cinematic_globals_ptr->titles[i];
        
        
        datum_index fullscreen_font = *(datum_index *)&hud_globals_tag_data->fullscreen_font.tag_id;

        if (slot->title_index == k_cinematic_title_none || fullscreen_font == (datum_index)k_datum_index_none) {
            continue;
        }

        {
            ScenarioCutsceneTitle *title =
                &((ScenarioCutsceneTitle *)global_scenario->cutscene_titles.pointer)[slot->title_index];
            datum_index help_text_list = *(datum_index *)&global_scenario->ingame_help_text.tag_id;

            
            
            if (help_text_list == (datum_index)k_datum_index_none || (int16_t)title->string_index < 0) {
                continue;
            }

            {
                UnicodeStringList *help_text_data =
                    (UnicodeStringList *)tag_instances[halo::datum_slot(help_text_list)].data;
                if ((int32_t)(int16_t)title->string_index >= (int32_t)help_text_data->strings.count) {
                    continue;
                }
            }

            {
                float ticks = (float)slot->ticks;
                float fade;
                ColorARGB tint;
                int32_t shadow_alpha;
                uint16_t *help_text;
                Rectangle2D *dest_rect;
                int16_t new_ticks;

                if (ticks < title->fade_in_time) {
                    fade = ticks / title->fade_in_time;
                    if (fade < 0.0f) fade = 0.0f;
                    else if (fade > 1.0f) fade = 1.0f;
                } else if (ticks > title->up_time) {
                    fade = 1.0f - (ticks - title->up_time) / title->fade_out_time;
                    if (fade < 0.0f) fade = 0.0f;
                    else if (fade > 1.0f) fade = 1.0f;
                } else {
                    fade = 1.0f;
                }

                color_argb_int_to_real(&tint, *(uint32_t *)&title->text_color);
                tint.alpha *= fade;

                if (fabsf(tint.red - 1.0f) < 0.0001f && fabsf(tint.green - 1.0f) < 0.0001f &&
                    fabsf(tint.blue - 1.0f) < 0.0001f) {
                    
                    if (tint.red > 0.8f) tint.red = 0.8f;
                    if (tint.green > 0.8f) tint.green = 0.8f;
                    if (tint.blue > 0.8f) tint.blue = 0.8f;
                }

                text_set_render_context(fullscreen_font, &tint, (int16_t)(title->text_style - 1),
                    title->justification, title->text_flags);

                shadow_alpha = ROUND((float)title->shadow_color.alpha * fade);
                if (shadow_alpha < 0) {
                    shadow_alpha = 0;
                } else if (shadow_alpha > 0xff) {
                    shadow_alpha = 0xff;
                }
                text_shadow_color_argb =
                    (*(uint32_t *)&title->shadow_color & k_rgb_mask) | ((uint32_t)shadow_alpha << k_alpha_shift);

                help_text = text_string_list_get_string(help_text_list, (int16_t)title->string_index);
                dest_rect = (title->text_bounds.right == title->text_bounds.left ||
                             title->text_bounds.bottom == title->text_bounds.top)
                    ? &hud_globals_tag_data->default_chapter_title_bounds
                    : &title->text_bounds;
                chimera__draw_16_bit_text((Rectangle2D *)0, (int32_t *)dest_rect, 0, 0,
                    (const int16_t *)help_text);
                text_shadow_color_argb = 0;

                new_ticks = slot->ticks + (game_time->paused == 0 ? game_time->ticks_this_frame : 0);
                slot->ticks = new_ticks;
                if (title->up_time + title->fade_out_time <= (float)new_ticks) {
                    slot->title_index = k_cinematic_title_none;
                    slot->ticks = k_cinematic_title_none;
                }
            }
        }
    }
}

}

extern "C" {

void cutscene_start(void)
{
    halo::cutscene::CutsceneDirector::start();
}

void cutscene_stop(void)
{
    halo::cutscene::CutsceneDirector::stop();
}

void cutscene_title_queue(int16_t title_index, float delay_seconds)
{
    halo::cutscene::CutsceneDirector::title_queue(title_index, delay_seconds);
}

void chimera__letterbox(void)
{
    halo::cutscene::CutsceneDirector::letterbox();
}

}
