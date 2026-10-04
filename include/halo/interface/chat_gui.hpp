/**
 * @file include/halo/interface/chat_gui.hpp
 * The multiplayer chat user interface: the one-line input box and the short scrolling message log.
 */
#pragma once

#include <stdint.h>

namespace halo::interface {

/**
 * Owns the chat input line and the recent message log, handles the keyboard messages typed into the input line and draws
 * both over the finished frame in 640x480 interface coordinates.
 */
class ChatGui {
public:
    /** Character capacity of the input line, and of a log line, including the terminator. */
    static constexpr int k_edit_chars = 129;
    static constexpr int k_line_chars = 256;
    /** Number of lines the log keeps. */
    static constexpr int k_max_lines = 8;

    static ChatGui &get();

    /** Shows the input line with the given prompt and an empty text. */
    void open_edit(const wchar_t *prompt);
    void close_edit();
    bool edit_open() const { return edit_open_; }
    const wchar_t *edit_text() const { return edit_text_; }

    /**
     * Applies a window keyboard message to the input line while it is open. Returns true when the message was consumed
     * (typed characters and the editing keys), false when it should take its normal path (Enter, Escape and everything else).
     */
    bool handle_message(uint32_t message, uint32_t wparam);

    void add_line(const wchar_t *text);
    void remove_oldest_line();
    void clear_lines();
    void set_log_visible(bool visible) { log_visible_ = visible; }

    /** Draws the log and, when open, the input line. Called once per frame with the interface render state set up. */
    void draw();

private:
    ChatGui() = default;

    wchar_t prompt_[64] = {};
    wchar_t edit_text_[k_edit_chars] = {};
    int32_t edit_length_ = 0;
    bool edit_open_ = false;

    wchar_t lines_[k_max_lines][k_line_chars] = {};
    int32_t line_count_ = 0;
    bool log_visible_ = false;
};

}  // namespace halo::interface
