#if defined _amxx_mqtt_client
#endinput
#endif
#define _amxx_mqtt_client

#pragma reqlib mqtt_client
#if !defined AMXMODX_NOAUTOLOAD
#pragma loadlib mqtt_client
#endif

enum MqttHandle
{
    MqttHandle_Invalid = 0
}

enum MqttReasonCode {
    SUCCESS = 0,
    NORMAL_DISCONNECTION = 0,
    GRANTED_QOS_0 = 0,
    GRANTED_QOS_1 = 1,
    GRANTED_QOS_2 = 2,
    DISCONNECT_WITH_WILL_MESSAGE = 4,
    NO_MATCHING_SUBSCRIBERS = 16,
    NO_SUBSCRIPTION_FOUND = 17,
    CONTINUE_AUTHENTICATION = 24,
    RE_AUTHENTICATE = 25,
    UNSPECIFIED_ERROR = 128,
    MALFORMED_PACKET = 129,
    PROTOCOL_ERROR = 130,
    IMPLEMENTATION_SPECIFIC_ERROR = 131,
    UNSUPPORTED_PROTOCOL_VERSION = 132,
    CLIENT_IDENTIFIER_NOT_VALID = 133,
    BAD_USER_NAME_OR_PASSWORD = 134,
    NOT_AUTHORIZED = 135,
    SERVER_UNAVAILABLE = 136,
    SERVER_BUSY = 137,
    BANNED = 138,
    SERVER_SHUTTING_DOWN = 139,
    BAD_AUTHENTICATION_METHOD = 140,
    KEEP_ALIVE_TIMEOUT = 141,
    SESSION_TAKEN_OVER = 142,
    TOPIC_FILTER_INVALID = 143,
    TOPIC_NAME_INVALID = 144,
    PACKET_IDENTIFIER_IN_USE = 145,
    PACKET_IDENTIFIER_NOT_FOUND = 146,
    RECEIVE_MAXIMUM_EXCEEDED = 147,
    TOPIC_ALIAS_INVALID = 148,
    PACKET_TOO_LARGE = 149,
    MESSAGE_RATE_TOO_HIGH = 150,
    QUOTA_EXCEEDED = 151,
    ADMINISTRATIVE_ACTION = 152,
    PAYLOAD_FORMAT_INVALID = 153,
    RETAIN_NOT_SUPPORTED = 154,
    QOS_NOT_SUPPORTED = 155,
    USE_ANOTHER_SERVER = 156,
    SERVER_MOVED = 157,
    SHARED_SUBSCRIPTIONS_NOT_SUPPORTED = 158,
    CONNECTION_RATE_EXCEEDED = 159,
    MAXIMUM_CONNECT_TIME = 160,
    SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED = 161,
    WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED = 162,
    // This is not a protocol code; used internally by the library (obsolete)
    MQTTPP_V3_CODE = 0
}

/**
 * Create a client
 *
 * @note Needs to be freed using mqtt_destroy() native.
 *
 * @param blocker the mqtt blocker server
 * @param client_id client id
 *
 * @return client handle
 */
native MqttHandle:mqtt_create(const blocker[], const client_id[]);

/**
 * Destroy a client
 *
 * @param handle which client
 *
 * @noreturn
 */
native mqtt_destroy(const MqttHandle:handle);

/**
 * Connect to blocker
 *
 * @param handle which client
 *
 * @noreturn
 */
native mqtt_connect(const MqttHandle:handle);

/**
 * Check client connection
 *
 * @note DO NOT CALL IT IN `connected_callback` !!!
 *
 * @param handle which client
 *
 * @return true for connected, false for otherwise
 */
native bool:mqtt_is_connect(const MqttHandle:handle);

/**
 * Subscribe topic
 *
 * @note It should check connect first but in `connected_callback` DON'T DO IT
 *
 * @param handle which client
 * @param topicName topic for subscribe
 *
 * @noreturn
 */
native mqtt_subscribe(const MqttHandle:handle, const topicName[]);

/**
 * Unsubscribe topic
 *
 * @note It should check connect first but in `connected_callback` DON'T DO IT
 *
 * @param handle which client
 * @param topicName topic for unsubscribe
 *
 * @noreturn
 */
native mqtt_unsubscribe(const MqttHandle:handle, const topicName[]);

/**
 * Publish message to topic
 *
 * @note It should check connect first but in `connected_callback` DON'T DO IT
 *
 * @param handle which client
 * @param topicName topic to publish
 *
 * @noreturn
 */
native mqtt_publish(const MqttHandle:handle, const topicName[], const data[]);

/**
 * The `callback` will be called when client connected
 *
 * callback should be: public on_connected(const MqttHandle:h)
 *
 * @param handle which client
 * @param callback target function
 *
 * @noreturn
 */
native mqtt_set_connected_callback(const MqttHandle:handle, const callback[]);

/**
 * The `callback` will be called when client receive message
 *
 * @note callback should be: public on_message(const MqttHandle:h, const message[])
 *
 * @param handle which client
 * @param callback target function
 *
 * @noreturn
 */
native mqtt_set_message_callback(const MqttHandle:handle, const callback[]);
