#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include <string.h>
#include "units.h"

namespace halo::ai {

/**
 * Behaviour group "alert_mode" of the actor AI: 5 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class alert_mode {
public:
    explicit alert_mode(datum_index value) : datum(value) {}

    void movement_cancelled();
    uint8_t process();
    void target_cleared();
    void tick();
    void update();

    datum_index datum;
};

/**
 * Behaviour group "avoid_mode" of the actor AI: 1 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class avoid_mode {
public:
    explicit avoid_mode(datum_index value) : datum(value) {}

    void update();

    datum_index datum;
};

/**
 * Behaviour group "converse_mode" of the actor AI: 4 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class converse_mode {
public:
    explicit converse_mode(datum_index value) : datum(value) {}

    void exit();
    uint8_t process();
    void replace_reference(datum_index old_reference, datum_index new_reference);
    void update();

    datum_index datum;
};

/**
 * Behaviour group "obey_mode" of the actor AI: 5 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class obey_mode {
public:
    explicit obey_mode(datum_index value) : datum(value) {}

    void enter();
    void exit();
    uint8_t process();
    void tick_members();
    void update();

    datum_index datum;
};

/**
 * Behaviour group "search_mode" of the actor AI: 5 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class search_mode {
public:
    explicit search_mode(datum_index value) : datum(value) {}

    void enter();
    void movement_cancelled();
    uint8_t process();
    void tick();
    void update();

    datum_index datum;
};

/**
 * Behaviour group "sleep_mode" of the actor AI: 1 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class sleep_mode {
public:
    explicit sleep_mode(datum_index value) : datum(value) {}

    void update();

    datum_index datum;
};

/**
 * Behaviour group "charge_mode" of the actor AI: 4 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class charge_mode {
public:
    explicit charge_mode(datum_index value) : datum(value) {}

    void enter();
    uint8_t process();
    void tick();
    void update();

    datum_index datum;
};

/**
 * Behaviour group "fight_mode" of the actor AI: 2 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class fight_mode {
public:
    explicit fight_mode(datum_index value) : datum(value) {}

    void tick();
    void update();

    datum_index datum;
};

/**
 * Behaviour group "flee_mode" of the actor AI: 8 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class flee_mode {
public:
    explicit flee_mode(datum_index value) : datum(value) {}

    void enter();
    void exit();
    void get_look_weights(float *out_weights);
    void movement_cancelled();
    uint8_t process();
    void replace_reference(datum_index old_reference, datum_index new_reference);
    void tick();
    void update();

    datum_index datum;
};

/**
 * Behaviour group "guard_mode" of the actor AI: 8 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class guard_mode {
public:
    explicit guard_mode(datum_index value) : datum(value) {}

    void enter();
    void exit();
    void get_look_weights(float *out_weights);
    void movement_cancelled();
    void replace_reference(datum_index old_reference, datum_index new_reference);
    void target_cleared();
    void tick();
    void update();

    datum_index datum;
};

/**
 * Behaviour group "uncover_mode" of the actor AI: 3 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class uncover_mode {
public:
    explicit uncover_mode(datum_index value) : datum(value) {}

    void enter();
    void get_look_weights(float *out_weights);
    void movement_cancelled();

    datum_index datum;
};

}
