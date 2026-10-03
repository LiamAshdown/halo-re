#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Strategy interface for one client-bound message type. Instances are constant-initialised objects in static
 * storage and are looked up by the message type byte found at the end of a received record.
 */
class ClientMessageHandler {
public:
    virtual int32_t handle(network_client_globals *client, uint16_t *record, int32_t record_length, const uint32_t *sender) const = 0;

protected:
    ~ClientMessageHandler() = default;
};

/**
 * Registry of the client-bound message handlers (message types 0x02..0x0d, 0x16..0x19, 0x21 and 0x22).
 */
class ClientMessageRegistry {
public:
    ClientMessageRegistry() = delete;

    /**
     * Returns the handler registered for a message type byte, or null when the type has no client handler.
     */
    static const ClientMessageHandler *find(uint8_t message_type);
};

/**
 * State pattern interface for one client connection state; tick runs the per-frame work of that state.
 */
class ClientStateHandler {
public:
    virtual int8_t tick(network_client_globals *client) const = 0;

protected:
    ~ClientStateHandler() = default;
};

/**
 * Maps the client state value (0 handshake, 1 connect retry, 2 lobby, 3 in game, 4 channel service) to its
 * state object.
 */
class ClientStateMachine {
public:
    ClientStateMachine() = delete;

    /**
     * Returns the state object for a client state value, or null for an unknown state.
     */
    static const ClientStateHandler *handler_for(uint16_t state);
};

/**
 * Strategy interface for one server-bound message type received from a machine.
 */
class ServerMessageHandler {
public:
    virtual uint32_t handle(network_server_globals *server, network_machine *machine, network_message_record *bytes, int32_t length) const = 0;

protected:
    ~ServerMessageHandler() = default;
};

/**
 * Registry of the server-bound message handlers (message types 0x01, 0x0e..0x15, 0x1a..0x1e and 0x23..0x25).
 */
class ServerMessageRegistry {
public:
    ServerMessageRegistry() = delete;

    /**
     * Returns the handler registered for a message type byte, or null when the type has no server handler.
     */
    static const ServerMessageHandler *find(uint8_t message_type);
};

}
