#include "halo/hs/hs3_objects.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"


namespace halo::hs::part3 {

/**
 * If `object_name_index` currently caches a resolved object that fails hs_object_hierarchy_test, invalidates it
 * via object_delete. Either way, if the slot is still out of range or unresolved afterward, falls through to the
 * object_new_from_scenario_name resolve path.
 *
 * @address 0x487d20
 */
void ScriptObjects::object_name_cache_validate(int16_t object_name_index) const
{
    datum_index cached;

    if (object_name_index == -1) {
        return;
    }
    if (-1 < object_name_index) {
        if (object_name_index < 0x200) {
            cached = halo::objects::globals().object_name_list[object_name_index];
            if (cached != k_datum_index_none && halo::hs::hs_object_hierarchy_test(cached) == 0) {
                halo::objects::object_delete(cached);
            }
        }
        if (-1 < object_name_index && object_name_index < 0x200 &&
            halo::objects::globals().object_name_list[object_name_index] != k_datum_index_none) {
            return;
        }
    }
    halo::objects::object_new_from_scenario_name(object_name_index);
}

}
