#include "win32.h"
#include "halo/shell/window.hpp"
#include "interface.h"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/shell/api.hpp"

static auto &default_locale_name = halo::link::ref<uint8_t [2]>(halo::shell::vars().default_locale_name);
static auto &locale_codepage_format = halo::link::ref<char [4]>(halo::shell::vars().locale_codepage_format);

namespace halo::shell {

/**
 * Switches the C runtime's character type locale to the system code page when it is still the default
 * "C" locale, so the multibyte conversions see the user's code page.
 */
void CodepageLocale::apply()
{
    uint8_t *current_locale;
    int32_t compare;
    int32_t i;
    uint8_t less_than;
    uint8_t equal;
    char codepage_locale[16];

    current_locale = (uint8_t *)setlocale(2, 0);
    compare = 0;
    less_than = 0;
    equal = 1;
    for (i = 0; i < 2; i++) {
        less_than = default_locale_name[i] < current_locale[i];
        equal = default_locale_name[i] == current_locale[i];
        if (!equal) break;
    }
    if (!equal) {
        compare = (1 - less_than) - (less_than != 0);
    }
    if (compare == 0) {
        wsprintfA(codepage_locale, locale_codepage_format, GetACP());
        setlocale(2, codepage_locale);
    }
}

}
