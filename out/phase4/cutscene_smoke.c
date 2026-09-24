#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// recorded_animation, unit_control_data_field_layout and recorded_animation_codec hold
// pointers, so their size checks are gated on PTRS32 and only fire with -m32.
// Run both:  gcc -fsyntax-only -I types out/phase4/cutscene_smoke.c
//            gcc -m32 -fsyntax-only -I types out/phase4/cutscene_smoke.c
#define PTRS32 (sizeof(void *) == 4)
#define OFF(t, f) __builtin_offsetof(t, f)

// cinematic globals: 0x1c from the game state allocation in 0x45a9c0
typedef char c_cg_size[(sizeof(cinematic_globals) == 0x1c) ? 1 : -1];
typedef char c_ts_size[(sizeof(cinematic_title_slot) == 0x04) ? 1 : -1];
typedef char c_cg_scale[(OFF(cinematic_globals, letterbox_scale) == 0x00) ? 1 : -1];
typedef char c_cg_tick[(OFF(cinematic_globals, letterbox_last_tick) == 0x04) ? 1 : -1];
typedef char c_cg_show[(OFF(cinematic_globals, show_letterbox) == 0x08) ? 1 : -1];
typedef char c_cg_prog[(OFF(cinematic_globals, in_progress) == 0x09) ? 1 : -1];
typedef char c_cg_skip[(OFF(cinematic_globals, skip_in_progress) == 0x0a) ? 1 : -1];
typedef char c_cg_bsp[(OFF(cinematic_globals, suppress_bsp_object_creation) == 0x0b) ? 1 : -1];
typedef char c_cg_titles[(OFF(cinematic_globals, titles) == 0x0c) ? 1 : -1];
typedef char c_cg_ticks0[(OFF(cinematic_globals, titles[0].ticks) == 0x0e) ? 1 : -1];
typedef char c_cg_title3[(OFF(cinematic_globals, titles[3]) == 0x18) ? 1 : -1];

// recorded animation codec pieces, sizes from the bysw records
typedef char c_ang[(sizeof(recorded_animation_angles) == 0x04) ? 1 : -1];
typedef char c_state[(sizeof(recorded_animation_decoder_state) == 0x0c) ? 1 : -1];
typedef char c_state_aim[(OFF(recorded_animation_decoder_state, aiming) == 0x04) ? 1 : -1];
typedef char c_state_look[(OFF(recorded_animation_decoder_state, looking) == 0x08) ? 1 : -1];
typedef char c_cdiff[(sizeof(recorded_animation_char_difference) == 0x02) ? 1 : -1];
typedef char c_sdiff[(sizeof(recorded_animation_short_difference) == 0x04) ? 1 : -1];
typedef char c_ev1[(sizeof(recorded_animation_event_v1) == 0x04) ? 1 : -1];
typedef char c_ev1_as[(sizeof(recorded_animation_animation_state_set_event_v1) == 0x06) ? 1 : -1];
typedef char c_ev1_sp[(sizeof(recorded_animation_aiming_speed_set_event_v1) == 0x06) ? 1 : -1];
typedef char c_ev1_cf[(sizeof(recorded_animation_control_flags_set_event_v1) == 0x06) ? 1 : -1];
typedef char c_ev1_wi[(sizeof(recorded_animation_weapon_index_set_event_v1) == 0x06) ? 1 : -1];
typedef char c_ev1_th[(sizeof(recorded_animation_throttle_set_event_v1) == 0x0c) ? 1 : -1];
typedef char c_ev1_mv[(sizeof(recorded_animation_multi_vector_set_event_v1) == 0x10) ? 1 : -1];
typedef char c_ev1_av[(sizeof(recorded_animation_angle_vector_set_event_v1) == 0x0c) ? 1 : -1];
typedef char c_ev1_av_pitch[(OFF(recorded_animation_angle_vector_set_event_v1, angles.pitch) == 0x08) ? 1 : -1];
typedef char c_layout[(!PTRS32 || (sizeof(unit_control_data_field_layout) == 0x0c)) ? 1 : -1];
typedef char c_layout_off[(!PTRS32 || (OFF(unit_control_data_field_layout, offset) == 0x08)) ? 1 : -1];
typedef char c_codec[(!PTRS32 || (sizeof(recorded_animation_codec) == 0x08)) ? 1 : -1];

