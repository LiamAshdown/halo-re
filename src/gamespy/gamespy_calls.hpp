/**
 * @file src/gamespy/gamespy_calls.hpp
 * C++ view of the vendored GameSpy SDK entry points the engine calls (the SDK stays C, so the calls keep C linkage). This is
 * the only place outside the SDK that declares them.
 */
#pragma once

#include <stdint.h>

struct network_receive_queue;
struct network_listen_accept_config;

extern "C" {
void KeyValCompareKeyA(const void *a, const void *b);
int32_t NNBeginNegotiationWithSocket(int32_t hostname, int32_t request_id, int32_t one, void (*progress_callback)(void), void (*complete_callback)(int32_t, uint32_t, uint8_t *), int32_t zero);
void NNCancel(datum_index tag);
void NNProcessData(char *data, int32_t len, void *fromaddr);
void NegotiateThink(void *element);
int32_t SBServerDirectConnect(int32_t handle);
int32_t SBServerGetBoolValue(void *entry, const char *key, int32_t default_value);
int32_t SBServerGetIntValue(void *entry, const char *key, int32_t default_value);
int32_t SBServerGetPing(void *entry);
char * SBServerGetPlayerStringValue(void *entry, int32_t index, const char *key, const char *default_value);
char *SBServerGetPrivateAddress(int32_t handle);
uint32_t SBServerGetPrivateQueryPort(int32_t handle);
char *SBServerGetPublicAddress(int32_t handle);
uint32_t SBServerGetPublicQueryPort(int32_t handle);
char * SBServerGetStringValue(void *entry, const char *key, const char *default_value);
int32_t SBServerHasBasicKeys(void *server);
int32_t SBServerHasFullKeys(void *server);
int32_t SBServerHasPrivateAddress(int32_t handle);
int32_t ServerBrowserAuxUpdateServer(void *engine, void *server_record, int32_t flag_a, int32_t flag_b);
void ServerBrowserClear(void *engine);
int32_t ServerBrowserCount(void *engine);
void ServerBrowserFree(void *sb);
char *ServerBrowserGetMyPublicIP(void *handle);
uint32_t ServerBrowserGetMyPublicIPAddr(void *handle);
int32_t ServerBrowserGetServer(void *query_engine, int32_t index);
void ServerBrowserHalt(void *engine);
int32_t ServerBrowserLANUpdate(void *engine, int32_t flag, uint32_t address, uint16_t port);
void ServerBrowserSendNatNegotiateCookieToServer(void *handle, char *hostname, uint32_t port, int32_t request_id);
int32_t ServerBrowserState(void *engine);
int32_t ServerBrowserThink(void *engine);
void gcd_authenticate_user(int32_t game_id, int32_t local_id, uint32_t ip, const char *challenge, const char *response, void *callback, void *instance);
void gcd_compute_response(void *a, void *request, uint8_t *out);
void gcd_disconnect_all(int32_t connection_id);
void gcd_disconnect_user(int32_t id, int32_t value);
char *gcd_getkeyhash(int32_t connection_id, int32_t identity_lookup_key);
void gcd_init_qr2(void *qrec, int32_t game_id, int32_t use_network);
void gcd_shutdown(void);
void gcd_think(void);
void ghttpCancelRequest(int32_t request_id);
int32_t ghttpCleanup(void);
void ghttpSetProxy(void *proxy_settings);
void ghttpStartup(void);
void ghttpThink(void);
int32_t gt2Accept(int32_t reply_socket, network_listen_accept_config *config);
void gt2AddressToString(uint32_t address, uint16_t port, void *out_address);
void gt2CloseAllConnections(void *socket);
void gt2CloseConnectionHard(int32_t socket);
void gt2CloseSocket(int32_t socket);
int gt2Connect(void *socket, void **connection_out, const char *remote_address, const unsigned char *message, int len, unsigned long timeout, const void *callbacks, int blocking);
int32_t gt2CreateSocket(int32_t *socket_out, uint8_t address_buffer[24], int32_t unused_a, int32_t unused_b, void *receive_callback);
void *gt2GetConnectionData(void *gamespy_connection);
int32_t gt2GetConnectionState(int32_t socket);
uint32_t gt2GetLocalIP(int32_t socket);
uint16_t gt2GetLocalPort(int32_t socket);
uint16_t gt2GetRemotePort(int32_t object);
void gt2GetSocketData(int32_t listen_handle);
int32_t gt2Listen(int32_t socket, void *callback);
int gt2NetworkToHostInt(unsigned int value);
uint32_t gt2NetworkToHostShort(int16_t value);
void gt2Reject(int32_t socket, void *buffer, int32_t length);
int32_t gt2Send(int32_t socket, uint8_t *buffer, int32_t byte_count, int32_t mode);
void gt2SetConnectionData(int32_t socket, network_receive_queue *queue);
void gt2SetReceiveDump(int32_t socket, void *callback);
void gt2SetSendDump(int32_t socket, void *callback);
void gt2SetSocketData(int32_t socket, void *data);
void gt2SetUnrecognizedMessageCallback(int32_t socket, void *callback);
void gt2Think(int32_t socket);
void qr2_buffer_add(void *buffer, const char *value);
void qr2_buffer_add_int(void *buffer, int32_t value);
int32_t qr2_init_socketA(void **qrec_out, uint32_t socket, int32_t port, const char *gamename, const char *secret_key, int32_t ispublic, int32_t natnegotiate, void *server_key, void *player_key, void *team_key, void *key_list, void *count, void *adderror, void *userdata);
void qr2_keybuffer_add(void *keybuffer, int32_t key_id);
void qr2_parse_queryA(void *qrec, char *query, int32_t len, void *sender);
void qr2_register_natneg_callback(void *qrec, void *callback);
void qr2_send_statechanged(void *object);
void qr2_shutdown(void *object);
void qr2_think(void *object);
}
