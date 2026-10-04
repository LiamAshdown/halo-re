#include "halo/interface/ifr1_chat_dialog.hpp"
#include "halo/interface/chat_gui.hpp"
#include "halo/core/datum.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/text/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/interface/records.hpp"
#include "halo/interface/wide_text.hpp"
#include <string.h>
#include <wchar.h>
#include <windows.h>

#ifdef interface
#undef interface
#endif

static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &hud_text_draw_color_or_flags = halo::link::ref<uint16_t>(halo::ui::vars().hud_text_draw_color_or_flags);
static auto &hud_text_draw_font_tag_id = halo::link::ref<int32_t>(halo::ui::vars().hud_text_draw_font_tag_id);
static auto &hud_text_draw_color_a = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_a);
static auto &hud_text_draw_color_r = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_r);
static auto &hud_text_draw_color_g = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_g);
static auto &hud_text_draw_color_b = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_b);

namespace halo::interface {

namespace {

/** Layout of the chat UI in 640x480 interface coordinates. */
constexpr int16_t k_log_left = 5;
constexpr int16_t k_log_top = 260;
constexpr int16_t k_edit_top = 462;
constexpr int16_t k_edit_prompt_left = 5;
constexpr int16_t k_edit_text_left = 90;
constexpr int16_t k_screen_right = 640;

/** Number of trailing characters of the input line that are drawn, so a long line scrolls inside its box. */
constexpr int32_t k_edit_visible_chars = 60;

constexpr uint32_t k_caret_blink_ms = 500;
constexpr uint32_t k_vk_back = 0x08;
constexpr uint32_t k_vk_delete = 0x2e;
constexpr uint32_t k_char_return = 0x0d;
constexpr uint32_t k_char_escape = 0x1b;
constexpr uint32_t k_char_delete = 0x7f;
constexpr uint32_t k_char_first_printable = 0x20;

/** Points the text renderer at the terminal font and white, and returns that font's line height. */
int16_t begin_text()
{
    GlobalsInterfaceBitmaps *interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                                                     ? nullptr
                                                     : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    int32_t font_id = halo::interface::tag_handle(interface_bitmaps->font_terminal.tag_id);
    Font *font = (Font *)halo::cache::globals().tag_instances[(uint16_t)font_id].data;

    hud_text_draw_color_a = 1.0f;
    hud_text_draw_color_r = 1.0f;
    hud_text_draw_color_g = 1.0f;
    hud_text_draw_color_b = 1.0f;
    hud_text_draw_color_or_flags = halo::k_word_none;
    halo::text::globals().hud_text_draw_column = 0;
    halo::text::globals().hud_text_draw_flags = 0;
    hud_text_draw_font_tag_id = font_id;
    return static_cast<int16_t>(font->ascending_height + font->descending_height + font->leading_height);
}

void draw_line(int16_t left, int16_t top, int16_t line_height, const wchar_t *text)
{
    Rectangle2D bounds;

    bounds.top = top;
    bounds.left = left;
    bounds.bottom = static_cast<int16_t>(top + line_height);
    bounds.right = k_screen_right;
    draw_text16(nullptr, &bounds, reinterpret_cast<const uint16_t *>(text));
}

}  // namespace

ChatGui &ChatGui::get()
{
    static ChatGui instance;
    return instance;
}

void ChatGui::open_edit(const wchar_t *prompt)
{
    wcsncpy(prompt_, prompt, sizeof(prompt_) / sizeof(prompt_[0]) - 1);
    prompt_[sizeof(prompt_) / sizeof(prompt_[0]) - 1] = 0;
    edit_text_[0] = 0;
    edit_length_ = 0;
    edit_open_ = true;
}

void ChatGui::close_edit()
{
    edit_open_ = false;
}

bool ChatGui::handle_message(uint32_t message, uint32_t wparam)
{
    if (!edit_open_) {
        return false;
    }

    if (message == k_wm_char) {
        wchar_t character;
        char narrow;

        if (wparam < k_char_first_printable || wparam == k_char_delete) {
            return wparam != k_char_return && wparam != k_char_escape;
        }
        // The window is an ANSI window, so typed characters arrive in the system code page.
        narrow = static_cast<char>(wparam);
        if (MultiByteToWideChar(CP_ACP, 0, &narrow, 1, &character, 1) == 1 && edit_length_ < k_edit_chars - 1) {
            edit_text_[edit_length_++] = character;
            edit_text_[edit_length_] = 0;
        }
        return true;
    }

    if (message == k_wm_keydown && (wparam == k_vk_back || wparam == k_vk_delete)) {
        if (edit_length_ > 0) {
            edit_text_[--edit_length_] = 0;
        }
        return true;
    }
    return false;
}

void ChatGui::add_line(const wchar_t *text)
{
    if (line_count_ == k_max_lines) {
        remove_oldest_line();
    }
    wcsncpy(lines_[line_count_], text, k_line_chars - 1);
    lines_[line_count_][k_line_chars - 1] = 0;
    line_count_++;
}

void ChatGui::remove_oldest_line()
{
    if (line_count_ == 0) {
        return;
    }
    memmove(lines_[0], lines_[1], sizeof(lines_[0]) * (line_count_ - 1));
    line_count_--;
}

void ChatGui::clear_lines()
{
    line_count_ = 0;
}

void ChatGui::draw()
{
    if ((!log_visible_ || line_count_ == 0) && !edit_open_) {
        return;
    }

    int16_t line_height = begin_text();

    if (log_visible_) {
        for (int32_t i = 0; i < line_count_; i++) {
            draw_line(k_log_left, static_cast<int16_t>(k_log_top + i * line_height), line_height, lines_[i]);
        }
    }

    if (edit_open_) {
        wchar_t shown[k_edit_visible_chars + 2];
        int32_t first = edit_length_ > k_edit_visible_chars ? edit_length_ - k_edit_visible_chars : 0;
        int32_t length = edit_length_ - first;

        memcpy(shown, edit_text_ + first, length * sizeof(wchar_t));
        if ((GetTickCount() / k_caret_blink_ms) % 2 == 0) {
            shown[length++] = L'_';
        }
        shown[length] = 0;

        draw_line(k_edit_prompt_left, k_edit_top, line_height, prompt_);
        draw_line(k_edit_text_left, k_edit_top, line_height, shown);
    }
}

}  // namespace halo::interface
