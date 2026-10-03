#include "halo/game/game2_variants.hpp"

namespace halo::game {

namespace {

constexpr VariantDefaultsEntry k_variant_defaults[] = {
    {&VariantDefaults::assault},
    {&VariantDefaults::classic_accumulation},
    {&VariantDefaults::classic_crazy_king},
    {&VariantDefaults::classic_ctf},
    {&VariantDefaults::classic_ctf_pro},
    {&VariantDefaults::classic_elimination},
    {&VariantDefaults::classic_endurance},
    {&VariantDefaults::classic_invasion},
    {&VariantDefaults::classic_iron_ctf},
    {&VariantDefaults::classic_juggernaut},
    {&VariantDefaults::classic_king},
    {&VariantDefaults::classic_king_pro},
    {&VariantDefaults::classic_oddball},
    {&VariantDefaults::classic_phantoms},
    {&VariantDefaults::classic_race},
    {&VariantDefaults::classic_rally},
    {&VariantDefaults::classic_reverse_tag},
    {&VariantDefaults::classic_rockets},
    {&VariantDefaults::classic_slayer},
    {&VariantDefaults::classic_slayer_pro},
    {&VariantDefaults::classic_snipers},
    {&VariantDefaults::classic_stalker},
    {&VariantDefaults::classic_team_king},
    {&VariantDefaults::classic_team_oddball},
    {&VariantDefaults::classic_team_race},
    {&VariantDefaults::classic_team_rally},
    {&VariantDefaults::classic_team_slayer},
    {&VariantDefaults::crazy_king},
    {&VariantDefaults::juggernaut},
    {&VariantDefaults::king},
    {&VariantDefaults::oddball},
    {&VariantDefaults::race},
    {&VariantDefaults::slayer},
    {&VariantDefaults::stalker},
    {&VariantDefaults::team_king},
    {&VariantDefaults::team_oddball},
    {&VariantDefaults::team_race},
    {&VariantDefaults::team_slayer},
};

}

/**
 * Returns the table of variant-defaults factories and stores its length in out_count.
 */
const VariantDefaultsEntry *variant_defaults_table(int32_t *out_count)
{
    *out_count = (int32_t)(sizeof(k_variant_defaults) / sizeof(k_variant_defaults[0]));
    return k_variant_defaults;
}

}
