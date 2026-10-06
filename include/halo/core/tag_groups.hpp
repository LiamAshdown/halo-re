#pragma once

#include <cstdint>

namespace halo {

/** Packs four characters into a tag group four-character code. */
constexpr uint32_t fourcc(char a, char b, char c, char d) noexcept {
    return (static_cast<uint32_t>(static_cast<unsigned char>(a)) << 24) | (static_cast<uint32_t>(static_cast<unsigned char>(b)) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(c)) << 8) | static_cast<uint32_t>(static_cast<unsigned char>(d));
}

/** Tag group codes the engine code looks tags up by. */
namespace groups {
inline constexpr uint32_t globals = fourcc('m', 'a', 't', 'g');
inline constexpr uint32_t unicode_string_list = fourcc('u', 's', 't', 'r');
inline constexpr uint32_t font = fourcc('f', 'o', 'n', 't');
inline constexpr uint32_t contrail = fourcc('c', 'o', 'n', 't');
inline constexpr uint32_t effect = fourcc('e', 'f', 'f', 'e');
inline constexpr uint32_t sound = fourcc('s', 'n', 'd', '!');
inline constexpr uint32_t light = fourcc('l', 'i', 'g', 'h');
inline constexpr uint32_t damage_effect = fourcc('j', 'p', 't', '!');
inline constexpr uint32_t decal = fourcc('d', 'e', 'c', 'a');
inline constexpr uint32_t object = fourcc('o', 'b', 'j', 'e');
inline constexpr uint32_t particle_system = fourcc('p', 'c', 't', 'l');
inline constexpr uint32_t input_device_defaults = fourcc('d', 'e', 'v', 'c');
inline constexpr uint32_t ui_widget_definition = fourcc('D', 'e', 'L', 'a');
}  // namespace groups

}  // namespace halo