// playback record: stride 100 in 0x44aa90, entry point argument addresses
typedef char c_ra[(!PTRS32 || (sizeof(recorded_animation) == 0x64)) ? 1 : -1];
typedef char c_ra_unit[(OFF(recorded_animation, unit_index) == 0x04) ? 1 : -1];
typedef char c_ra_ticks[(OFF(recorded_animation, ticks_remaining) == 0x08) ? 1 : -1];
typedef char c_ra_flags[(OFF(recorded_animation, flags) == 0x0a) ? 1 : -1];
typedef char c_ra_evt[(OFF(recorded_animation, event_ticks) == 0x0c) ? 1 : -1];
typedef char c_ra_cur[(OFF(recorded_animation, event_cursor) == 0x10) ? 1 : -1];
typedef char c_ra_ctl[(!PTRS32 || (OFF(recorded_animation, control_data) == 0x14)) ? 1 : -1];
typedef char c_ra_st[(!PTRS32 || (OFF(recorded_animation, decoder_state) == 0x54)) ? 1 : -1];
typedef char c_ra_codec[(!PTRS32 || (OFF(recorded_animation, codec_index) == 0x60)) ? 1 : -1];

// the unit_control_data offsets the v1 layout table and the handlers write (types/units.h)
typedef char c_ucd_size[(sizeof(unit_control_data) == 0x40) ? 1 : -1];
typedef char c_ucd_flags[(OFF(unit_control_data, control_flags) == 0x02) ? 1 : -1];
typedef char c_ucd_weapon[(OFF(unit_control_data, weapon_index) == 0x04) ? 1 : -1];
typedef char c_ucd_grenade[(OFF(unit_control_data, grenade_index) == 0x06) ? 1 : -1];
typedef char c_ucd_zoom[(OFF(unit_control_data, zoom_level) == 0x08) ? 1 : -1];
typedef char c_ucd_throttle[(OFF(unit_control_data, throttle) == 0x0c) ? 1 : -1];
typedef char c_ucd_trigger[(OFF(unit_control_data, primary_trigger) == 0x18) ? 1 : -1];
typedef char c_ucd_facing[(OFF(unit_control_data, facing_vector) == 0x1c) ? 1 : -1];
typedef char c_ucd_aiming[(OFF(unit_control_data, aiming_vector) == 0x28) ? 1 : -1];
typedef char c_ucd_looking[(OFF(unit_control_data, looking_vector) == 0x34) ? 1 : -1];

// tag layouts the module reads (types/tags.h)
typedef char c_scn_recanim[(OFF(Scenario, recorded_animations) == 0x36c) ? 1 : -1];
typedef char c_scn_titles[(OFF(Scenario, cutscene_titles) + OFF(TagReflexive, pointer) == 0x500) ? 1 : -1];
typedef char c_scn_help[(OFF(Scenario, ingame_help_text) + OFF(TagDependency, tag_id) == 0x590) ? 1 : -1];
typedef char c_sra_size[(sizeof(ScenarioRecordedAnimation) == 0x40) ? 1 : -1];
typedef char c_sra_ucdv[(OFF(ScenarioRecordedAnimation, unit_control_data_version) == 0x22) ? 1 : -1];
typedef char c_sct_size[(sizeof(ScenarioCutsceneTitle) == 0x60) ? 1 : -1];
typedef char c_sct_string[(OFF(ScenarioCutsceneTitle, string_index) == 0x30) ? 1 : -1];
typedef char c_sct_style[(OFF(ScenarioCutsceneTitle, text_style) == 0x32) ? 1 : -1];
typedef char c_sct_just[(OFF(ScenarioCutsceneTitle, justification) == 0x34) ? 1 : -1];
typedef char c_sct_flags[(OFF(ScenarioCutsceneTitle, text_flags) == 0x38) ? 1 : -1];
typedef char c_sct_color[(OFF(ScenarioCutsceneTitle, text_color) == 0x3c) ? 1 : -1];
typedef char c_sct_shadow[(OFF(ScenarioCutsceneTitle, shadow_color) == 0x40) ? 1 : -1];
typedef char c_sct_fadein[(OFF(ScenarioCutsceneTitle, fade_in_time) == 0x44) ? 1 : -1];
typedef char c_sct_up[(OFF(ScenarioCutsceneTitle, up_time) == 0x48) ? 1 : -1];
typedef char c_sct_fadeout[(OFF(ScenarioCutsceneTitle, fade_out_time) == 0x4c) ? 1 : -1];
typedef char c_hud_font[(OFF(HUDGlobals, fullscreen_font) + OFF(TagDependency, tag_id) == 0x54) ? 1 : -1];
typedef char c_glob_rast[(OFF(Globals, rasterizer_data) == 0x134) ? 1 : -1];
typedef char c_grd_2d[(OFF(GlobalsRasterizerData, default_2d) + OFF(TagDependency, tag_id) == 0xb8) ? 1 : -1];
typedef char c_bitmap_data[(OFF(Bitmap, bitmap_data) + OFF(TagReflexive, pointer) == 0x64) ? 1 : -1];
typedef char c_bitmapdata_size[(sizeof(BitmapData) == 0x30) ? 1 : -1];

int cutscene_smoke(void) { return (int)(sizeof(cinematic_globals) + sizeof(recorded_animation)); }
