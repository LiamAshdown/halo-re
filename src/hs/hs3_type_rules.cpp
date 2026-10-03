#include "halo/hs/hs3_machine.hpp"
#include "halo/hs/api.hpp"

extern "C" {
extern uint16_t hs_object_type_masks[6];
extern int32_t (*hs_type_conversion_procedures[k_hs_type_count][k_hs_type_count])(int32_t value);
}

namespace halo::hs::part3 {

/**
 * Tests whether console-context bit `bit_index` (of `flags`) is satisfied by hs_autocomplete_gametype_mask:
 * passes if the bit is not required or is present, and if the bit is not forbidden or is absent.
 *
 * @address 0x4835b0
 */
char TypeRules::gametype_flag_satisfied(uint8_t bit_index, uint8_t flags) const
{
    uint32_t bit_mask;
    uint8_t bit_mask_byte;
    char satisfied;

    bit_mask = 1u << (bit_index & 0x1f);
    bit_mask_byte = (uint8_t)bit_mask;
    satisfied = 1;
    if ((((bit_mask & (uint16_t)halo::hs::globals().autocomplete_gametype_mask) == 0) ||
         (satisfied = (char)((flags & bit_mask_byte) != 0), satisfied != 0)) &&
        (((uint16_t)halo::hs::globals().autocomplete_gametype_mask & (1u << ((bit_index + 8) & 0x1f))) != 0)) {
        satisfied = (char)(1 - ((flags & bit_mask_byte) != 0));
    }
    return satisfied;
}

/**
 * Returns 1 if every one of the console command context bits (0 through _hs_context_always_bit) required or
 * forbidden by hs_autocomplete_gametype_mask is satisfied by `flags`, 0 otherwise.
 *
 * @address 0x483600
 */
uint8_t TypeRules::gametype_flags_applicable(uint8_t flags) const
{
    uint8_t result;

    result = 1;
    if ((int16_t)halo::hs::globals().autocomplete_gametype_mask != 0) {
        if (((halo::hs::globals().autocomplete_gametype_mask & 1) == 0) ||
            (result = (uint8_t)(flags & 1), result != 0)) {
            if ((halo::hs::globals().autocomplete_gametype_mask & 0x100) != 0) {
                result = (uint8_t)(~flags & 1);
            }
            if (result != 0) {
                if ((halo::hs::hs_gametype_flag_satisfied(1, flags) != 0) &&
                    (halo::hs::hs_gametype_flag_satisfied(2, flags) != 0) &&
                    (halo::hs::hs_gametype_flag_satisfied(3, flags) != 0) &&
                    (halo::hs::hs_gametype_flag_satisfied(4, flags) != 0) &&
                    (halo::hs::hs_gametype_flag_satisfied(6, flags) != 0) &&
                    (halo::hs::hs_gametype_flag_satisfied(5, flags) != 0)) {
                    return 1;
                }
            }
        }
        result = 0;
    }
    return result;
}

/**
 * Behaviour of the original `hs_string_is_empty` function, moved unchanged into the class.
 *
 * @address 0x48aaf0
 */
char TypeRules::string_is_empty(char *s) const
{
    return s[0] == '\0';
}

/**
 * Returns 1 if every bit set in hs_object_type_masks[subtype_index] is also set in
 * hs_object_type_masks[supertype_index] (i.e. subtype_index's object family is a subset of supertype_index's),
 * used for polymorphic type compatibility such as 'object' encompassing 'unit'/'vehicle'.
 *
 * @address 0x48ac60
 */
char TypeRules::type_mask_is_subset(int16_t subtype_index, int16_t supertype_index) const
{
    uint16_t subtype_mask;

    subtype_mask = hs_object_type_masks[subtype_index];
    return (hs_object_type_masks[supertype_index] & subtype_mask) == subtype_mask;
}

/**
 * A value of `source_type` can be used where `dest_type` is expected if: they are the same type, or source_type
 * is "passthrough"; or (for ordinary value types) the conversion table has a procedure for
 * [dest_type][source_type]; or (for the object and object-name families) one type's bitmask is a subset of the
 * other's, via hs_type_mask_is_subset.
 *
 * @address 0x48ac90
 */
char TypeRules::types_are_compatible(hs_type_t dest_type, hs_type_t source_type) const
{
    if (source_type == _hs_type_passthrough || source_type == dest_type) {
        return 1;
    }
    if (0x24 < dest_type && dest_type < 0x2b) {
        if (0x24 < source_type && source_type < 0x2b) {
            return halo::hs::hs_type_mask_is_subset(source_type - 0x25, dest_type - 0x25);
        }
        if (0x2a < source_type && source_type < 0x31) {
            return halo::hs::hs_type_mask_is_subset(source_type - 0x2b, dest_type - 0x25);
        }
        return 0;
    }
    if (0x2a < dest_type && dest_type < 0x31) {
        if (source_type < 0x2b || 0x30 < source_type) {
            return 0;
        }
        return halo::hs::hs_type_mask_is_subset(source_type - 0x2b, dest_type - 0x2b);
    }
    return hs_type_conversion_procedures[dest_type][source_type] != 0;
}

}
