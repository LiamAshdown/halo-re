#include "halo/ai/actor_behavior.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

static auto &actor_mode_definitions = halo::link::ref<actor_mode_definition [16]>(halo::ai::vars().actor_mode_definitions);
static auto &actor_type_procs = halo::link::ref<void *[16]>(halo::ai::vars().actor_type_procs);

namespace halo::ai {

/**
 * Runs the mode's enter procedure after actor_set_mode has committed the new mode.
 */
void TableActorMode::enter(ActorView actor) const
{
    if (row->enter_proc != 0) {
        ((void (*)(datum_index))row->enter_proc)(actor.actor_index);
    }
}

/**
 * Runs the mode's per-tick decision procedure and returns its "keep transitioning" result, or 0 when the mode has none.
 */
uint8_t TableActorMode::process(ActorView actor) const
{
    if (row->process_proc != 0) {
        return ((uint8_t (*)(datum_index))row->process_proc)(actor.actor_index);
    }
    return 0;
}

/**
 * Runs the outgoing mode's exit procedure.
 */
void TableActorMode::exit(ActorView actor) const
{
    if (row->exit_proc != 0) {
        ((void (*)(datum_index))row->exit_proc)(actor.actor_index);
    }
}

/**
 * Returns the combat grade of the mode: nonzero raises the actor's awareness to 3, zero clamps it to 2.
 */
int16_t TableActorMode::combat_grade() const
{
    return row->combat_grade;
}

/**
 * Returns how many bytes of mode-specific data actor_set_mode copies into the actor.
 */
uint32_t TableActorMode::data_size() const
{
    return row->data_size;
}

/**
 * Returns the mode object for a mode number.
 */
TableActorMode ActorModeRegistry::get(int32_t mode)
{
    return TableActorMode(&actor_mode_definitions[mode]);
}

/**
 * Runs the actor type's per-pass transition procedure when it has one.
 */
void TableActorType::transition(ActorView actor) const
{
    if (row->proc_14 != 0) {
        ((void (*)(datum_index))row->proc_14)(actor.actor_index);
    }
}

/**
 * Returns the type's swarm byte that grenade ally validation compares against the actor's swarm flag.
 */
uint8_t TableActorType::swarm_flag() const
{
    return row->swarm;
}

/**
 * Returns the type object for an actor type number.
 */
TableActorType ActorTypeRegistry::get(int32_t type)
{
    return TableActorType((const actor_type_table_entry *)actor_type_procs[type]);
}

}
