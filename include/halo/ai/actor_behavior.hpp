#pragma once

#include "halo/ai/actor_view.hpp"

namespace halo::ai {

/**
 * State interface for one actor mode (guard, search, uncover, wait, vehicle, ...). The engine keeps the per-mode
 * procedures in a fixed table; this interface is what the mode machinery in ActorView talks to instead of indexing
 * that table and casting raw addresses itself.
 *
 * Objects are cheap value types created on the stack by ActorModeRegistry::get and never stored in game state.
 */
class ActorMode {
public:
    virtual void enter(ActorView actor) const = 0;
    virtual uint8_t process(ActorView actor) const = 0;
    virtual void exit(ActorView actor) const = 0;
    virtual int16_t combat_grade() const = 0;
    virtual uint32_t data_size() const = 0;

protected:
    ~ActorMode() = default;
};

/**
 * ActorMode backed by one row of the engine's actor_mode_definitions table. Each hook calls the row's procedure with
 * the actor index when it is non-null and does nothing otherwise, exactly like the original table dispatch.
 */
class TableActorMode final : public ActorMode {
public:
    explicit constexpr TableActorMode(const actor_mode_definition *definition) : row(definition) {}

    void enter(ActorView actor) const override;
    uint8_t process(ActorView actor) const override;
    void exit(ActorView actor) const override;
    int16_t combat_grade() const override;
    uint32_t data_size() const override;

private:
    const actor_mode_definition *row;
};

/**
 * Looks up the ActorMode object for a mode number (the index into the engine's mode table).
 */
class ActorModeRegistry {
public:
    static TableActorMode get(int32_t mode);
};

/**
 * Strategy interface for the per-actor-type hooks (elite, grunt, jackal, flood, sentinel, ...). Only the hook that the
 * mode-transition loop runs on every pass is modelled here.
 */
class ActorTypeBehavior {
public:
    virtual void transition(ActorView actor) const = 0;
    virtual uint8_t swarm_flag() const = 0;

protected:
    ~ActorTypeBehavior() = default;
};

/**
 * ActorTypeBehavior backed by one entry of the engine's actor_type_procs table; the transition hook runs only when
 * the entry's procedure is non-null.
 */
class TableActorType final : public ActorTypeBehavior {
public:
    explicit constexpr TableActorType(const actor_type_table_entry *entry) : row(entry) {}

    void transition(ActorView actor) const override;
    uint8_t swarm_flag() const override;

private:
    const actor_type_table_entry *row;
};

/**
 * Looks up the ActorTypeBehavior object for an actor type number.
 */
class ActorTypeRegistry {
public:
    static TableActorType get(int32_t type);
};

}
