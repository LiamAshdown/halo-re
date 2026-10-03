/**
 * @file standalone/data/link/memory.hpp
 * Link names of the engine variables the memory module binds in halo::memory::Globals (src/memory/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern crc32_table crc32_lookup_table;
extern uint8_t crc32_lookup_table_initialized;
extern const char *data_packet_group_error;
extern byte_swap_definition packet_header_byte_swap_definition;
extern uint8_t bit_mask_keep[9];
extern uint8_t bit_mask_clear[8];
}
