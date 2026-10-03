#include "halo/hs/hs3_machine.hpp"
#include "crt.h"
#include "halo/hs/api.hpp"
#include "halo/core/link.hpp"
#include "halo/hs/vars.hpp"

static auto &hs_enum_definitions = halo::link::ref<hs_enum_definition [5]>(halo::hs::vars().hs_enum_definitions);

namespace halo::hs::part3 {

/**
 * Behaviour of the original `hs_inspect_boolean` function, moved unchanged into the class.
 *
 * @address 0x489aa0
 */
void ValueInspector::inspect_boolean(int16_t type, int32_t value, char *buffer) const
{
    sprintf(buffer, "%s", (uint8_t)value ? "true" : "false");
}

/**
 * Behaviour of the original `hs_inspect_enum` function, moved unchanged into the class.
 *
 * @address 0x489b50
 */
void ValueInspector::inspect_enum(int16_t type, int32_t value, char *buffer) const
{
    sprintf(buffer, "%s", hs_enum_definitions[type - 0x20].names[(int16_t)value]);
}

/**
 * Behaviour of the original `hs_inspect_long` function, moved unchanged into the class.
 *
 * @address 0x489b10
 */
void ValueInspector::inspect_long(int16_t type, int32_t value, char *buffer) const
{
    sprintf(buffer, "%ld", value);
}

/**
 * Behaviour of the original `hs_inspect_real` function, moved unchanged into the class.
 *
 * @address 0x489ad0
 */
void ValueInspector::inspect_real(int16_t type, int32_t value, char *buffer) const
{
    union { int32_t i; float f; } bits;

    bits.i = value;
    sprintf(buffer, "%f", (double)bits.f);
}

/**
 * Behaviour of the original `hs_inspect_short` function, moved unchanged into the class.
 *
 * @address 0x489af0
 */
void ValueInspector::inspect_short(int16_t type, int32_t value, char *buffer) const
{
    sprintf(buffer, "%d", (int32_t)(int16_t)value);
}

/**
 * Behaviour of the original `hs_inspect_string` function, moved unchanged into the class.
 *
 * @address 0x489b30
 */
void ValueInspector::inspect_string(int16_t type, int32_t value, char *buffer) const
{
    sprintf(buffer, "%s", (char *)value);
}

}
