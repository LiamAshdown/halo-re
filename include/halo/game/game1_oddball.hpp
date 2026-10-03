#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Oddball game engine text builders. Stateless service class: every function is a static member and the state
 * it acts on lives in the engine globals.
 */
class Oddball {
public:
    static uint8_t build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count);
    static wchar_t *build_player_text(datum_index player, wchar_t *buffer);
    static wchar_t *build_score_header_text(wchar_t *buffer);

private:
    static const uint16_t *game_text(int16_t index);
    static const uint16_t *place_text(datum_index recipient);
    static uint16_t *multiplayer_text(int16_t index);
};

}
