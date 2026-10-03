#include "halo/hs/hs3_objects.hpp"

extern "C" {
extern int32_t strstr(ScenarioObjectName *entry, uint32_t predicate_arg);
extern Scenario *global_scenario;
}

namespace halo::hs::part3 {

/**
 * Invokes `callback(index)` for every entry of Scenario::object_names that satisfies the strstr predicate, in
 * ascending index order.
 *
 * @address 0x487ef0
 */
void ScriptObjects::object_names_for_each(void (*callback)(int32_t index), uint32_t predicate_arg) const
{
    ScenarioObjectName *object_names;
    int32_t index;

    object_names = (ScenarioObjectName *)global_scenario->object_names.pointer;
    for (index = 0; index < (int32_t)global_scenario->object_names.count; index++) {
        if (strstr(&object_names[index], predicate_arg) != 0) {
            callback(index);
        }
    }
}

}
