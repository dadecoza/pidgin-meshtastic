#ifndef PURPLE_PLUGINS
#define PURPLE_PLUGINS
#endif

// Glib
#include <glib.h>

// GNU C libraries
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
#include <termios.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#else
#include <windows.h>
#endif

#include <accountopt.h>
#include <core.h>
#include <debug.h>
#include <prpl.h>
#include <request.h>
#include <version.h>

// Protobufs
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include "meshtastic/mesh.pb.h"
#include "meshtastic/admin.pb.h"

#include "mtstrings.h"
#include "meshtastic.h"

#ifdef _WIN32
#include <win32dep.h>
#endif


#define PROTO_NAME "meshtastic"
static PurplePlugin *_mt_protocol = NULL;

/**
 *
 * Protocol Functions
 *
 */
int mt_connect(MeshtasticAccount *mta, char *port, enum connection_type type)
{
    mta->protobuf_size = 0;
    mta->protobuf_remaining = 0;

    if (type == meshtastic_serial_connection)
    {
        purple_debug_info(PROTO_NAME, "Making serial connection\n");
#ifndef _WIN32
        struct termios tty;
        mta->fd = open(port, O_RDWR);
        mta->protobuf_size = 0;
        mta->protobuf_remaining = 0;
        if (tcgetattr(mta->fd, &tty) != 0)
        {
            purple_debug_error(PROTO_NAME, "Error %i from tcgetattr: %s\n", errno, strerror(errno));
            return 1;
        }
        tty.c_cflag &= ~PARENB;        // Clear parity bit, disabling parity (most common)
        tty.c_cflag &= ~CSTOPB;        // Clear stop field, only one stop bit used in communication (most common)
        tty.c_cflag &= ~CSIZE;         // Clear all bits that set the data size
        tty.c_cflag |= CS8;            // 8 bits per byte (most common)
        tty.c_cflag &= ~CRTSCTS;       // Disable RTS/CTS hardware flow control (most common)
        tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
        tty.c_lflag &= ~ICANON;
        tty.c_lflag &= ~ECHO;                                                        // Disable echo
        tty.c_lflag &= ~ECHOE;                                                       // Disable erasure
        tty.c_lflag &= ~ECHONL;                                                      // Disable new-line echo
        tty.c_lflag &= ~ISIG;                                                        // Disable interpretation of INTR, QUIT and SUSP
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // Turn off s/w flow ctrl
        tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes
        tty.c_oflag &= ~OPOST;                                                       // Prevent special interpretation of output bytes (e.g. newline chars)
        tty.c_oflag &= ~ONLCR;                                                       // Prevent conversion of newline to carriage return/line feed
        tty.c_cc[VTIME] = 10;                                                        // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
        tty.c_cc[VMIN] = 0;
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);
        if (tcsetattr(mta->fd, TCSANOW, &tty) != 0)
        {
            purple_debug_error(PROTO_NAME, "Error %i from tcsetattr: %s\n", errno, strerror(errno));
            return 1;
        }
#else
        mta->handle = CreateFileA(port, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (mta->handle == INVALID_HANDLE_VALUE)
        {
            purple_debug_error(PROTO_NAME, "Failed to create handle\n");
            return 1;
        }

        BOOL success = FlushFileBuffers(mta->handle);
        if (!success)
        {
            purple_debug_error(PROTO_NAME, "Failed to flush handle buffers\n");
            CloseHandle(mta->handle);
            return 1;
        }

        // Configure read and write operations to time out after 100 ms.
        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 0;
        timeouts.ReadTotalTimeoutConstant = 100;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 100;
        timeouts.WriteTotalTimeoutMultiplier = 0;

        success = SetCommTimeouts(mta->handle, &timeouts);
        if (!success)
        {
            purple_debug_error(PROTO_NAME, "Failed to configure handle timeouts\n");
            CloseHandle(mta->handle);
            return 1;
        }
        DCB state = {0};
        state.DCBlength = sizeof(DCB);
        state.BaudRate = 115200;
        state.ByteSize = 8;
        state.Parity = NOPARITY;
        state.StopBits = ONESTOPBIT;
        success = SetCommState(mta->handle, &state);
        if (!success)
        {
            purple_debug_error(PROTO_NAME, "Failed to configure serial port\n");
            CloseHandle(mta->handle);
            return 1;
        }
        mta->mt_handle_timeout = purple_timeout_add(100, (GSourceFunc)mt_read_handle, mta);
        return 0;
#endif
    }
    else if (type == meshtastic_socket_connection)
    {
        struct sockaddr_in radio_address;
        purple_debug_info(PROTO_NAME, "Making socket connection\n");
        if ((mta->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            purple_debug_error(PROTO_NAME, "Socket creation error");
            return 1;
        }
        radio_address.sin_family = AF_INET;
        radio_address.sin_port = htons(TCP_PORT);
        if (inet_pton(AF_INET, port, &radio_address.sin_addr) <= 0)
        {
            purple_debug_error(PROTO_NAME, "Invalid address or address not supported\n");
            return 1;
        }
        if ((connect(mta->fd, (struct sockaddr *)&radio_address, sizeof(radio_address))) < 0)
        {
            purple_debug_error(PROTO_NAME, "Connection failed\n");
            return 1;
        }
    }
    else
    {
        purple_debug_error(PROTO_NAME, "Unexpected connection type\n");
        return 1;
    }
    mta->gc->inpa = purple_input_add(mta->fd, PURPLE_INPUT_READ, mt_from_radio_cb, mta);
    return 0; // success
}

int mt_write(MeshtasticAccount *mta, uint8_t *buffer, size_t size)
{
#ifdef _WIN32
    if (mta->handle != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        BOOL success = WriteFile(mta->handle, buffer, size, &written, NULL);
        if (!success)
        {
            return -1;
        }
        return written;
    }
#endif
    return write(mta->fd, buffer, size);
}

int mt_read(MeshtasticAccount *mta, uint8_t *buffer, size_t size)
{
    int len = 0;
#ifdef _WIN32
    if (mta->handle != INVALID_HANDLE_VALUE)
    {

        DWORD received;
        BOOL success = ReadFile(mta->handle, buffer, size, &received, NULL);
        if (!success)
        {
            len = -1;
        }
        else
        {
            len = received;
        }
    }
    else
    {
        len = read(mta->fd, buffer, size);
    }
#else
    len = read(mta->fd, buffer, size);
#endif
    if (len < 0 && errno == EAGAIN)
    {
        return 0;
    }
    else if (len < 0)
    {
        gchar *tmp = g_strdup_printf(_("Lost connection with radio: %s"), g_strerror(errno));
        purple_connection_error_reason(mta->gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
        g_free(tmp);
        return -1;
    }
    else if (len == 0)
    {
        purple_connection_error_reason(mta->gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Radio closed the connection"));
        return -1;
    }
    return len;
}

bool mt_routing_from_packet(meshtastic_MeshPacket *packet, meshtastic_Routing *routing)
{
    pb_istream_t stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    bool status = pb_decode(&stream, meshtastic_Routing_fields, routing);
    return status;
}

// Radio Communication
int mt_send_to_radio(MeshtasticAccount *mta, meshtastic_ToRadio toRadio)
{
    uint8_t buffer[MESHTASTIC_BUFFER_SIZE];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer + 4, MESHTASTIC_BUFFER_SIZE);
    bool status;
    buffer[0] = START_BYTE_1;
    buffer[1] = START_BYTE_2;
    status = pb_encode(&stream, meshtastic_ToRadio_fields, &toRadio);
    if (!status)
    {
        return -1;
    }
    buffer[2] = stream.bytes_written / 256;
    buffer[3] = stream.bytes_written % 256;
    return mt_write(mta, buffer, 4 + stream.bytes_written);
}

int mt_send_packet_to_radio(MeshtasticAccount *mta, meshtastic_MeshPacket packet)
{
    meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_default;
    toRadio.which_payload_variant = meshtastic_ToRadio_packet_tag;
    toRadio.packet = packet;
    return mt_send_to_radio(mta, toRadio);
}

int mt_send_admin(MeshtasticAccount *mta, meshtastic_AdminMessage admin, int32_t dest, bool want_response)
{
    uint8_t buffer[512];
    meshtastic_MeshPacket meshPacket = meshtastic_MeshPacket_init_default;
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, 512);
    pb_encode(&stream, meshtastic_AdminMessage_fields, &admin);
    meshPacket.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    meshPacket.id = rand() % 0x7FFFFFFF;
    meshPacket.decoded.portnum = meshtastic_PortNum_ADMIN_APP;
    meshPacket.to = dest;
    meshPacket.channel = mta->admin_channel;
    meshPacket.want_ack = true;
    meshPacket.decoded.want_response = want_response;
    meshPacket.decoded.payload.size = stream.bytes_written;
    memcpy(meshPacket.decoded.payload.bytes, buffer, stream.bytes_written);
    return mt_send_packet_to_radio(mta, meshPacket);
}

// Request Data
int mt_request_initial_config(MeshtasticAccount *mta)
{
    uint8_t buffer[32];
    memset(buffer, START_BYTE_2, sizeof(buffer));
    mt_write(mta, buffer, 32);
    int want_config_id = rand() % 0x7FffFFff;
    meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_default;
    toRadio.which_payload_variant = meshtastic_ToRadio_want_config_id_tag;
    toRadio.want_config_id = want_config_id;
    mt_send_to_radio(mta, toRadio);
    return 1;
}

int mt_request_session_key(MeshtasticAccount *mta, uint32_t dest)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_get_config_request_tag;
    admin_message.get_config_request = meshtastic_AdminMessage_ConfigType_SESSIONKEY_CONFIG;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_request_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_AdminMessage_ConfigType type)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_get_config_request_tag;
    admin_message.get_config_request = type;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_request_module(MeshtasticAccount *mta, uint32_t dest, meshtastic_AdminMessage_ModuleConfigType type)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_get_module_config_request_tag;
    admin_message.get_module_config_request = type;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_request_position_config(MeshtasticAccount *mta, uint32_t dest)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_get_config_request_tag;
    admin_message.get_config_request = meshtastic_AdminMessage_ConfigType_POSITION_CONFIG;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_request_owner_config(MeshtasticAccount *mta, uint32_t dest)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_get_owner_request_tag;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_request_channel_config(MeshtasticAccount *mta, uint32_t dest, uint32_t channel_index)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_get_channel_request_tag;
    admin_message.get_channel_request = channel_index + 1;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_request_reboot(MeshtasticAccount *mta, uint32_t dest)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_reboot_seconds_tag;
    admin_message.session_passkey = *mta->session_passkey;
    admin_message.reboot_seconds = 2;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_request_reset_nodedb(MeshtasticAccount *mta, uint32_t dest)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_nodedb_reset_tag;
    admin_message.nodedb_reset = 1;
    admin_message.session_passkey = *mta->session_passkey;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_request_factory_reset(MeshtasticAccount *mta, uint32_t dest)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_factory_reset_device_tag;
    admin_message.factory_reset_device = 1;
    admin_message.session_passkey = *mta->session_passkey;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_traceroute_request(MeshtasticAccount *mta, uint32_t dest)
{
    uint8_t buffer[512];
    meshtastic_RouteDiscovery route_discovery = meshtastic_RouteDiscovery_init_default;
    meshtastic_MeshPacket meshPacket = meshtastic_MeshPacket_init_default;
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, 512);
    pb_encode(&stream, meshtastic_RouteDiscovery_fields, &route_discovery);
    meshPacket.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    meshPacket.id = rand() % 0x7FFFFFFF;
    meshPacket.decoded.portnum = meshtastic_PortNum_TRACEROUTE_APP;
    meshPacket.to = dest;
    meshPacket.channel = 0;
    meshPacket.want_ack = false;
    meshPacket.decoded.want_response = true;
    meshPacket.hop_limit = 7;
    meshPacket.decoded.payload.size = stream.bytes_written;
    memcpy(meshPacket.decoded.payload.bytes, buffer, stream.bytes_written); // Copy data
    return mt_send_packet_to_radio(mta, meshPacket);
}

// Set Data
int mt_set_lora_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_Config_LoRaConfig lora)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    meshtastic_Config config = meshtastic_Config_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
    admin_message.session_passkey = *mta->session_passkey;
    config.which_payload_variant = meshtastic_Config_lora_tag;
    config.payload_variant.lora = lora;
    admin_message.set_config = config;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_set_device_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_Config_DeviceConfig device)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    meshtastic_Config config = meshtastic_Config_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
    admin_message.session_passkey = *mta->session_passkey;
    config.which_payload_variant = meshtastic_Config_device_tag;
    config.payload_variant.device = device;
    admin_message.set_config = config;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_set_position_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_Config_PositionConfig position)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    meshtastic_Config config = meshtastic_Config_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
    admin_message.session_passkey = *mta->session_passkey;
    config.which_payload_variant = meshtastic_Config_position_tag;
    config.payload_variant.position = position;
    admin_message.set_config = config;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_set_network_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_Config_NetworkConfig network)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    meshtastic_Config config = meshtastic_Config_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
    admin_message.session_passkey = *mta->session_passkey;
    config.which_payload_variant = meshtastic_Config_network_tag;
    config.payload_variant.network = network;
    admin_message.set_config = config;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_set_bluetooth_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_Config_BluetoothConfig bluetooth)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    meshtastic_Config config = meshtastic_Config_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
    admin_message.session_passkey = *mta->session_passkey;
    config.which_payload_variant = meshtastic_Config_bluetooth_tag;
    config.payload_variant.bluetooth = bluetooth;
    admin_message.set_config = config;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_set_security_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_Config_SecurityConfig security)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    meshtastic_Config config = meshtastic_Config_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_config_tag;
    admin_message.session_passkey = *mta->session_passkey;
    config.which_payload_variant = meshtastic_Config_security_tag;
    config.payload_variant.security = security;
    admin_message.set_config = config;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_set_mqtt_module_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_ModuleConfig_MQTTConfig mqtt)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    meshtastic_ModuleConfig module_config = meshtastic_ModuleConfig_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_module_config_tag;
    admin_message.session_passkey = *mta->session_passkey;
    module_config.which_payload_variant = meshtastic_ModuleConfig_mqtt_tag;
    module_config.payload_variant.mqtt = mqtt;
    admin_message.set_module_config = module_config;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_set_owner_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_User user)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_owner_tag;
    admin_message.session_passkey = *mta->session_passkey;
    admin_message.set_owner = user;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_set_fixed_position(MeshtasticAccount *mta, uint32_t dest, meshtastic_Position position)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_fixed_position_tag;
    admin_message.session_passkey = *mta->session_passkey;
    admin_message.set_fixed_position = position;
    return mt_send_admin(mta, admin_message, dest, true);
}

int mt_set_channel_config(MeshtasticAccount *mta, uint32_t dest, meshtastic_Channel channel)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_channel_tag;
    admin_message.session_passkey = *mta->session_passkey;
    admin_message.set_channel = channel;
    return mt_send_admin(mta, admin_message, dest, true);
}

void mt_set_time(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    meshtastic_AdminMessage admin_message = meshtastic_AdminMessage_init_default;
    admin_message.which_payload_variant = meshtastic_AdminMessage_set_time_only_tag;
    admin_message.session_passkey = *mta->session_passkey;
    admin_message.set_time_only = time(NULL);
    mta->cb_routing = NULL;
    mt_send_admin(mta, admin_message, mta->id, true);
}

int mt_send_heartbeat(MeshtasticAccount *mta)
{
    meshtastic_Heartbeat hearbeat = meshtastic_Heartbeat_init_default;
    meshtastic_ToRadio toRadio = meshtastic_ToRadio_init_default;
    hearbeat.dummy_field = rand() % 255;
    toRadio.which_payload_variant = meshtastic_ToRadio_heartbeat_tag;
    toRadio.heartbeat = hearbeat;
    return mt_send_to_radio(mta, toRadio);
}

int mt_send_text(MeshtasticAccount *mta, const char *text, uint32_t dest, uint8_t channel_index)
{
    meshtastic_MeshPacket meshPacket = meshtastic_MeshPacket_init_default;
    meshPacket.which_payload_variant = meshtastic_MeshPacket_decoded_tag;
    meshPacket.id = rand() % 0x7FFFFFFF;
    meshPacket.decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
    meshPacket.to = dest;
    meshPacket.channel = channel_index;
    meshPacket.want_ack = true;
    meshPacket.decoded.payload.size = strlen(text);
    memcpy(meshPacket.decoded.payload.bytes, text, meshPacket.decoded.payload.size);
    return mt_send_packet_to_radio(mta, meshPacket);
}

int mt_channel_message(MeshtasticAccount *mta, const char *text, int index)
{
    return mt_send_text(mta, text, 0xFFFFFFFF, index);
}

/**
 *
 * Helpers
 *
 */
double to_radians(double degree)
{
    return degree * (M_PI / 180.0);
}

double haversine(double lat1, double lon1, double lat2, double lon2)
{
    double dlat, dlon, a, c, distance;
    double R = 6371.0;
    lat1 = to_radians(lat1);
    lon1 = to_radians(lon1);
    lat2 = to_radians(lat2);
    lon2 = to_radians(lon2);
    dlat = lat2 - lat1;
    dlon = lon2 - lon1;
    a = sin(dlat / 2) * sin(dlat / 2) +
        cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
    c = 2 * atan2(sqrt(a), sqrt(1 - a));
    distance = R * c;
    return distance;
}

static guint64 to_int(const gchar *id)
{
    return id ? g_ascii_strtoull(id, NULL, 10) : 0;
}

static gchar *from_int(guint64 id)
{
    return g_strdup_printf("%" G_GUINT64_FORMAT, id);
}

void mt_join_room(MeshtasticAccount *mta, PurpleConvChat *chat)
{
    if (!purple_conv_chat_find_user(chat, (const char *)mta->me->user.id))
    {
        purple_conv_chat_add_user(chat, mta->me->user.id, NULL, PURPLE_CBFLAGS_NONE, FALSE);
    }
}

void seconds_to_days_str(uint32_t seconds, char *buffer)
{
    int32_t days, hours, minutes;
    GString *str;
    str = g_string_new(NULL);
    days = seconds / (24 * 3600);
    seconds %= (24 * 3600);
    hours = seconds / 3600;
    seconds %= 3600;
    minutes = seconds / 60;
    seconds %= 60;
    if (days > 0)
        g_string_append_printf(str, "%d days, ", days);
    if (hours > 0)
        g_string_append_printf(str, "%d hours, ", hours);
    if (minutes > 0)
        g_string_append_printf(str, "%d miutes, ", minutes);
    if (seconds > 0)
        g_string_append_printf(str, "%d seconds ", seconds);
    strcpy(buffer, str->str);
    g_string_free(str, TRUE);
}

int mt_get_connection(MeshtasticAccount *mta, char *port)
{
    int result;
    char username[100];
    struct sockaddr_in sa;
    strcpy(username, purple_account_get_username(mta->account));
    strcpy(port, strchr(username, '@') + 1);
    result = inet_pton(AF_INET, port, &(sa.sin_addr));
    if (result <= 0)
        return meshtastic_serial_connection;

    return meshtastic_socket_connection;
}

void mt_update_presence(MeshtasticAccount *mta, PurpleBuddy *buddy)
{
    const PurplePresence *presence;
    MeshtasticBuddy *mb = buddy->proto_data;
    bool active;
    if (!mb)
        return;
    presence = purple_buddy_get_presence(buddy);
    active = ((time(NULL) - AWAY_TIME) < mb->last_heard);
    if (active && !purple_presence_is_status_active(presence, purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE)))
    {
        purple_prpl_got_user_status(mta->account, buddy->name, purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE), NULL);
    }
    else if (!active && !purple_presence_is_status_active(presence, purple_primitive_get_id_from_type(PURPLE_STATUS_AWAY)))
    {
        purple_prpl_got_user_status(mta->account, buddy->name, purple_primitive_get_id_from_type(PURPLE_STATUS_AWAY), NULL);
    }
}

int mt_get_channel_name(MeshtasticAccount *mta, int id, char *name)
{
    PurpleChat *chat = NULL;
    chat = mt_find_blist_chat(mta->account, from_int(id));
    if (chat)
    {
        GHashTable *components = purple_chat_get_components(chat);
        strcpy(name, g_hash_table_lookup(components, "name"));
        return 1;
    }
    return 0;
}

void mt_set_buddy_packet(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy = mt_find_blist_buddy(mta->account, packet->from);
    MeshtasticBuddy *mb;
    if (buddy)
    {
        mb = buddy->proto_data;
        if (mb)
        {
            memcpy(&mb->packet, packet, sizeof(meshtastic_MeshPacket));
            mb->last_heard = time(NULL);
            purple_prpl_got_user_status(mta->account, buddy->name, purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE), NULL);
        }
    }
}

PurpleBuddy *mt_add_meshtastic_buddy(MeshtasticAccount *mta, uint32_t id, meshtastic_User user)
{
    char username[100];
    char port[40];
    char new[100];
    PurpleAccount *account = mta->account;
    PurpleBuddy *buddy = purple_find_buddy(account, user.id);
    MeshtasticBuddy *mb;
    meshtastic_MeshPacket meshPacket = meshtastic_MeshPacket_init_default;
    meshtastic_NodeInfo nodeInfo = meshtastic_NodeInfo_init_default;

    purple_debug_info(PROTO_NAME, "Adding new buddy %s\n", user.long_name);
    if (buddy)
    {
        purple_blist_alias_buddy(buddy, user.long_name);
        mb = buddy->proto_data;
        free(mb);
        mb = g_new0(MeshtasticBuddy, 1);
        mb->user = user;
        mb->id = id;
        mb->last_heard = 0;
        buddy->proto_data = mb;
        memcpy(&mb->packet, &meshPacket, sizeof(meshtastic_MeshPacket));
        memcpy(&mb->info, &nodeInfo, sizeof(meshtastic_NodeInfo));
    }
    else
    {
        buddy = purple_buddy_new(account, user.id, user.long_name);
        mb = g_new0(MeshtasticBuddy, 1);
        mb->user = user;
        mb->id = id;
        mb->last_heard = 0;
        buddy->proto_data = mb;
        purple_blist_add_buddy(buddy, NULL, purple_find_group("Nodes"), NULL);
    }

    if (mb->id == mta->id)
    {
        mta->me = mb;
        purple_account_set_alias(mta->account, user.long_name);
        strcpy(username, purple_account_get_username(mta->account));
        strcpy(port, strchr(username, '@') + 1);
        sprintf(new, "%s@%s", user.long_name, port);
        purple_account_set_username(mta->account, new);
    }

    mt_update_presence(mta, buddy);
    return buddy;
}

PurpleBuddy *mt_unknown_buddy(PurpleAccount *account, uint32_t id)
{
    char user_id[16];
    char long_name[40];
    char short_name[11];
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    meshtastic_User user = meshtastic_User_init_default;
    sprintf(user_id, "!%08x", id);
    sprintf(long_name, "Meshtastic %s", user_id + 5);
    sprintf(short_name, "%s", user_id + 5);
    strcpy(user.id, user_id);
    strcpy(user.long_name, long_name);
    strcpy(user.short_name, short_name);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    return mt_add_meshtastic_buddy(mta, id, user);
}

static void mt_acknowledge_cb(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    meshtastic_Routing routing = meshtastic_Routing_init_default;
    const char *message;
    mta->cb_routing = NULL;
    if (!mt_routing_from_packet(packet, &routing))
        return;
    message = mt_lookup_error_reason(routing.error_reason);
    if (routing.error_reason == meshtastic_Routing_Error_NONE)
    {
        purple_notify_info(mta->gc, g_strdup("Meshtastic Action"), g_strdup("Success!"), g_strdup("Action completed"));
    }
    else
    {
        purple_notify_info(mta->gc, g_strdup("Meshtastic Action"), g_strdup("Error"), message);
    }
}

/**
 *
 * MeshPacket App Port Handlers
 *
 */
void mt_handle_text_message_app(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    PurpleConvChat *convchat;
    PurpleConversation *conversation;
    GHashTable *components;
    const gchar *name;

    purple_debug_info(PROTO_NAME, "New Incoming message\n");
    if (packet->to == 0xFFFFFFFF)
    {
        PurpleChat *chat = mt_find_blist_chat(mta->account, from_int(packet->channel));
        if (!chat)
        {
            purple_debug_error(PROTO_NAME, "Channel not found\n");
            return;
        }
        components = purple_chat_get_components(chat);
        name = g_hash_table_lookup(components, "name");

        conversation = purple_find_chat(mta->gc, packet->channel);
        if (!conversation)
            conversation = serv_got_joined_chat(mta->gc, packet->channel, name);
        convchat = purple_conversation_get_chat_data(conversation);
        buddy = mt_find_blist_buddy(mta->account, packet->from);
        if (buddy)
        {
            if (!purple_conv_chat_find_user(convchat, (const char *)buddy->name))
            {
                purple_conv_chat_add_user(convchat, buddy->name, NULL, PURPLE_CBFLAGS_NONE, TRUE);
            }
            purple_conv_chat_write(convchat, buddy->name, (const char *)packet->decoded.payload.bytes, PURPLE_MESSAGE_RECV, time(NULL));
        }
        mt_join_room(mta, convchat);
    }
    else
    {
        PurpleBuddy *buddy = mt_find_blist_buddy(mta->account, packet->from);
        PurpleConversation *conversation = NULL;
        if (buddy)
        {
            conversation = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, buddy->name, mta->account);
        }
        if (!conversation)
        {
            conversation = purple_conversation_new(PURPLE_CONV_TYPE_IM, mta->account, buddy->name);
        }
        purple_conversation_present(conversation);
        purple_conversation_write(conversation, buddy->name, (const char *)packet->decoded.payload.bytes, PURPLE_MESSAGE_RECV, time(NULL));
    }
}

void mt_handle_position_app(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;
    meshtastic_Position position = meshtastic_Position_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    bool status = pb_decode(&stream, meshtastic_Position_fields, &position);
    if (!status)
        return;
    buddy = mt_find_blist_buddy(mta->account, packet->from);
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    memcpy(&mb->info.position, &position, sizeof(meshtastic_Position));
    purple_debug_info(PROTO_NAME, "%s updated their position\n", buddy->alias);
}

void mt_handle_node_info_app(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    meshtastic_User user = meshtastic_User_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    bool status = pb_decode(&stream, meshtastic_User_fields, &user);
    if (!status)
        return;
    mt_add_meshtastic_buddy(mta, packet->from, user);
}

void mt_handle_routing_app(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy = NULL;
    char notification_label[40];
    const char *message;
    meshtastic_Routing routing = meshtastic_Routing_init_zero;
    purple_debug_info(PROTO_NAME, "Routing data received\n");
    if (mta->cb_routing != NULL)
    {
        mta->cb_routing(mta, packet);
        return;
    }
    if (!mt_routing_from_packet(packet, &routing))
        return;
    message = mt_lookup_error_reason(routing.error_reason);
    if (packet->from == 0xffffffff)
    {
        strcpy(notification_label, "Broadcast");
    }
    else
    {
        buddy = mt_find_blist_buddy(mta->account, packet->from);
        strcpy(notification_label, buddy->alias);
    }
    if (routing.error_reason == meshtastic_Routing_Error_NONE)
    {
        if (buddy)
        {
            purple_debug_info(PROTO_NAME, "Acknowledge from %s\n", buddy->alias);
        }
        else
        {
            purple_debug_info(PROTO_NAME, "Acknowledge from %d\n", packet->from);
        }
    }
    else
    {
        purple_debug_error(PROTO_NAME, "%s\n", message);
        purple_notify_error(mta->gc, g_strdup(notification_label), g_strdup("Error"), (const char *)message);
        mta->cb_routing = NULL;
        mta->cb_config = NULL;
        mta->cb_data = NULL;
    }
}

void mt_handle_admin_app(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    pb_istream_t stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    bool status = pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    purple_debug_info(PROTO_NAME, "Admin data received\n");
    if (!status)
        return;
    purple_debug_info(PROTO_NAME, "Admin variant: %d\n", admin.which_payload_variant);
    switch (admin.which_payload_variant)
    {
    case meshtastic_AdminMessage_get_channel_response_tag:       // 2
    case meshtastic_AdminMessage_get_owner_response_tag:         // 4
    case meshtastic_AdminMessage_get_config_response_tag:        // 6
    case meshtastic_AdminMessage_get_module_config_response_tag: // 8
        if (admin.session_passkey.size)
        {
            memcpy(mta->session_passkey, &admin.session_passkey, sizeof(meshtastic_AdminMessage_session_passkey_t));
            purple_debug_info(PROTO_NAME, "Session passkey set\n");
        }
        if (mta->cb_config != NULL)
            mta->cb_config(mta, packet);
        break;
    default:
        purple_debug_info(PROTO_NAME, "Unhandled admin variant: %d\n", admin.which_payload_variant);
    }
}

void mt_handle_telemetry_app(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;
    meshtastic_Telemetry telemetry = meshtastic_Telemetry_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    bool status = pb_decode(&stream, meshtastic_Telemetry_fields, &telemetry);
    if (!status)
        return;
    purple_debug_info(PROTO_NAME, "Got telemetry data\n");
    if (telemetry.which_variant != meshtastic_Telemetry_device_metrics_tag)
    {
        return;
    }
    buddy = mt_find_blist_buddy(mta->account, packet->from);
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    memcpy(&mb->info.device_metrics, &telemetry.variant.device_metrics, sizeof(meshtastic_DeviceMetrics));
    purple_debug_info(PROTO_NAME, "%s updated their device metrics\n", buddy->alias);
}

void mt_handle_traceroute_app(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{

    meshtastic_RouteDiscovery discovery = meshtastic_RouteDiscovery_init_zero;
    PurpleNotifyUserInfo *info;
    pb_istream_t stream;
    bool status;
    int i;
    PurpleBuddy *buddy;
    PurpleBuddy *target_buddy;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    status = pb_decode(&stream, meshtastic_RouteDiscovery_fields, &discovery);
    if (!status)
        return;
    target_buddy = mt_find_blist_buddy(mta->account, packet->from);
    info = purple_notify_user_info_new();
    purple_notify_user_info_add_section_header(info, g_strdup_printf("Path towards %s", target_buddy->alias));
    purple_notify_user_info_add_pair(info, g_strdup_printf("%d", 1), mta->me->user.long_name);
    for (i = 0; i < discovery.route_count; i++)
    {
        buddy = mt_find_blist_buddy(mta->account, discovery.route[i]);
        if (buddy)
        {
            purple_notify_user_info_add_pair(info, g_strdup_printf("%d", i + 2), buddy->alias);
        }
        else
        {
            purple_notify_user_info_add_pair(info, g_strdup_printf("%d", i + 2), "Unknown");
        }
    }
    purple_notify_user_info_add_pair(info, g_strdup_printf("%d", i + 2), target_buddy->alias);
    purple_notify_user_info_add_section_header(info, g_strdup_printf("Return path from %s", target_buddy->alias));
    purple_notify_user_info_add_pair(info, g_strdup_printf("%d", 1), target_buddy->alias);
    for (i = 0; i < discovery.route_back_count; i++)
    {
        buddy = mt_find_blist_buddy(mta->account, discovery.route_back[i]);
        if (buddy)
        {
            purple_notify_user_info_add_pair(info, g_strdup_printf("%d", i + 2), buddy->alias);
        }
        else
        {
            purple_notify_user_info_add_pair(info, g_strdup_printf("%d", i + 2), "Unknown");
        }
    }
    purple_notify_user_info_add_pair(info, g_strdup_printf("%d", i + 2), mta->me->user.long_name);
    purple_notify_userinfo(mta->gc, target_buddy->alias, info, NULL, NULL);
}

/**
 *
 * FromRadio Handlers
 *
 */
void mt_handle_packet(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    mt_set_buddy_packet(mta, packet);
    if (packet->which_payload_variant != meshtastic_MeshPacket_decoded_tag)
        return;
    purple_debug_info(PROTO_NAME, "Received application port %d.\n", packet->decoded.portnum);
    switch (packet->decoded.portnum)
    {
    case meshtastic_PortNum_TEXT_MESSAGE_APP: // 1
        mt_handle_text_message_app(mta, packet);
        break;
    case meshtastic_PortNum_POSITION_APP: // 3
        mt_handle_position_app(mta, packet);
        break;
    case meshtastic_PortNum_NODEINFO_APP: // 4
        mt_handle_node_info_app(mta, packet);
        break;
    case meshtastic_PortNum_ROUTING_APP: // 5
        mt_handle_routing_app(mta, packet);
        break;
    case meshtastic_PortNum_ADMIN_APP: // 6
        mt_handle_admin_app(mta, packet);
        break;
    case meshtastic_PortNum_TELEMETRY_APP: // 67
        mt_handle_telemetry_app(mta, packet);
        break;
    case meshtastic_PortNum_TRACEROUTE_APP: // 70
        mt_handle_traceroute_app(mta, packet);
        break;
    default:
        purple_debug_info(PROTO_NAME, "Port %d not yet implemented.\n", packet->decoded.portnum);
    }
}

void mt_handle_my_info(MeshtasticAccount *mta, meshtastic_MyNodeInfo *info)
{
    mta->id = info->my_node_num;
}

void mt_handle_node_info(MeshtasticAccount *mta, meshtastic_NodeInfo *info)
{
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;
    if (!info->has_user)
    {
        return;
    }

    if (*info->user.id == '\0')
    {
        return;
    }
    buddy = mt_add_meshtastic_buddy(mta, info->num, info->user);
    mb = buddy->proto_data;
    mb->last_heard = info->last_heard;
    memcpy(&mb->info, info, sizeof(meshtastic_NodeInfo));
    mt_update_presence(mta, buddy);
}

void mt_handle_config(MeshtasticAccount *mta, meshtastic_Config *config)
{
    switch (config->which_payload_variant)
    {
    case meshtastic_Config_lora_tag:
        memcpy(&mta->config->payload_variant.lora, &config->payload_variant.lora, sizeof(meshtastic_Config_LoRaConfig));
        break;
    default:
        purple_debug_info(PROTO_NAME, "%d config not handled yet\n", config->which_payload_variant);
    }
}

void mt_handle_config_complete(MeshtasticAccount *mta, meshtastic_FromRadio *fromRadio)
{
    mta->got_config = 1;
    mt_request_session_key(mta, mta->id);
    mta->cb_config = &mt_set_time;
    if (!mta->config->payload_variant.lora.region)
    {
        mt_request_lora_region_config(mta);
    }
}

void mt_handle_channel(MeshtasticAccount *mta, meshtastic_Channel *channel)
{
    gchar *id;
    gchar *name;
    PurpleChat *chat;
    PurpleAccount *account = mta->account;

    if (channel->role == meshtastic_Channel_Role_DISABLED)
    {
        id = from_int(channel->index);
        chat = purple_blist_find_chat(account, id);
        if (strcmp(channel->settings.name, "admin") == 0)
            mta->admin_channel = 0;
        if (chat)
            purple_blist_remove_chat(chat);
        // purple_blist_node_set_flags((PurpleBlistNode *)chat,  PURPLE_BLIST_NODE_FLAG_INVISIBLE);
        return;
    }
    if (*channel->settings.name == '\0')
    {
        if (channel->index == 0)
        {
            strcpy(channel->settings.name, "Default");
        }
        else
        {
            strcpy(channel->settings.name, g_strdup_printf("Channel%d", channel->index));
        }
    }

    purple_debug_info(PROTO_NAME, "Adding new channel %s at index %d\n", channel->settings.name, channel->index);
    if (strcmp(channel->settings.name, "admin") == 0)
    {
        purple_debug_info(PROTO_NAME, "Found admin channel at index %d\n", channel->index);
        mta->admin_channel = channel->index;
    }
    id = from_int(channel->index);
    name = channel->settings.name;
    chat = purple_blist_find_chat(account, id);
    if (!chat)
    {
        GHashTable *components = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
        g_hash_table_replace(components, g_strdup("id"), id);
        g_hash_table_replace(components, g_strdup("name"), g_strdup(name));
        chat = purple_chat_new(account, name, components);
        purple_blist_add_chat(chat, purple_find_group("Channels"), NULL);
    }
    else
    {
        GHashTable *components = purple_chat_get_components(chat);
        g_hash_table_replace(components, g_strdup("name"), g_strdup(name));
        purple_blist_alias_chat(chat, name);
    }
}

void mt_handle_metadata(MeshtasticAccount *mta, meshtastic_DeviceMetadata *metadata)
{
    memcpy(mta->metadata, metadata, sizeof(meshtastic_DeviceMetadata));
}

void mt_handle_fileinfo(MeshtasticAccount *mta, meshtastic_FileInfo *file)
{
}

void mt_handle_clientnotification(MeshtasticAccount *mta, meshtastic_ClientNotification *notification)
{
    switch (notification->level)
    {
    case meshtastic_LogRecord_Level_CRITICAL:
    case meshtastic_LogRecord_Level_ERROR:
        purple_notify_error(mta->gc, g_strdup("Device Notification"), g_strdup("Attention"), (const char *)notification->message);
        break;
    case meshtastic_LogRecord_Level_WARNING:
        purple_notify_warning(mta->gc, g_strdup("Device Notification"), g_strdup("Attention"), (const char *)notification->message);
        break;
    default:
        purple_notify_info(mta->gc, g_strdup("Device Notification"), g_strdup("Attention"), (const char *)notification->message);
    }
}

static void mt_received_from_radio(MeshtasticAccount *mta)
{
    int status;
    meshtastic_FromRadio fromRadio = meshtastic_FromRadio_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(mta->protobuf, mta->protobuf_size);
    status = pb_decode(&stream, meshtastic_FromRadio_fields, &fromRadio);
    if (!status)
    {
        purple_debug_error(PROTO_NAME, "Failed to parse FromRadio protobuf\n");
        return;
    }
    switch (fromRadio.which_payload_variant)
    {
    case meshtastic_FromRadio_packet_tag: // 2
        mt_handle_packet(mta, &fromRadio.packet);
        break;
    case meshtastic_FromRadio_my_info_tag: // 3
        mt_handle_my_info(mta, &fromRadio.my_info);
        break;
    case meshtastic_FromRadio_node_info_tag: // 4
        mt_handle_node_info(mta, &fromRadio.node_info);
        break;
    case meshtastic_FromRadio_config_tag: // 5
        mt_handle_config(mta, &fromRadio.config);
        break;
    case meshtastic_FromRadio_config_complete_id_tag: // 7
        mt_handle_config_complete(mta, &fromRadio);
        break;
    case meshtastic_FromRadio_rebooted_tag: // 8
        mt_request_initial_config(mta);
        break;
    case meshtastic_FromRadio_channel_tag: // 10
        mt_handle_channel(mta, &fromRadio.channel);
        break;
    case meshtastic_FromRadio_metadata_tag: // 13
        mt_handle_metadata(mta, &fromRadio.metadata);
        break;
    case meshtastic_FromRadio_fileInfo_tag: // 15
        mt_handle_fileinfo(mta, &fromRadio.fileInfo);
        break;
    case meshtastic_FromRadio_clientNotification_tag: // 16
        mt_handle_clientnotification(mta, &fromRadio.clientNotification);
        break;
    default:
        purple_debug_info(PROTO_NAME, "Unhandled variant %d\n", fromRadio.which_payload_variant);
    }
}
/**
 *
 * Event Loop Functions
 *
 */
static void mt_get_packet(gpointer data)
{
    MeshtasticAccount *mta = data;
    uint8_t b, msb, lsb;
    size_t size;
    int bytes_num;
    if (mta->protobuf_remaining == 0)
    {
        bytes_num = mt_read(mta, &b, 1);
        if (bytes_num <= 0)
            return;
        if (b != START_BYTE_1)
            return;
        bytes_num = mt_read(mta, &b, 1);
        if (bytes_num <= 0)
            return;
        if (b != START_BYTE_2)
            return;
        bytes_num = mt_read(mta, &msb, 1);
        if (bytes_num <= 0)
            return;
        bytes_num = mt_read(mta, &lsb, 1);
        if (bytes_num <= 0)
            return;
        size = ((int)msb << 8) | (int)lsb;
        if (size > MESHTASTIC_BUFFER_SIZE)
        {
            purple_debug_error(PROTO_NAME, "Protobuf size larger than buffer (%d)\n", MESHTASTIC_BUFFER_SIZE);
            return;
        }
        mta->protobuf_size = size;
        mta->protobuf_remaining = size;
    }
    bytes_num = mt_read(mta, mta->protobuf + (mta->protobuf_size - mta->protobuf_remaining), mta->protobuf_remaining);
    if (bytes_num <= 0)
    {
        purple_debug_error(PROTO_NAME, "Something went wrong while receiving protobuf ... aborting\n");
        mta->protobuf_size = 0;
        mta->protobuf_remaining = 0;
        return;
    }
    mta->protobuf_remaining = mta->protobuf_remaining - bytes_num;
    if (mta->protobuf_remaining == 0)
    {
        mt_received_from_radio(mta);
    }
}

static void mt_from_radio_cb(gpointer data, gint source, PurpleInputCondition cond)
{
    mt_get_packet(data);
}

#ifdef _WIN32
static gboolean mt_read_handle(MeshtasticAccount *mta)
{
    DWORD dwErrors;
    COMSTAT commStatus;
    ClearCommError(mta->handle, &dwErrors, &commStatus);
    while ((commStatus.cbInQue > 0) && (dwErrors <= 0))
    {
        mt_get_packet(mta);
        ClearCommError(mta->handle, &dwErrors, &commStatus);
    }
    if (dwErrors > 0)
    {
        purple_connection_error_reason(mta->gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Lost connection with radio"));
        return FALSE;
    }
    return TRUE;
}
#endif

static gboolean mt_update_statusses(MeshtasticAccount *mta)
{
    PurpleAccount *account = mta->account;
    PurpleBlistNode *node;
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;

    for (
        node = purple_blist_get_root();
        node != NULL;
        node = purple_blist_node_next(node, TRUE))
    {
        if (purple_blist_node_get_type(node) == PURPLE_BLIST_BUDDY_NODE)
        {
            buddy = PURPLE_BUDDY(node);
            if (purple_buddy_get_account(buddy) != account)
                continue;
            mb = buddy->proto_data;
            if (!mb)
                continue;
            mt_update_presence(mta, buddy);
        }
    }
    return TRUE;
}

static gboolean mt_heartbeat(MeshtasticAccount *mta)
{
    mt_send_heartbeat(mta);
    return TRUE;
}

void mt_add_groups(MeshtasticAccount *mta)
{
    PurpleGroup *group;
    group = purple_find_group("Nodes");
    if (!group)
    {
        group = purple_group_new("Nodes");
        purple_blist_add_group(group, NULL);
    }
    group = purple_find_group("Channels");
    if (!group)
    {
        group = purple_group_new("Channels");
        purple_blist_add_group(group, NULL);
    }
}

void mt_user_info_base(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info)
{
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    MeshtasticBuddy *me;
    MeshtasticBuddy *mb = buddy->proto_data;
    int32_t rssi;
    float snr;
    double lat1, lon1, lat2, lon2, distance;
    GString *signal;
    char buffer[100];
    if (!mb)
    {
        return;
    }
    purple_notify_user_info_add_pair(user_info, "Long Name", mb->user.long_name);
    purple_notify_user_info_add_pair(user_info, "Short Name", mb->user.short_name);

    rssi = mb->packet.rx_rssi;
    if (rssi != 0)
    {
        signal = g_string_new(NULL);
        if (rssi < -100)
        {
            g_string_append_printf(signal, "<span bgcolor=\"#e6b0aa\" color=\"#000000\"> Bad </span>");
        }
        else if (rssi < -60)
        {
            g_string_append_printf(signal, "<span bgcolor=\"#f9e79f\" color=\"#000000\"> Good </span>");
        }
        else if (rssi <= -30)
        {
            g_string_append_printf(signal, "<span bgcolor=\"#a9dfbf\" color=\"#000000\"> Very Good </span>");
        }
        g_string_append_printf(signal, " (%d dB)", rssi);
        purple_notify_user_info_add_pair(user_info, "Signal", signal->str);
        g_string_free(signal, TRUE);
    }

    snr = mb->packet.rx_snr;
    if (snr != 0)
    {
        signal = g_string_new(NULL);
        if (snr < -10)
        {
            g_string_append_printf(signal, "<span bgcolor=\"#e6b0aa\" color=\"#000000\"> Bad </span>");
        }
        else if (snr < -0)
        {
            g_string_append_printf(signal, "<span bgcolor=\"#f9e79f\" color=\"#000000\"> Good </span>");
        }
        else if (snr <= 10)
        {
            g_string_append_printf(signal, "<span bgcolor=\"#a9dfbf\" color=\"#000000\"> Very Good </span>");
        }
        g_string_append_printf(signal, " (%.2f dB)", snr);
        purple_notify_user_info_add_pair(user_info, "SNR", signal->str);
        g_string_free(signal, TRUE);
    }

    if (mb->info.hops_away)
    {
        purple_notify_user_info_add_pair(user_info, "Hops away", g_strdup_printf("%d", mb->info.hops_away));
    }
    else
    {
        purple_notify_user_info_add_pair(user_info, "Hops away", g_strdup_printf("Direct"));
    }

    // Distance
    account = buddy->account;
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    me = mta->me;
    if ((me->info.position.latitude_i) && (mb->info.position.latitude_i))
    {
        lat1 = me->info.position.latitude_i * 1e-7;
        lon1 = me->info.position.longitude_i * 1e-7;
        lat2 = mb->info.position.latitude_i * 1e-7;
        lon2 = mb->info.position.longitude_i * 1e-7;
        distance = haversine(lat1, lon1, lat2, lon2);
        purple_notify_user_info_add_pair(user_info, "Distance away", g_strdup_printf("%.2fkm", distance));
    }

    if (mb->last_heard)
    {
        seconds_to_days_str(time(NULL) - mb->last_heard, buffer);
        purple_notify_user_info_add_pair(user_info, "Seen", g_strdup_printf("%s ago", buffer));
    }
}
/**
 *
 * Configuration screens
 *
 */
static void mt_cancel_cb(MeshtasticAccount *mta, PurpleRequestFields *fields)
{
}

// User / Owner Config
void mt_save_user_config(MeshtasticConfigFields *cf)
{
    meshtastic_User *user = cf->config;
    MeshtasticBuddy *mb = cf->buddy->proto_data;
    strcpy(user->long_name, purple_request_fields_get_string(cf->fields, "long_name"));
    strcpy(user->short_name, purple_request_fields_get_string(cf->fields, "short_name"));
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_owner_config(cf->mta, mb->id, *user);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_user_config(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_User user)
{
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *user_config = purple_request_field_group_new(NULL);
    PurpleRequestField *long_name;
    PurpleRequestField *short_name;
    MeshtasticConfigFields *conf;
    meshtastic_User *usr;

    // long name
    long_name = purple_request_field_string_new("long_name", g_strdup("Long name"), user.long_name, FALSE);
    purple_request_field_group_add_field(user_config, long_name);

    // short name
    short_name = purple_request_field_string_new("short_name", g_strdup("Short name"), user.short_name, FALSE);
    purple_request_field_group_add_field(user_config, short_name);

    purple_request_fields_add_group(fields, user_config);

    conf = g_new0(MeshtasticConfigFields, 1);
    usr = g_new0(meshtastic_User, 1);
    memcpy(usr, &user, sizeof(meshtastic_User));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = usr;

    purple_request_fields(
        mta->gc,                        // void *handle,
        buddy->alias,                   // const char *title,
        g_strdup("User Configuration"), // const char *primary,
        NULL,
        fields,                          // PurpleRequestFields *fields,
        g_strdup("Save"),                // const char *ok_text,
        G_CALLBACK(mt_save_user_config), // GCallback ok_cb,
        g_strdup("Cancel"),              // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),        // GCallback cancel_cb,
        mta->account,                    // PurpleAccount *account,
        NULL,                            // const char *who,
        NULL,                            // PurpleConversation *conv,
        conf                             // void *user_data
    );
}

// LoRa Config
void mt_save_lora_config(MeshtasticConfigFields *cf)
{
    char *i;
    meshtastic_Config *config = cf->config;
    PurpleRequestField *region;
    GList *selectedRegion;
    PurpleRequestField *modem_preset;
    GList *selected_modem_preset;
    PurpleRequestField *hop_limit;
    GList *selected_hop_limit;
    gboolean mqtt;
    MeshtasticBuddy *mb;

    // region
    region = purple_request_fields_get_field(cf->fields, "region");
    selectedRegion = purple_request_field_list_get_selected(region);
    i = purple_request_field_list_get_data(region, (const char *)selectedRegion->data);
    config->payload_variant.lora.region = to_int(i);

    // modem preset
    config->payload_variant.lora.use_preset = true;
    modem_preset = purple_request_fields_get_field(cf->fields, "modem_preset");
    selected_modem_preset = purple_request_field_list_get_selected(modem_preset);
    i = purple_request_field_list_get_data(modem_preset, (const char *)selected_modem_preset->data);
    config->payload_variant.lora.modem_preset = to_int(i);

    // hop limit
    hop_limit = purple_request_fields_get_field(cf->fields, "hop_limit");
    selected_hop_limit = purple_request_field_list_get_selected(hop_limit);
    i = purple_request_field_list_get_data(hop_limit, (const char *)selected_hop_limit->data);
    config->payload_variant.lora.hop_limit = to_int(i);

    // allow mqtt
    mqtt = purple_request_fields_get_bool(cf->fields, "allow_mqtt");
    config->payload_variant.lora.ignore_mqtt = !mqtt;
    config->payload_variant.lora.config_ok_to_mqtt = mqtt;

    mb = cf->buddy->proto_data;
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_lora_config(cf->mta, mb->id, config->payload_variant.lora);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_lora_config(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_Config config)
{
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *lora_config = purple_request_field_group_new(NULL);
    int i;
    PurpleRequestField *region;
    PurpleRequestField *modem_preset;
    PurpleRequestField *hop_limit;
    PurpleRequestField *allow_mqtt;
    MeshtasticConfigFields *conf;
    meshtastic_Config *con;

    // region
    region = purple_request_field_list_new("region", g_strdup("Region (frequency plan)"));
    purple_request_field_list_set_multi_select(region, FALSE);
    for (i = 1; i < 22; i++)
    {
        purple_request_field_list_add(region, mt_lookup_region(i), from_int(i));
    }
    purple_request_field_list_add_selected(region, mt_lookup_region(config.payload_variant.lora.region));
    purple_request_field_group_add_field(lora_config, region);

    // modem preset
    modem_preset = purple_request_field_list_new("modem_preset", g_strdup("Modem preset"));
    purple_request_field_list_set_multi_select(modem_preset, FALSE);
    for (i = 0; i < 9; i++)
    {
        purple_request_field_list_add(modem_preset, mt_lookup_modem_preset(i), from_int(i));
    }
    purple_request_field_list_add_selected(modem_preset, mt_lookup_modem_preset(config.payload_variant.lora.modem_preset));
    purple_request_field_group_add_field(lora_config, modem_preset);

    // hop limit
    hop_limit = purple_request_field_list_new("hop_limit", g_strdup("Hop limit"));
    purple_request_field_list_set_multi_select(hop_limit, FALSE);
    for (i = 1; i < 8; i++)
    {
        purple_request_field_list_add(hop_limit, from_int(i), from_int(i));
    }
    purple_request_field_list_add_selected(hop_limit, from_int(config.payload_variant.lora.hop_limit));
    purple_request_field_group_add_field(lora_config, hop_limit);

    // allow MQTT
    allow_mqtt = purple_request_field_bool_new("allow_mqtt", g_strdup("Allow MQTT"), !config.payload_variant.lora.ignore_mqtt);
    purple_request_field_group_add_field(lora_config, allow_mqtt);

    purple_request_fields_add_group(fields, lora_config);

    conf = g_new0(MeshtasticConfigFields, 1);
    con = g_new0(meshtastic_Config, 1);
    memcpy(con, &config, sizeof(meshtastic_Config));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = con;

    purple_request_fields(
        mta->gc,                                                                             // void *handle,
        buddy->alias,                                                                        // const char *title,
        g_strdup("LoRa Config                                                            "), // const char *primary,
        NULL,
        fields,                          // PurpleRequestFields *fields,
        g_strdup("Save"),                // const char *ok_text,
        G_CALLBACK(mt_save_lora_config), // GCallback ok_cb,
        g_strdup("Cancel"),              // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),        // GCallback cancel_cb,
        mta->account,                    // PurpleAccount *account,
        NULL,                            // const char *who,
        NULL,                            // PurpleConversation *conv,
        conf                             // void *user_data
    );
}

void mt_save_lora_region_config(MeshtasticConfigFields *cf)
{
    char *i;
    meshtastic_Config *config = cf->config;
    PurpleRequestField *region;
    GList *selectedRegion;
    MeshtasticBuddy *mb;

    // region
    region = purple_request_fields_get_field(cf->fields, "region");
    selectedRegion = purple_request_field_list_get_selected(region);
    if (selectedRegion)
    {
        i = purple_request_field_list_get_data(region, (const char *)selectedRegion->data);
        config->payload_variant.lora.region = to_int(i);

        mb = cf->buddy->proto_data;
        cf->mta->cb_routing = &mt_acknowledge_cb;
        mt_set_lora_config(cf->mta, mb->id, config->payload_variant.lora);
    }
    g_free(cf->config);
    g_free(cf);
}

void mt_request_lora_region_config(MeshtasticAccount *mta)
{
    PurpleRequestField *region;
    MeshtasticConfigFields *conf;
    meshtastic_Config *con;
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *lora_config = purple_request_field_group_new(NULL);
    PurpleBuddy *buddy = mt_find_blist_buddy(mta->account, mta->me->id);
    int i;
    // region
    region = purple_request_field_list_new("region", g_strdup("Region (frequency plan)"));
    purple_request_field_list_set_multi_select(region, FALSE);
    for (i = 1; i < 22; i++)
    {
        purple_request_field_list_add(region, mt_lookup_region(i), from_int(i));
    }
    purple_request_field_list_add_selected(region, mt_lookup_region(meshtastic_Config_LoRaConfig_RegionCode_US));
    purple_request_field_group_add_field(lora_config, region);
    purple_request_fields_add_group(fields, lora_config);
    conf = g_new0(MeshtasticConfigFields, 1);
    con = g_new0(meshtastic_Config, 1);
    memcpy(con, mta->config, sizeof(meshtastic_Config));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = con;

    purple_request_fields(
        mta->gc,                                                                // void *handle,
        buddy->alias,                                                           // const char *title,
        g_strdup("Region not set                                            "), // const char *primary,
        NULL,
        fields,                                 // PurpleRequestFields *fields,
        g_strdup("Save"),                       // const char *ok_text,
        G_CALLBACK(mt_save_lora_region_config), // GCallback ok_cb,
        g_strdup("Cancel"),                     // const char *cancel_text,
        NULL,                                   // GCallback cancel_cb,
        mta->account,                           // PurpleAccount *account,
        NULL,                                   // const char *who,
        NULL,                                   // PurpleConversation *conv,
        conf                                    // void *user_data
    );
}

// Device Config
void mt_save_device_config(MeshtasticConfigFields *cf)
{
    char *i;
    meshtastic_Config *config = cf->config;
    PurpleRequestField *role;
    PurpleRequestField *rebroadcast_mode;
    PurpleRequestField *node_info_broadcast_secs;
    GList *selectedRole;
    GList *selectedRebroadcastMode;
    GList *selectedSecs;
    PurpleRequestField *tzdef;
    GList *selectedDef;
    MeshtasticBuddy *mb;

    // Role
    role = purple_request_fields_get_field(cf->fields, "role");
    selectedRole = purple_request_field_list_get_selected(role);
    i = purple_request_field_list_get_data(role, (const char *)selectedRole->data);
    config->payload_variant.device.role = to_int(i);

    // Rebroadcast Mode
    rebroadcast_mode = purple_request_fields_get_field(cf->fields, "rebroadcast_mode");
    selectedRebroadcastMode = purple_request_field_list_get_selected(rebroadcast_mode);
    i = purple_request_field_list_get_data(rebroadcast_mode, (const char *)selectedRebroadcastMode->data);
    config->payload_variant.device.rebroadcast_mode = to_int(i);

    // Rebroadcast Interval
    node_info_broadcast_secs = purple_request_fields_get_field(cf->fields, "node_info_broadcast_secs");
    selectedSecs = purple_request_field_list_get_selected(node_info_broadcast_secs);
    i = purple_request_field_list_get_data(node_info_broadcast_secs, (const char *)selectedSecs->data);
    config->payload_variant.device.node_info_broadcast_secs = to_int(i);

    // Timezone
    tzdef = purple_request_fields_get_field(cf->fields, "tzdef");
    selectedDef = purple_request_field_list_get_selected(tzdef);
    if (selectedDef != NULL)
        strcpy(config->payload_variant.device.tzdef, selectedDef->data);

    // LED Heartbeat
    config->payload_variant.device.led_heartbeat_disabled = purple_request_fields_get_bool(cf->fields, "led_heartbeat_disabled");

    mb = cf->buddy->proto_data;
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_device_config(cf->mta, mb->id, config->payload_variant.device);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_device_config(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_Config config)
{
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *device_config = purple_request_field_group_new(NULL);
    int i;
    PurpleRequestField *role;
    PurpleRequestField *rebroadcast_mode;
    PurpleRequestField *node_info_broadcast_secs;
    PurpleRequestField *tzdef;
    GList *tzdefs;
    GList *elem;
    PurpleRequestField *led_heartbeat_disabled;
    MeshtasticConfigFields *conf;
    meshtastic_Config *con;

    // Role
    role = purple_request_field_list_new("role", g_strdup("Role"));
    purple_request_field_list_set_multi_select(role, FALSE);
    for (i = 0; i < 11; i++)
    {
        purple_request_field_list_add(role, mt_lookup_role(i), from_int(i));
    }
    purple_request_field_list_add_selected(role, mt_lookup_role(config.payload_variant.device.role));
    purple_request_field_group_add_field(device_config, role);

    // Rebroadcast Mode
    rebroadcast_mode = purple_request_field_list_new("rebroadcast_mode", g_strdup("Rebroadcast mode"));
    purple_request_field_list_set_multi_select(rebroadcast_mode, FALSE);
    for (i = 0; i < 6; i++)
    {
        purple_request_field_list_add(rebroadcast_mode, mt_lookup_rebroadcast_mode(i), from_int(i));
    }
    purple_request_field_list_add_selected(rebroadcast_mode, mt_lookup_rebroadcast_mode(config.payload_variant.device.rebroadcast_mode));
    purple_request_field_group_add_field(device_config, rebroadcast_mode);

    // Rebroadcast Interval
    node_info_broadcast_secs = purple_request_field_list_new("node_info_broadcast_secs", g_strdup("Node info broadcast interval"));
    purple_request_field_list_set_multi_select(node_info_broadcast_secs, FALSE);
    purple_request_field_list_add(node_info_broadcast_secs, g_strdup("1 Hour"), from_int(3600));
    purple_request_field_list_add(node_info_broadcast_secs, g_strdup("2 Hours"), from_int(7200));
    purple_request_field_list_add(node_info_broadcast_secs, g_strdup("4 Hours"), from_int(14400));
    purple_request_field_list_add(node_info_broadcast_secs, g_strdup("6 Hours"), from_int(21600));
    purple_request_field_list_add(node_info_broadcast_secs, g_strdup("12 Hours"), from_int(43200));
    purple_request_field_list_add(node_info_broadcast_secs, g_strdup("24 Hours"), from_int(86400));

    if (config.payload_variant.device.node_info_broadcast_secs <= 3600)
        purple_request_field_list_add_selected(node_info_broadcast_secs, "1 Hour");
    else if (config.payload_variant.device.node_info_broadcast_secs <= 7200)
        purple_request_field_list_add_selected(node_info_broadcast_secs, "2 Hours");
    else if (config.payload_variant.device.node_info_broadcast_secs <= 14400)
        purple_request_field_list_add_selected(node_info_broadcast_secs, "4 Hours");
    else if (config.payload_variant.device.node_info_broadcast_secs <= 21600)
        purple_request_field_list_add_selected(node_info_broadcast_secs, "6 Hours");
    else if (config.payload_variant.device.node_info_broadcast_secs <= 43200)
        purple_request_field_list_add_selected(node_info_broadcast_secs, "12 Hours");
    else if (config.payload_variant.device.node_info_broadcast_secs <= 86400)
        purple_request_field_list_add_selected(node_info_broadcast_secs, "24 Hours");
    else
        purple_request_field_list_add_selected(node_info_broadcast_secs, "1 Hour");
    purple_request_field_group_add_field(device_config, node_info_broadcast_secs);

    // Timezone
    tzdef = purple_request_field_list_new("tzdef", g_strdup("Timezone"));
    purple_request_field_list_set_multi_select(tzdef, FALSE);
    tzdefs = mt_lookup_timezones();
    for (elem = tzdefs; elem; elem = elem->next)
    {
        purple_request_field_list_add(tzdef, elem->data, elem->data);
        if (strcmp(config.payload_variant.device.tzdef, elem->data) == 0)
        {
            purple_request_field_list_add_selected(tzdef, config.payload_variant.device.tzdef);
        }
    }
    purple_request_field_group_add_field(device_config, tzdef);
    g_list_free(elem);
    g_list_free(tzdefs);

    // Heartbeat LED
    led_heartbeat_disabled = purple_request_field_bool_new("led_heartbeat_disabled", g_strdup("Disable heartbeat LED"), config.payload_variant.device.led_heartbeat_disabled);
    purple_request_field_group_add_field(device_config, led_heartbeat_disabled);

    purple_request_fields_add_group(fields, device_config);

    conf = g_new0(MeshtasticConfigFields, 1);
    con = g_new0(meshtastic_Config, 1);
    memcpy(con, &config, sizeof(meshtastic_Config));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = con;

    purple_request_fields(
        mta->gc,                                                                               // void *handle,
        buddy->alias,                                                                          // const char *title,
        g_strdup("Device Config                                                            "), // const char *primary,
        NULL,
        fields,                            // PurpleRequestFields *fields,
        g_strdup("Save"),                  // const char *ok_text,
        G_CALLBACK(mt_save_device_config), // GCallback ok_cb,
        g_strdup("Cancel"),                // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),          // GCallback cancel_cb,
        mta->account,                      // PurpleAccount *account,
        NULL,                              // const char *who,
        NULL,                              // PurpleConversation *conv,
        conf                               // void *user_data
    );
}

// Position Config
void mt_save_position_config(MeshtasticConfigFields *cf)
{
    meshtastic_Config *config = cf->config;
    uint32_t flags, flag;
    PurpleRequestField *position_flags;
    GList *flist;
    GList *elem;
    MeshtasticBuddy *mb;

    // Broadcast Interval
    config->payload_variant.position.position_broadcast_secs = purple_request_fields_get_integer(cf->fields, "position_broadcast_secs");

    // Smart Position
    config->payload_variant.position.position_broadcast_smart_enabled = purple_request_fields_get_bool(cf->fields, "position_broadcast_smart_enabled");

    // Smart Position Distance
    config->payload_variant.position.broadcast_smart_minimum_distance = purple_request_fields_get_integer(cf->fields, "broadcast_smart_minimum_distance");

    // Smart Position Interval
    config->payload_variant.position.broadcast_smart_minimum_interval_secs = purple_request_fields_get_integer(cf->fields, "broadcast_smart_minimum_interval_secs");

    // Fixed Position
    config->payload_variant.position.fixed_position = purple_request_fields_get_bool(cf->fields, "fixed_position");

    // GPS Enabled
    config->payload_variant.position.gps_enabled = purple_request_fields_get_bool(cf->fields, "gps_enabled");

    // GPS Update Interval
    config->payload_variant.position.gps_enabled = purple_request_fields_get_bool(cf->fields, "gps_enabled");

    // Position Flags
    flags = 0;
    position_flags = purple_request_fields_get_field(cf->fields, "position_flags");
    flist = purple_request_field_list_get_selected(position_flags);

    for (elem = flist; elem; elem = elem->next)
    {
        flag = to_int(purple_request_field_list_get_data(position_flags, elem->data));
        flags += flag;
    }
    config->payload_variant.position.position_flags = flags;

    mb = cf->buddy->proto_data;
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_position_config(cf->mta, mb->id, config->payload_variant.position);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_position_config_(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_Config config)
{
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *position_config = purple_request_field_group_new(NULL);
    int i;
    PurpleRequestField *position_broadcast_secs;
    PurpleRequestField *position_broadcast_smart_enabled;
    PurpleRequestField *broadcast_smart_minimum_distance;
    PurpleRequestField *broadcast_smart_minimum_interval_secs;
    PurpleRequestField *fixed_position;
    PurpleRequestField *gps_enabled;
    PurpleRequestField *gps_update_interval;
    PurpleRequestField *position_flags;
    MeshtasticConfigFields *conf;
    meshtastic_Config *con;

    // Broadcast Interval
    position_broadcast_secs = purple_request_field_int_new("position_broadcast_secs", g_strdup("Broadcast interval (seconds)"), config.payload_variant.position.position_broadcast_secs);
    purple_request_field_group_add_field(position_config, position_broadcast_secs);

    // Smart Position
    position_broadcast_smart_enabled = purple_request_field_bool_new("position_broadcast_smart_enabled", g_strdup("Enable smart position"), config.payload_variant.position.position_broadcast_smart_enabled);
    purple_request_field_group_add_field(position_config, position_broadcast_smart_enabled);

    // Smart Position Distance
    broadcast_smart_minimum_distance = purple_request_field_int_new("broadcast_smart_minimum_distance", g_strdup("Smart position distance (meters)"), config.payload_variant.position.broadcast_smart_minimum_distance);
    purple_request_field_group_add_field(position_config, broadcast_smart_minimum_distance);

    // Smart Position Interval
    broadcast_smart_minimum_interval_secs = purple_request_field_int_new("broadcast_smart_minimum_interval_secs", g_strdup("Smart position min interval (seconds)"), config.payload_variant.position.broadcast_smart_minimum_interval_secs);
    purple_request_field_group_add_field(position_config, broadcast_smart_minimum_interval_secs);

    // Fixed Position
    fixed_position = purple_request_field_bool_new("fixed_position", g_strdup("Fixed position"), config.payload_variant.position.fixed_position);
    purple_request_field_group_add_field(position_config, fixed_position);

    // GPS Enabled
    gps_enabled = purple_request_field_bool_new("gps_enabled", g_strdup("GPS enabled"), config.payload_variant.position.gps_enabled);
    purple_request_field_group_add_field(position_config, gps_enabled);

    // GPS Update Interval
    gps_update_interval = purple_request_field_int_new("gps_update_interval", g_strdup("GPS poll interval (seconds)"), config.payload_variant.position.gps_update_interval);
    purple_request_field_group_add_field(position_config, gps_update_interval);

    // Position Flags
    position_flags = purple_request_field_list_new("position_flags", g_strdup("Include fields"));
    purple_request_field_list_set_multi_select(position_flags, TRUE);
    for (i = 0; i < 10; i++)
    {
        purple_request_field_list_add(position_flags, mt_lookup_position_flags(pow(2, i)), from_int(pow(2, i)));
        if (config.payload_variant.position.position_flags & (uint32_t)pow(2, i))
        {
            purple_request_field_list_add_selected(position_flags, mt_lookup_position_flags(pow(2, i)));
        }
    }
    purple_request_field_group_add_field(position_config, position_flags);

    purple_request_fields_add_group(fields, position_config);

    conf = g_new0(MeshtasticConfigFields, 1);
    con = g_new0(meshtastic_Config, 1);
    memcpy(con, &config, sizeof(meshtastic_Config));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = con;

    purple_request_fields(
        mta->gc,                                                                              // void *handle,
        buddy->alias,                                                                         // const char *title,
        g_strdup("Position Config                                                         "), // const char *primary,
        NULL,
        fields,                              // PurpleRequestFields *fields,
        g_strdup("Save"),                    // const char *ok_text,
        G_CALLBACK(mt_save_position_config), // GCallback ok_cb,
        g_strdup("Cancel"),                  // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),            // GCallback cancel_cb,
        mta->account,                        // PurpleAccount *account,
        NULL,                                // const char *who,
        NULL,                                // PurpleConversation *conv,
        conf                                 // void *user_data
    );
}

// Network Config
void mt_save_network_config(MeshtasticConfigFields *cf)
{
    meshtastic_Config *config = cf->config;
    MeshtasticBuddy *mb;

    // Ethernet enabled
    config->payload_variant.network.eth_enabled = purple_request_fields_get_bool(cf->fields, "eth_enabled");

    // WiFi enabled
    config->payload_variant.network.wifi_enabled = purple_request_fields_get_bool(cf->fields, "wifi_enabled");

    // WiFi SSID
    strcpy(config->payload_variant.network.wifi_ssid, purple_request_fields_get_string(cf->fields, "wifi_ssid"));

    // WiFi PSK
    strcpy(config->payload_variant.network.wifi_psk, purple_request_fields_get_string(cf->fields, "wifi_psk"));

    mb = cf->buddy->proto_data;
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_network_config(cf->mta, mb->id, config->payload_variant.network);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_network_config(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_Config config)
{
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *group = purple_request_field_group_new(NULL);
    PurpleRequestField *eth_enabled;
    PurpleRequestField *wifi_enabled;
    PurpleRequestField *wifi_ssid;
    PurpleRequestField *wifi_psk;
    MeshtasticConfigFields *conf;
    meshtastic_Config *con;

    // Ethernet enabled
    eth_enabled = purple_request_field_bool_new("eth_enabled", g_strdup("Ethernet enabled"), config.payload_variant.network.eth_enabled);
    purple_request_field_group_add_field(group, eth_enabled);

    // WiFi enabled
    wifi_enabled = purple_request_field_bool_new("wifi_enabled", g_strdup("WiFi enabled"), config.payload_variant.network.wifi_enabled);
    purple_request_field_group_add_field(group, wifi_enabled);

    // WiFi SSID
    wifi_ssid = purple_request_field_string_new("wifi_ssid", g_strdup("WiFi SSID"), config.payload_variant.network.wifi_ssid, FALSE);
    purple_request_field_group_add_field(group, wifi_ssid);

    // WiFi PSK
    wifi_psk = purple_request_field_string_new("wifi_psk", g_strdup("WiFi Password"), config.payload_variant.network.wifi_psk, FALSE);
    purple_request_field_string_set_masked(wifi_psk, TRUE);
    purple_request_field_group_add_field(group, wifi_psk);

    purple_request_fields_add_group(fields, group);

    conf = g_new0(MeshtasticConfigFields, 1);
    con = g_new0(meshtastic_Config, 1);
    memcpy(con, &config, sizeof(meshtastic_Config));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = con;

    purple_request_fields(
        mta->gc,                           // void *handle,
        buddy->alias,                      // const char *title,
        g_strdup("Network Configuration"), // const char *primary,
        NULL,
        fields,                             // PurpleRequestFields *fields,
        g_strdup("Save"),                   // const char *ok_text,
        G_CALLBACK(mt_save_network_config), // GCallback ok_cb,
        g_strdup("Cancel"),                 // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),           // GCallback cancel_cb,
        mta->account,                       // PurpleAccount *account,
        NULL,                               // const char *who,
        NULL,                               // PurpleConversation *conv,
        conf                                // void *user_data
    );
}
// Bluetooth Config
void mt_save_bluetooth_config(MeshtasticConfigFields *cf)
{
    PurpleRequestField *mode;
    meshtastic_Config *config = cf->config;
    MeshtasticBuddy *mb;
    GList *list;
    const char *c;

    // Enabled
    config->payload_variant.bluetooth.enabled = purple_request_fields_get_bool(cf->fields, "enabled");

    // Pairing Mode
    mode = purple_request_fields_get_field(cf->fields, "mode");
    list = purple_request_field_list_get_selected(mode);
    c = purple_request_field_list_get_data(mode, (const char *)list->data);
    config->payload_variant.bluetooth.mode = to_int(c);

    // Fixed PIN
    config->payload_variant.bluetooth.fixed_pin = purple_request_fields_get_integer(cf->fields, "fixed_pin");

    mb = cf->buddy->proto_data;
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_bluetooth_config(cf->mta, mb->id, config->payload_variant.bluetooth);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_bluetooth_config(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_Config config)
{
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *group = purple_request_field_group_new(NULL);
    int i;
    PurpleRequestField *enabled;
    PurpleRequestField *mode;
    PurpleRequestField *fixed_pin;

    // Enabled
    enabled = purple_request_field_bool_new("enabled", g_strdup("Enabled"), config.payload_variant.bluetooth.enabled);
    purple_request_field_group_add_field(group, enabled);

    // Mode
    mode = purple_request_field_list_new("mode", g_strdup("PIN mode"));
    purple_request_field_list_set_multi_select(mode, FALSE);
    for (i = 0; i < 3; i++)
    {
        purple_request_field_list_add(mode, mt_lookup_pairing_mode(i), from_int(i));
    }
    purple_request_field_list_add_selected(mode, mt_lookup_pairing_mode(config.payload_variant.bluetooth.mode));
    purple_request_field_group_add_field(group, mode);

    // PIN
    fixed_pin = purple_request_field_int_new("fixed_pin", g_strdup("Fixed pin"), config.payload_variant.bluetooth.fixed_pin);
    purple_request_field_group_add_field(group, fixed_pin);

    MeshtasticConfigFields *conf;
    meshtastic_Config *con;

    purple_request_fields_add_group(fields, group);

    conf = g_new0(MeshtasticConfigFields, 1);
    con = g_new0(meshtastic_Config, 1);
    memcpy(con, &config, sizeof(meshtastic_Config));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = con;

    purple_request_fields(
        mta->gc,                             // void *handle,
        buddy->alias,                        // const char *title,
        g_strdup("Bluetooth Configuration"), // const char *primary,
        NULL,
        fields,                               // PurpleRequestFields *fields,
        g_strdup("Save"),                     // const char *ok_text,
        G_CALLBACK(mt_save_bluetooth_config), // GCallback ok_cb,
        g_strdup("Cancel"),                   // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),             // GCallback cancel_cb,
        mta->account,                         // PurpleAccount *account,
        NULL,                                 // const char *who,
        NULL,                                 // PurpleConversation *conv,
        conf                                  // void *user_data
    );
}

// Security Config
void mt_save_security_config(MeshtasticConfigFields *cf)
{
    meshtastic_Config *config = cf->config;
    guchar *decoded;
    const char *encoded;
    gsize size;
    pb_size_t admin_key_count = 0;
    meshtastic_Config_SecurityConfig_admin_key_t admin_key;
    meshtastic_Config_SecurityConfig_public_key_t public_key;
    meshtastic_Config_SecurityConfig_private_key_t private_key;
    MeshtasticBuddy *mb;

    // Public Key
    encoded = purple_request_fields_get_string(cf->fields, "public_key");
    if (encoded)
    {
        decoded = purple_base64_decode(encoded, &size);
        memcpy(public_key.bytes, decoded, sizeof(guchar) * size);
        public_key.size = size;
        memcpy(&config->payload_variant.security.public_key, &public_key, sizeof(meshtastic_Config_SecurityConfig_public_key_t));
    }

    // Private Key
    encoded = purple_request_fields_get_string(cf->fields, "private_key");
    if (encoded)
    {
        decoded = purple_base64_decode(encoded, &size);
        memcpy(private_key.bytes, decoded, sizeof(guchar) * size);
        private_key.size = size;
        memcpy(&config->payload_variant.security.private_key, &private_key, sizeof(meshtastic_Config_SecurityConfig_private_key_t));
    }

    // Primary Admin Key
    encoded = purple_request_fields_get_string(cf->fields, "primary_admin_key");
    if (encoded)
    {
        decoded = purple_base64_decode(encoded, &size);
        memcpy(admin_key.bytes, decoded, sizeof(guchar) * size);
        admin_key.size = size;
        memcpy(&config->payload_variant.security.admin_key[0], &admin_key, sizeof(meshtastic_Config_SecurityConfig_admin_key_t));
        admin_key_count++;
    }

    // Secondary Admin Key
    encoded = purple_request_fields_get_string(cf->fields, "secondary_admin_key");
    if (encoded)
    {
        decoded = purple_base64_decode(encoded, &size);
        memcpy(admin_key.bytes, decoded, sizeof(guchar) * size);
        admin_key.size = size;
        memcpy(&config->payload_variant.security.admin_key[1], &admin_key, sizeof(meshtastic_Config_SecurityConfig_admin_key_t));
        admin_key_count++;
    }

    // Tertiary Admin Key
    encoded = purple_request_fields_get_string(cf->fields, "tertiary_admin_key");
    if (encoded)
    {
        decoded = purple_base64_decode(encoded, &size);
        memcpy(admin_key.bytes, decoded, sizeof(guchar) * size);
        admin_key.size = size;
        memcpy(&config->payload_variant.security.admin_key[1], &admin_key, sizeof(meshtastic_Config_SecurityConfig_admin_key_t));
        admin_key_count++;
    }
    config->payload_variant.security.admin_key_count = admin_key_count;

    // Admin channel enabled
    config->payload_variant.security.admin_channel_enabled = purple_request_fields_get_bool(cf->fields, "admin_channel_enabled");

    mb = cf->buddy->proto_data;
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_security_config(cf->mta, mb->id, config->payload_variant.security);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_security_config(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_Config config)
{
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *group = purple_request_field_group_new(NULL);
    PurpleRequestField *public_key;
    PurpleRequestField *private_key;
    PurpleRequestField *primary_admin_key;
    PurpleRequestField *secondary_admin_key;
    PurpleRequestField *tertiary_admin_key;
    PurpleRequestField *admin_channel_enabled;
    MeshtasticConfigFields *conf;
    meshtastic_Config *con;

    // Public Key
    public_key = purple_request_field_string_new("public_key", g_strdup("Public key"), purple_base64_encode(config.payload_variant.security.public_key.bytes, config.payload_variant.security.public_key.size), FALSE);
    purple_request_field_group_add_field(group, public_key);

    // Private Key
    private_key = purple_request_field_string_new("private_key", g_strdup("Private key"), purple_base64_encode(config.payload_variant.security.private_key.bytes, config.payload_variant.security.private_key.size), FALSE);
    purple_request_field_group_add_field(group, private_key);

    // Primary Admin Key
    primary_admin_key = purple_request_field_string_new("primary_admin_key", g_strdup("Primary admin key"), purple_base64_encode(config.payload_variant.security.admin_key[0].bytes, config.payload_variant.security.admin_key[0].size), FALSE);
    purple_request_field_group_add_field(group, primary_admin_key);

    // Secondary Admin Key
    secondary_admin_key = purple_request_field_string_new("secondary_admin_key", g_strdup("Secondary admin key"), purple_base64_encode(config.payload_variant.security.admin_key[1].bytes, config.payload_variant.security.admin_key[1].size), FALSE);
    purple_request_field_group_add_field(group, secondary_admin_key);

    // Tertiary Admin Key
    tertiary_admin_key = purple_request_field_string_new("tertiary_admin_key", g_strdup("Tertiary admin key"), purple_base64_encode(config.payload_variant.security.admin_key[2].bytes, config.payload_variant.security.admin_key[2].size), FALSE);
    purple_request_field_group_add_field(group, tertiary_admin_key);

    // admin channel enabled
    admin_channel_enabled = purple_request_field_bool_new("admin_channel_enabled", g_strdup("Allow legacy admin channel"), config.payload_variant.security.admin_channel_enabled);
    purple_request_field_group_add_field(group, admin_channel_enabled);

    purple_request_fields_add_group(fields, group);

    conf = g_new0(MeshtasticConfigFields, 1);
    con = g_new0(meshtastic_Config, 1);
    memcpy(con, &config, sizeof(meshtastic_Config));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = con;

    purple_request_fields(
        mta->gc,                            // void *handle,
        buddy->alias,                       // const char *title,
        g_strdup("Security Configuration"), // const char *primary,
        buddy->alias,
        fields,                              // PurpleRequestFields *fields,
        g_strdup("Save"),                    // const char *ok_text,
        G_CALLBACK(mt_save_security_config), // GCallback ok_cb,
        g_strdup("Cancel"),                  // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),            // GCallback cancel_cb,
        mta->account,                        // PurpleAccount *account,
        NULL,                                // const char *who,
        NULL,                                // PurpleConversation *conv,
        conf                                 // void *user_data
    );
}

// Fixed Position
void mt_save_fixed_position(MeshtasticConfigFields *cf)
{
    char buffer[100];
    float flat, flon;
    int32_t altitude;
    MeshtasticBuddy *mb;
    meshtastic_Position position = meshtastic_Position_init_default;

    // Latitude
    strcpy(buffer, purple_request_fields_get_string(cf->fields, "latitude_i"));
    sscanf(buffer, " %f ", &flat);
    position.has_latitude_i = true;
    position.latitude_i = (int32_t)(flat / 1e-7);

    // Longitude
    strcpy(buffer, purple_request_fields_get_string(cf->fields, "longitude_i"));
    sscanf(buffer, " %f ", &flon);
    position.has_longitude_i = true;
    position.longitude_i = (int32_t)(flon / 1e-7);

    // altitude
    altitude = purple_request_fields_get_integer(cf->fields, "altitude");
    if (altitude)
    {
        position.has_altitude = true;
        position.altitude = altitude;
    }

    position.time = time(NULL);
    mb = cf->buddy->proto_data;
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_fixed_position(cf->mta, mb->id, position);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_fixed_position(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_AdminMessage_session_passkey_t session_passkey)
{
    MeshtasticBuddy *mb = buddy->proto_data;
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *group = purple_request_field_group_new(NULL);
    char buffer[100];
    PurpleRequestField *latitude_i;
    PurpleRequestField *longitude_i;
    PurpleRequestField *altitude;
    MeshtasticConfigFields *conf;

    // Latitude
    sprintf(buffer, "%f", (float)(mb->info.position.latitude_i * 1e-7));
    latitude_i = purple_request_field_string_new("latitude_i", g_strdup("Latitude"), g_strdup(buffer), FALSE);
    purple_request_field_group_add_field(group, latitude_i);

    // Longitude
    sprintf(buffer, "%f", (float)(mb->info.position.longitude_i * 1e-7));
    longitude_i = purple_request_field_string_new("longitude_i", g_strdup("Longitude"), g_strdup(buffer), FALSE);
    purple_request_field_group_add_field(group, longitude_i);

    // Altitude
    altitude = purple_request_field_int_new("altitude", g_strdup("Altitude (meters)"), mb->info.position.altitude);
    purple_request_field_group_add_field(group, altitude);

    purple_request_fields_add_group(fields, group);

    conf = g_new0(MeshtasticConfigFields, 1);
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = NULL;

    purple_request_fields(
        mta->gc,                        // void *handle,
        buddy->alias,                   // const char *title,
        g_strdup("Set Fixed Position"), // const char *primary,
        NULL,
        fields,                             // PurpleRequestFields *fields,
        g_strdup("Save"),                   // const char *ok_text,
        G_CALLBACK(mt_save_fixed_position), // GCallback ok_cb,
        g_strdup("Cancel"),                 // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),           // GCallback cancel_cb,
        mta->account,                       // PurpleAccount *account,
        NULL,                               // const char *who,
        NULL,                               // PurpleConversation *conv,
        conf                                // void *user_data
    );
}

// Channel Config
void mt_save_channel_config(MeshtasticConfigFields *cf)
{
    meshtastic_Channel *channel = cf->config;
    MeshtasticBuddy *mb = cf->buddy->proto_data;
    meshtastic_ChannelSettings empty = meshtastic_ChannelSettings_init_zero;
    PurpleChat *chat = mt_find_blist_chat(cf->mta->account, from_int(channel->index));
    gsize psk_size;
    guchar *decoded;
    meshtastic_ChannelSettings_psk_t channel_psk;
    PurpleRequestField *position_precision;
    GList *selected_position_precision;
    char *pospre;

    // Enabled
    if (purple_request_fields_get_bool(cf->fields, "remove_channel"))
    {
        if (strcmp(channel->settings.name, "admin") == 0)
            cf->mta->admin_channel = 0;
        channel->has_settings = false;
        channel->settings = empty;
        if (chat)
        {
            purple_blist_remove_chat(chat);
        }
    }
    else
    {
        channel->has_settings = true;

        // Role
        if (purple_request_fields_get_bool(cf->fields, "disable_channel"))
        {
            channel->role = meshtastic_Channel_Role_DISABLED;
        }
        else if (channel->index == 0)
        {
            channel->role = meshtastic_Channel_Role_PRIMARY;
        }
        else
        {
            channel->role = meshtastic_Channel_Role_SECONDARY;
        }

        // Name
        strcpy(channel->settings.name, purple_request_fields_get_string(cf->fields, "name"));
        if (chat)
        {
            purple_blist_alias_chat(chat, channel->settings.name);
        }

        // psk
        decoded = purple_base64_decode(purple_request_fields_get_string(cf->fields, "psk"), &psk_size);
        memcpy(channel_psk.bytes, decoded, sizeof(guchar) * psk_size);
        channel_psk.size = psk_size;
        memcpy(&channel->settings.psk, &channel_psk, sizeof(meshtastic_ChannelSettings_psk_t));

        // uplink enabled
        channel->settings.uplink_enabled = purple_request_fields_get_bool(cf->fields, "uplink_enabled");

        // downlink enabled
        channel->settings.downlink_enabled = purple_request_fields_get_bool(cf->fields, "downlink_enabled");

        // position precision
        position_precision = purple_request_fields_get_field(cf->fields, "position_precision");
        selected_position_precision = purple_request_field_list_get_selected(position_precision);
        pospre = purple_request_field_list_get_data(position_precision, (const char *)selected_position_precision->data);
        channel->settings.has_module_settings = true;
        channel->settings.module_settings.position_precision = to_int(pospre);
        if (cf->mta->me->id == mb->id)
        {
            mt_handle_channel(cf->mta, channel);
        }
    }
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_channel_config(cf->mta, mb->id, *channel);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_channel_config_(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_Channel channel)
{
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *channel_config = purple_request_field_group_new(NULL);
    PurpleRequestField *remove_channel;
    PurpleRequestField *disable_channel;
    PurpleRequestField *name;
    PurpleRequestField *psk;
    PurpleRequestField *uplink_enabled;
    PurpleRequestField *downlink_enabled;
    PurpleRequestField *position_precision;
    MeshtasticConfigFields *conf;
    meshtastic_Channel *con;

    // remove channel
    remove_channel = purple_request_field_bool_new("remove_channel", g_strdup("Remove channel"), FALSE);
    purple_request_field_group_add_field(channel_config, remove_channel);

    // disable channel
    disable_channel = purple_request_field_bool_new("disable_channel", g_strdup("Disabled"), (channel.role == meshtastic_Channel_Role_DISABLED));
    purple_request_field_group_add_field(channel_config, disable_channel);

    // name
    name = purple_request_field_string_new("name", g_strdup("Channel name"), channel.settings.name, FALSE);
    purple_request_field_group_add_field(channel_config, name);

    // psk
    psk = purple_request_field_string_new("psk", g_strdup("Pre-shared key"), purple_base64_encode(channel.settings.psk.bytes, channel.settings.psk.size), FALSE);
    purple_request_field_group_add_field(channel_config, psk);

    // uplink enabled
    uplink_enabled = purple_request_field_bool_new("uplink_enabled", g_strdup("Uplink enabled"), channel.settings.uplink_enabled);
    purple_request_field_group_add_field(channel_config, uplink_enabled);

    // downlink enabled
    downlink_enabled = purple_request_field_bool_new("downlink_enabled", g_strdup("Downlink enabled"), channel.settings.downlink_enabled);
    purple_request_field_group_add_field(channel_config, downlink_enabled);

    // position precision
    position_precision = purple_request_field_list_new("position_precision", g_strdup("Position precision"));
    purple_request_field_list_set_multi_select(position_precision, FALSE);
    purple_request_field_list_add(position_precision, g_strdup("Disabled"), "0");
    purple_request_field_list_add(position_precision, g_strdup("Medium"), "16");
    purple_request_field_list_add(position_precision, g_strdup("Full"), "32");
    switch (channel.settings.module_settings.position_precision)
    {
    case 0:
        purple_request_field_list_add_selected(position_precision, "Disabled");
        break;
    case 32:
        purple_request_field_list_add_selected(position_precision, "Full");
        break;
    default:
        purple_request_field_list_add_selected(position_precision, "Medium");
    }
    purple_request_field_group_add_field(channel_config, position_precision);

    purple_request_fields_add_group(fields, channel_config);

    conf = g_new0(MeshtasticConfigFields, 1);
    con = g_new0(meshtastic_Channel, 1);
    memcpy(con, &channel, sizeof(meshtastic_Channel));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = con;
    purple_request_fields(
        mta->gc,                                                           // void *handle,
        channel.settings.name,                                             // const char *title,
        g_strdup_printf("Channel %d config              ", channel.index), // const char *primary,
        NULL,
        fields,                             // PurpleRequestFields *fields,
        g_strdup("Save"),                   // const char *ok_text,
        G_CALLBACK(mt_save_channel_config), // GCallback ok_cb,
        g_strdup("Cancel"),                 // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),           // GCallback cancel_cb,
        mta->account,                       // PurpleAccount *account,
        NULL,                               // const char *who,
        NULL,                               // PurpleConversation *conv,
        conf                                // void *user_data
    );
}

// MQTT Module
void mt_save_mqtt_module(MeshtasticConfigFields *cf)
{
    meshtastic_ModuleConfig *module = cf->config;
    meshtastic_ModuleConfig_MQTTConfig mqtt = module->payload_variant.mqtt;
    PurpleRequestField *position_precision;
    GList *selected_position_precision;
    char *pospre;
    MeshtasticBuddy *mb;

    // Enabled
    mqtt.enabled = purple_request_fields_get_bool(cf->fields, "enabled");

    // Address
    strcpy(mqtt.address, purple_request_fields_get_string(cf->fields, "address"));

    // Username
    strcpy(mqtt.username, purple_request_fields_get_string(cf->fields, "username"));

    // Password
    strcpy(mqtt.password, purple_request_fields_get_string(cf->fields, "password"));

    // Encryption Enabled
    mqtt.encryption_enabled = purple_request_fields_get_bool(cf->fields, "encryption_enabled");

    // JSON Enabled
    mqtt.json_enabled = purple_request_fields_get_bool(cf->fields, "json_enabled");

    // TLS Enabled
    mqtt.tls_enabled = purple_request_fields_get_bool(cf->fields, "tls_enabled");

    // Root topic
    strcpy(mqtt.root, purple_request_fields_get_string(cf->fields, "root"));

    // Proxy Enabled
    mqtt.proxy_to_client_enabled = purple_request_fields_get_bool(cf->fields, "proxy_to_client_enabled");

    // Map Reporting Enabled
    mqtt.map_reporting_enabled = purple_request_fields_get_bool(cf->fields, "map_reporting_enabled");

    // Publish intervals
    mqtt.map_report_settings.publish_interval_secs = purple_request_fields_get_integer(cf->fields, "publish_interval_secs");

    // Position Precision
    position_precision = purple_request_fields_get_field(cf->fields, "position_precision");
    selected_position_precision = purple_request_field_list_get_selected(position_precision);
    pospre = purple_request_field_list_get_data(position_precision, (const char *)selected_position_precision->data);
    mqtt.map_report_settings.position_precision = to_int(pospre);

    mb = cf->buddy->proto_data;
    cf->mta->cb_routing = &mt_acknowledge_cb;
    mt_set_mqtt_module_config(cf->mta, mb->id, mqtt);
    g_free(cf->config);
    g_free(cf);
}

void mt_request_mqtt_module(MeshtasticAccount *mta, PurpleBuddy *buddy, meshtastic_ModuleConfig module)
{
    MeshtasticConfigFields *conf;
    meshtastic_ModuleConfig *con;
    meshtastic_ModuleConfig_MQTTConfig mqtt = module.payload_variant.mqtt;
    PurpleRequestFields *fields = purple_request_fields_new();
    PurpleRequestFieldGroup *group = purple_request_field_group_new(NULL);
    PurpleRequestField *position_precision;
    PurpleRequestField *enabled;
    PurpleRequestField *address;
    PurpleRequestField *username;
    PurpleRequestField *password;
    PurpleRequestField *encryption_enabled;
    PurpleRequestField *json_enabled;
    PurpleRequestField *tls_enabled;
    PurpleRequestField *root;
    PurpleRequestField *proxy_to_client_enabled;
    PurpleRequestField *map_reporting_enabled;
    PurpleRequestField *publish_interval_secs;

    // Enabled
    enabled = purple_request_field_bool_new("enabled", g_strdup("Enabled"), mqtt.enabled);
    purple_request_field_group_add_field(group, enabled);

    // Address
    address = purple_request_field_string_new("address", g_strdup("Address"), mqtt.address, FALSE);
    purple_request_field_group_add_field(group, address);

    // Username
    username = purple_request_field_string_new("username", g_strdup("Username"), mqtt.username, FALSE);
    purple_request_field_group_add_field(group, username);

    // Password
    password = purple_request_field_string_new("password", g_strdup("Password"), mqtt.password, FALSE);
    purple_request_field_group_add_field(group, password);
    purple_request_field_string_set_masked(password, TRUE);

    // Encryption Enabled
    encryption_enabled = purple_request_field_bool_new("encryption_enabled", g_strdup("Encryption enabled"), mqtt.encryption_enabled);
    purple_request_field_group_add_field(group, encryption_enabled);

    // JSON Enabled
    json_enabled = purple_request_field_bool_new("json_enabled", g_strdup("JSON enabled"), mqtt.json_enabled);
    purple_request_field_group_add_field(group, json_enabled);

    // TLS Enabled
    tls_enabled = purple_request_field_bool_new("tls_enabled", g_strdup("TLS enabled"), mqtt.tls_enabled);
    purple_request_field_group_add_field(group, tls_enabled);

    // Root Topic
    root = purple_request_field_string_new("root", g_strdup("Root topic"), mqtt.root, FALSE);
    purple_request_field_group_add_field(group, root);

    // Proxy Enabled
    proxy_to_client_enabled = purple_request_field_bool_new("proxy_to_client_enabled", g_strdup("Phone proxy enabled"), mqtt.proxy_to_client_enabled);
    purple_request_field_group_add_field(group, proxy_to_client_enabled);

    // Map Reporting Enabled
    map_reporting_enabled = purple_request_field_bool_new("map_reporting_enabled", g_strdup("Map reporting enabled"), mqtt.map_reporting_enabled);
    purple_request_field_group_add_field(group, map_reporting_enabled);

    // Publish Interval
    publish_interval_secs = purple_request_field_int_new("publish_interval_secs", g_strdup("Publish interval (seconds)"), mqtt.map_report_settings.publish_interval_secs);
    purple_request_field_group_add_field(group, publish_interval_secs);

    // Position precision
    position_precision = purple_request_field_list_new("position_precision", g_strdup("Position precision"));
    purple_request_field_list_set_multi_select(position_precision, FALSE);
    purple_request_field_list_add(position_precision, g_strdup("Disabled"), "0");
    purple_request_field_list_add(position_precision, g_strdup("Medium"), "16");
    purple_request_field_list_add(position_precision, g_strdup("Full"), "32");

    switch (mqtt.map_report_settings.position_precision)
    {
    case 0:
        purple_request_field_list_add_selected(position_precision, "Disabled");
        break;
    case 32:
        purple_request_field_list_add_selected(position_precision, "Full");
        break;
    default:
        purple_request_field_list_add_selected(position_precision, "Medium");
    }
    purple_request_field_group_add_field(group, position_precision);

    purple_request_fields_add_group(fields, group);

    conf = g_new0(MeshtasticConfigFields, 1);
    con = g_new0(meshtastic_ModuleConfig, 1);
    memcpy(con, &module, sizeof(meshtastic_ModuleConfig));
    conf->mta = mta;
    conf->fields = fields;
    conf->buddy = buddy;
    conf->config = con;

    purple_request_fields(
        mta->gc,                 // void *handle,
        buddy->alias,            // const char *title,
        g_strdup("MQTT Module"), // const char *primary,
        NULL,
        fields,                          // PurpleRequestFields *fields,
        g_strdup("Save"),                // const char *ok_text,
        G_CALLBACK(mt_save_mqtt_module), // GCallback ok_cb,
        g_strdup("Cancel"),              // const char *cancel_text,
        G_CALLBACK(mt_cancel_cb),        // GCallback cancel_cb,
        mta->account,                    // PurpleAccount *account,
        NULL,                            // const char *who,
        NULL,                            // PurpleConversation *conv,
        conf                             // void *user_data
    );
}

static void mt_user_config_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = (PurpleBuddy *)mta->cb_data;
    mt_request_user_config(mta, buddy, admin.get_owner_response);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_lora_config_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = (PurpleBuddy *)mta->cb_data;
    mt_request_lora_config(mta, buddy, admin.get_config_response);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_device_config_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = (PurpleBuddy *)mta->cb_data;
    mt_request_device_config(mta, buddy, admin.get_config_response);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_position_config_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = (PurpleBuddy *)mta->cb_data;
    mt_request_position_config_(mta, buddy, admin.get_config_response);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_network_config_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = (PurpleBuddy *)mta->cb_data;
    mt_request_network_config(mta, buddy, admin.get_config_response);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_bluetooth_config_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = (PurpleBuddy *)mta->cb_data;
    mt_request_bluetooth_config(mta, buddy, admin.get_config_response);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_security_config_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = (PurpleBuddy *)mta->cb_data;
    mt_request_security_config(mta, buddy, admin.get_config_response);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_mqtt_module_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = (PurpleBuddy *)mta->cb_data;
    mt_request_mqtt_module(mta, buddy, admin.get_module_config_response);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_session_key_for_fixed_position_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = (PurpleBuddy *)mta->cb_data;
    mt_request_fixed_position(mta, buddy, admin.session_passkey);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_channel_config_received(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleBuddy *buddy;
    pb_istream_t stream;
    meshtastic_AdminMessage admin = meshtastic_AdminMessage_init_default;
    purple_debug_info(PROTO_NAME, "Channel Config received\n");
    stream = pb_istream_from_buffer(packet->decoded.payload.bytes, packet->decoded.payload.size);
    pb_decode(&stream, meshtastic_AdminMessage_fields, &admin);
    buddy = mt_find_blist_buddy(mta->account, mta->me->id);
    mt_request_channel_config_(mta, buddy, admin.get_channel_response);
    mta->cb_config = NULL;
    mta->cb_data = NULL;
}

static void mt_got_passkey_for_node_reboot(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    mta->cb_routing = &mt_acknowledge_cb;
    mt_request_reboot(mta, packet->from);
    purple_debug_info(PROTO_NAME, "Reboot request sent\n");
    mta->cb_config = NULL;
}

static void mt_nodedb_reset_complete(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    meshtastic_Routing routing = meshtastic_Routing_init_default;
    if (!mt_routing_from_packet(packet, &routing))
        return;
    if (routing.error_reason == meshtastic_Routing_Error_NONE)
    {
        purple_debug_info(PROTO_NAME, "Setting all buddies to offiline\n");
        mt_blist_all_offine(mta->account);
    }
    mta->cb_routing = NULL;
}

static void mt_got_passkey_for_nodedb_reset(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    if (mt_request_reset_nodedb(mta, packet->from))
    {
        if (mta->me->id == packet->from)
        {
            mta->cb_routing = &mt_nodedb_reset_complete;
        }
        else
        {
            mta->cb_routing = &mt_acknowledge_cb;
        }
    }
    mta->cb_config = NULL;
}

static void mt_got_passkey_for_factory_reset(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    if (mt_request_factory_reset(mta, packet->from))
    {
        if (mta->me->id == packet->from)
        {
            mta->cb_routing = &mt_nodedb_reset_complete;
        }
        else
        {
            mta->cb_routing = &mt_acknowledge_cb;
        }
    }
    mta->cb_config = NULL;
}

/**
 *
 * Menu Items
 *
 */
static void mt_config_menu_item(
    PurpleBlistNode *node,
    gpointer userdata,
    meshtastic_AdminMessage_ConfigType config_type,
    cb_config callback)
{

    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;
    buddy = (PurpleBuddy *)node;
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    mta->cb_config = callback;
    mta->cb_data = buddy;
    mt_request_config(mta, mb->id, config_type);
}

static void mt_module_menu_item(
    PurpleBlistNode *node,
    gpointer userdata,
    meshtastic_AdminMessage_ModuleConfigType type,
    cb_config callback)
{
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;
    buddy = (PurpleBuddy *)node;
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    mta->cb_config = callback;
    mta->cb_data = buddy;
    mt_request_module(mta, mb->id, type);
}

static void mt_user_config_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;
    buddy = (PurpleBuddy *)node;
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    mta->cb_config = &mt_user_config_received;
    mta->cb_data = buddy;
    mt_request_owner_config(mta, mb->id);
}

static void mt_lora_config_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    mt_config_menu_item(
        node,
        userdata,
        meshtastic_AdminMessage_ConfigType_LORA_CONFIG,
        mt_lora_config_received);
}

static void mt_device_config_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    mt_config_menu_item(
        node,
        userdata,
        meshtastic_AdminMessage_ConfigType_DEVICE_CONFIG,
        mt_device_config_received);
}

static void mt_position_config_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    mt_config_menu_item(
        node,
        userdata,
        meshtastic_AdminMessage_ConfigType_POSITION_CONFIG,
        mt_position_config_received);
}

static void mt_fixed_position_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    MeshtasticBuddy *mb;
    PurpleBuddy *buddy;
    buddy = (PurpleBuddy *)node;
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    mta->cb_config = &mt_session_key_for_fixed_position_received;
    mta->cb_data = buddy;
    mt_request_session_key(mta, mb->id);
}

static void mt_network_config_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    mt_config_menu_item(
        node,
        userdata,
        meshtastic_AdminMessage_ConfigType_NETWORK_CONFIG,
        mt_network_config_received);
}

static void mt_bluetooth_config_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    mt_config_menu_item(
        node,
        userdata,
        meshtastic_AdminMessage_ConfigType_BLUETOOTH_CONFIG,
        mt_bluetooth_config_received);
}

static void mt_security_config_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    mt_config_menu_item(
        node,
        userdata,
        meshtastic_AdminMessage_ConfigType_SECURITY_CONFIG,
        mt_security_config_received);
}

static void mt_mqtt_module_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    mt_module_menu_item(
        node,
        userdata,
        meshtastic_AdminMessage_ModuleConfigType_MQTT_CONFIG,
        mt_mqtt_module_received);
}

static void mt_buddy_channel_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    MeshtasticBuddy *mb;
    PurpleBuddy *buddy = (PurpleBuddy *)node;
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    mta->cb_config = &mt_channel_config_received;
    mta->cb_data = buddy;
    mt_request_channel_config(mta, mb->id, to_int(userdata));
}

static void mt_traceroute_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;
    buddy = (PurpleBuddy *)node;
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    purple_notify_info(NULL, g_strdup("Traceroute Request"), g_strdup_printf("Traceroute request sent to %s", buddy->alias), "This could take a while ...");
    mt_traceroute_request(mta, mb->id);
}

static void mt_confirm_reset_nodedb(PurpleBuddy *buddy)
{
    MeshtasticBuddy *mb;
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    mb = buddy->proto_data;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    if (!mb)
        return;
    mt_request_session_key(mta, mb->id);
    mta->cb_config = &mt_got_passkey_for_nodedb_reset;
    mta->cb_data = NULL;
}

static void mt_confirm_factory_reset(PurpleBuddy *buddy)
{
    MeshtasticBuddy *mb;
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    mb = buddy->proto_data;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    if (!mb)
        return;
    mt_request_session_key(mta, mb->id);
    mta->cb_config = &mt_got_passkey_for_factory_reset;
    mta->cb_data = NULL;
}

static void mt_reset_nodedb_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticBuddy *mb;
    PurpleBuddy *buddy;
    buddy = (PurpleBuddy *)node;
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);

    purple_request_yes_no(
        gc,
        g_strdup(buddy->alias),
        g_strdup("Reset NodeDB"),
        g_strdup("Are you sure?"),
        1,
        account, NULL, NULL,
        buddy, mt_confirm_reset_nodedb,
        NULL);
}

static void mt_factory_reset_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    PurpleAccount *account;
    PurpleConnection *gc;
    PurpleBuddy *buddy;
    buddy = (PurpleBuddy *)node;
    if (!buddy)
        return;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    purple_request_yes_no(
        gc,
        g_strdup(buddy->alias),
        g_strdup("Factory reset"),
        g_strdup("Are you sure?"),
        1,
        account, NULL, NULL,
        buddy, mt_confirm_factory_reset,
        NULL);
}

static void mt_confirm_reboot(PurpleBuddy *buddy)
{
    MeshtasticBuddy *mb = buddy->proto_data;
    PurpleAccount *account = purple_buddy_get_account(buddy);
    PurpleConnection *gc = purple_account_get_connection(account);
    MeshtasticAccount *mta = gc->proto_data;
    if (!mb)
        return;
    mt_request_session_key(mta, mb->id);
    mta->cb_config = &mt_got_passkey_for_node_reboot;
    mta->cb_data = NULL;
}

static void mt_reboot_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    PurpleAccount *account;
    PurpleConnection *gc;
    PurpleBuddy *buddy = (PurpleBuddy *)node;
    if (!buddy)
        return;
    account = purple_buddy_get_account(buddy);
    gc = purple_account_get_connection(account);
    purple_request_yes_no(
        gc,
        g_strdup(buddy->alias),
        g_strdup("Reboot Node"),
        g_strdup("Are you sure?"),
        1,
        account, NULL, NULL,
        buddy, mt_confirm_reboot,
        NULL);
}

static void mt_edit_channel_menu_item(PurpleBlistNode *node, gpointer userdata)
{
    PurpleChat *chat;
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    GHashTable *components;
    const gchar *chat_id;
    uint32_t channel_index;
    chat = (PurpleChat *)node;
    if (!chat)
        return;
    components = purple_chat_get_components(chat);
    chat_id = g_hash_table_lookup(components, "id");
    channel_index = to_int(chat_id);
    account = purple_chat_get_account(chat);
    gc = purple_account_get_connection(account);
    mta = gc->proto_data;
    mta->cb_config = &mt_channel_config_received;
    mta->cb_data = chat;
    mt_request_channel_config(mta, mta->me->id, channel_index);
}

static GList *mt_blist_node_menu(PurpleBlistNode *node)
{
    GList *items = NULL;
    GList *config_items = NULL;
    GList *channel_items = NULL;
    GList *module_items = NULL;
    PurpleBuddy *buddy;
    PurpleChat *chat;
    PurpleAccount *account;
    PurpleConnection *gc;
    MeshtasticAccount *mta;
    MeshtasticBuddy *mb;
    GHashTable *components;
    char channel_name[12];

    int i;

    if (PURPLE_BLIST_NODE_IS_BUDDY(node))
    {
        buddy = (PurpleBuddy *)node;
        if (!PURPLE_BUDDY_IS_ONLINE(buddy))
        {
            return NULL;
        }

        mb = buddy->proto_data;
        account = buddy->account;
        gc = purple_account_get_connection(account);
        mta = gc->proto_data;

        for (i = 0; i < 8; i++)
        {
            if ((mta->me->id == mb->id) && (mt_get_channel_name(mta, i, channel_name)))
            {
                channel_items = g_list_append(channel_items, purple_menu_action_new(channel_name, PURPLE_CALLBACK(mt_buddy_channel_menu_item), from_int(i), NULL));
            }
            else
            {
                channel_items = g_list_append(channel_items, purple_menu_action_new(g_strdup_printf("Channel %d", i), PURPLE_CALLBACK(mt_buddy_channel_menu_item), from_int(i), NULL));
            }
        }
        config_items = g_list_append(config_items, purple_menu_action_new("User", PURPLE_CALLBACK(mt_user_config_menu_item), NULL, NULL));
        config_items = g_list_append(config_items, purple_menu_action_new("LoRa", PURPLE_CALLBACK(mt_lora_config_menu_item), NULL, NULL));
        config_items = g_list_append(config_items, purple_menu_action_new("Device", PURPLE_CALLBACK(mt_device_config_menu_item), NULL, NULL));
        config_items = g_list_append(config_items, purple_menu_action_new("Position", PURPLE_CALLBACK(mt_position_config_menu_item), NULL, NULL));
        config_items = g_list_append(config_items, purple_menu_action_new("Fixed Position", PURPLE_CALLBACK(mt_fixed_position_menu_item), NULL, NULL));
        config_items = g_list_append(config_items, purple_menu_action_new("Networking", PURPLE_CALLBACK(mt_network_config_menu_item), NULL, NULL));
        config_items = g_list_append(config_items, purple_menu_action_new("Bluetooth", PURPLE_CALLBACK(mt_bluetooth_config_menu_item), NULL, NULL));
        config_items = g_list_append(config_items, purple_menu_action_new("Security", PURPLE_CALLBACK(mt_security_config_menu_item), NULL, NULL));
        module_items = g_list_append(module_items, purple_menu_action_new("MQTT", PURPLE_CALLBACK(mt_mqtt_module_menu_item), NULL, NULL));
        items = g_list_append(items, purple_menu_action_new("Channels", NULL, NULL, channel_items));
        items = g_list_append(items, purple_menu_action_new("Config", NULL, NULL, config_items));
        items = g_list_append(items, purple_menu_action_new("Modules", NULL, NULL, module_items));

        items = g_list_append(items, purple_menu_action_new("Traceroute", PURPLE_CALLBACK(mt_traceroute_menu_item), NULL, NULL));
        items = g_list_append(items, purple_menu_action_new("Reset NodeDB", PURPLE_CALLBACK(mt_reset_nodedb_menu_item), NULL, NULL));
        items = g_list_append(items, purple_menu_action_new("Factory Reset", PURPLE_CALLBACK(mt_factory_reset_menu_item), NULL, NULL));
        items = g_list_append(items, purple_menu_action_new("Reboot", PURPLE_CALLBACK(mt_reboot_menu_item), NULL, NULL));
    }
    else if (PURPLE_BLIST_NODE_IS_CHAT(node))
    {

        chat = (PurpleChat *)node;
        components = purple_chat_get_components(chat);
        if (g_hash_table_lookup(components, "id") != NULL)
            items = g_list_append(items, purple_menu_action_new("Edit Channel", PURPLE_CALLBACK(mt_edit_channel_menu_item), NULL, NULL));
    }

    return g_list_append(NULL, purple_menu_action_new("Meshtastic", NULL, NULL, items));
}

static GList *mt_chat_info(PurpleConnection *gc)
{
    return NULL;
}

/**
 * Purple configuration
 */
static const char *mt_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
    return "meshtastic";
}

static const char *mt_list_emblem(PurpleBuddy *buddy)
{
    MeshtasticBuddy *mb = buddy->proto_data;
    PurpleAccount *account = purple_buddy_get_account(buddy);
    PurpleConnection *gc = purple_account_get_connection(account);
    MeshtasticAccount *mta = gc->proto_data;
    int32_t rssi;
    if (!mb)
    {
        return NULL;
    }

    if (mb->id == mta->me->id)
    {
        return "founder";
    }

    if (mb->packet.via_mqtt)
    {
        return "external";
    }

    if (mb->info.hops_away != 0)
    {
        return NULL;
    }

    rssi = mb->packet.rx_rssi;
    if (rssi != 0)
    {
        if (rssi < -110)
            return "meshtastic-signal-1";
        if (rssi < -90)
            return "meshtastic-signal-2";
        if (rssi < -60)
            return "meshtastic-signal-3";
        if (rssi <= -30)
            return "meshtastic-signal-4";
    }
    return NULL;
}

static void mt_tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full)
{
    mt_user_info_base(buddy, user_info);
}

static GList *mt_status_types(PurpleAccount *account)
{
    GList *types = NULL;
    PurpleStatusType *status;
    status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, NULL, "Online", TRUE, TRUE, FALSE);
    types = g_list_append(types, status);
    status = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, NULL, "Offline", TRUE, TRUE, FALSE);
    types = g_list_append(types, status);
    status = purple_status_type_new_full(PURPLE_STATUS_AWAY, NULL, "Away", TRUE, TRUE, FALSE);
    types = g_list_append(types, status);
    return types;
}

MeshtasticAccount *mt_init_account(PurpleAccount *account)
{
    PurpleConnection *gc = purple_account_get_connection(account);
    MeshtasticAccount *mta = g_new0(MeshtasticAccount, 1);
    meshtastic_Config *config = g_new0(meshtastic_Config, 1);
    meshtastic_DeviceMetadata *metadata = g_new0(meshtastic_DeviceMetadata, 1);
    meshtastic_AdminMessage_session_passkey_t *session_passkey = g_new0(meshtastic_AdminMessage_session_passkey_t, 1);
    mta->account = account;
    mta->gc = gc;
    mta->protobuf_size = 0;
    mta->protobuf_remaining = 0;
    mta->id = 0;
    mta->config = config;
    mta->metadata = metadata;
    mta->got_config = 0;
    mta->admin_channel = 0;
    mta->cb_routing = NULL;
    mta->cb_config = NULL;
    mta->cb_data = NULL;
    mta->mt_status_update_timeout = 0;
    mta->mt_heartbeat_timeout = 0;
    mta->me = NULL;
    mta->session_passkey = session_passkey;
#ifdef _WIN32
    mta->handle = INVALID_HANDLE_VALUE;
    mta->mt_handle_timeout = 0;
#endif
    gc->proto_data = mta;
    return mta;
}

static void mt_login(PurpleAccount *account)
{
    MeshtasticAccount *mta = mt_init_account(account);
    PurpleConnection *gc = mta->gc;
    int connection_type;
    char port[20];
    connection_type = mt_get_connection(mta, port);
    if (mt_connect(mta, port, connection_type) != 0)
    {
        purple_debug_error(PROTO_NAME, "Failed to connect to %s\n", port);
        purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "Failed to connect");
        return;
    }
    mt_request_initial_config(mta);
    mta->mt_status_update_timeout = purple_timeout_add_seconds(20, (GSourceFunc)mt_update_statusses, mta);
    mta->mt_heartbeat_timeout = purple_timeout_add_seconds(10, (GSourceFunc)mt_heartbeat, mta);
    mt_add_groups(mta);
    purple_connection_set_state(gc, PURPLE_CONNECTED);
}

static void mt_close(PurpleConnection *gc)
{
    MeshtasticAccount *mta = gc->proto_data;
    purple_timeout_remove(mta->mt_status_update_timeout);
    purple_timeout_remove(mta->mt_heartbeat_timeout);
#ifdef _WIN32
    if (mta->handle != INVALID_HANDLE_VALUE)
    {
        purple_timeout_remove(mta->mt_handle_timeout);
        CloseHandle(mta->handle);
    }
#endif
    if (gc->inpa)
        purple_input_remove(gc->inpa);
    close(mta->fd);
    g_free(mta->config);
    g_free(mta->metadata);
    g_free(mta->me);
    g_free(mta->session_passkey);
    g_free(mta);
}

static void mt_send_im_cb(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    PurpleConversation *conversation;
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;
    const char *message;
    meshtastic_Routing routing = meshtastic_Routing_init_default;
    if (!mt_routing_from_packet(packet, &routing))
        return;
    buddy = mta->cb_data;
    if (!buddy)
        return;
    mb = buddy->proto_data;
    if (!mb)
        return;
    message = mt_lookup_error_reason(routing.error_reason);
    conversation = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, buddy->name, mta->account);
    if (conversation)
    {
        if (mb && (packet->from != mb->id) && (routing.error_reason == meshtastic_Routing_Error_NONE))
        {
            purple_conversation_write(conversation, mta->me->user.id, "Acknowledged by another node", PURPLE_MESSAGE_SYSTEM, time(NULL));
            return;
        }
        purple_conversation_write(conversation, mta->me->user.id, (const char *)message, routing.error_reason ? PURPLE_MESSAGE_ERROR : PURPLE_MESSAGE_SYSTEM, time(NULL));
    }
    mta->cb_data = NULL;
    mta->cb_routing = NULL;
}

static int mt_send_im(PurpleConnection *gc, const char *who, const char *message, PurpleMessageFlags flags)
{
    MeshtasticAccount *mta = gc->proto_data;
    PurpleBuddy *buddy = purple_find_buddy(mta->account, who);
    MeshtasticBuddy *mb;
    if (!buddy)
        return -1;
    mb = buddy->proto_data;
    mta->cb_routing = &mt_send_im_cb;
    mta->cb_data = buddy;
    purple_debug_info(PROTO_NAME, "Sending \"%s\" to %s\n", message, buddy->alias);
    mt_send_text(mta, message, mb->id, 0);
    return 1;
}

static void mt_get_info(PurpleConnection *gc, const char *username)
{
    MeshtasticAccount *mta;
    PurpleBuddy *buddy;
    MeshtasticBuddy *mb;
    PurpleNotifyUserInfo *user_info;
    float channel_utilization;
    GString *utilization;
    char buffer[100];

    mta = gc->proto_data;
    buddy = purple_find_buddy(mta->account, username);
    user_info = purple_notify_user_info_new();
    mt_user_info_base(buddy, user_info);
    mb = buddy->proto_data;
    if (!mb)
        return;

    if (mb->id == mta->id) {
        purple_notify_user_info_prepend_pair(user_info, g_strdup("Firmware version"), g_strdup_printf("%s", mta->metadata->firmware_version));
    }
    purple_notify_user_info_prepend_pair(user_info, g_strdup("ID"), g_strdup_printf("%u", mb->id));
    purple_notify_user_info_add_pair(user_info, g_strdup("Role"), g_strdup(mt_lookup_role(mb->user.role)));

    if (mb->user.hw_model != 0)
    {
        const char *hw = mt_lookup_hw(mb->user.hw_model);
        purple_notify_user_info_add_pair(user_info, "Hardware", g_strdup(hw));
    }

    if (mb->info.has_device_metrics)
    {
        if (mb->info.device_metrics.has_battery_level)
        {
            if (mb->info.device_metrics.battery_level > 100)
            {
                purple_notify_user_info_add_pair(user_info, "Battery level", g_strdup("Plugged in"));
            }
            else
            {
                purple_notify_user_info_add_pair(user_info, "Battery level", g_strdup_printf("%d%%", mb->info.device_metrics.battery_level));
            }
        }
        if (mb->info.device_metrics.has_voltage)
        {
            purple_notify_user_info_add_pair(user_info, "Battery voltage", g_strdup_printf("%.2fV", mb->info.device_metrics.voltage));
        }
        if (mb->info.device_metrics.has_channel_utilization)
        {
            channel_utilization = mb->info.device_metrics.channel_utilization;
            utilization = g_string_new(NULL);
            if (channel_utilization > 50)
            {
                g_string_append_printf(utilization, "<span bgcolor=\"#e6b0aa\" color=\"#000000\"> Problematic </span>");
            }
            else if (channel_utilization > 25)
            {
                g_string_append_printf(utilization, "<span bgcolor=\"#f9e79f\" color=\"#000000\"> Acceptable </span>");
            }
            else
            {
                g_string_append_printf(utilization, "<span bgcolor=\"#a9dfbf\" color=\"#000000\"> Optimal </span>");
            }
            g_string_append_printf(utilization, " (%.2f%%)", channel_utilization);
            purple_notify_user_info_add_pair(user_info, g_strdup("Channel utilization"), utilization->str);
            g_string_free(utilization, TRUE);
        }
    }
    if (mb->info.device_metrics.has_air_util_tx)
    {
        purple_notify_user_info_add_pair(user_info, g_strdup("Air Util Tx"), g_strdup_printf("%.2f%%", mb->info.device_metrics.air_util_tx));
    }
    if (mb->info.device_metrics.has_uptime_seconds)
    {
        seconds_to_days_str(mb->info.device_metrics.uptime_seconds, buffer);
        purple_notify_user_info_add_pair(user_info, g_strdup("Upime"), buffer);
    }

    purple_notify_user_info_add_pair(user_info, "Meshtastic Map", g_strdup_printf("<a href=\"https://meshtastic.liamcottle.net/?node_id=%u\">Meshtastic Map</a>", mb->id));

    purple_notify_userinfo(gc, username, user_info, NULL, NULL);
}

static gboolean plugin_load(PurplePlugin *plugin)
{
    return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin)
{
    return TRUE;
}

static void mt_join_chat(PurpleConnection *gc, GHashTable *components)
{

    int id = to_int(g_hash_table_lookup(components, "id"));
    const gchar *name = g_hash_table_lookup(components, "name");
    PurpleConversation *conversation = purple_find_chat(gc, id);
    if (!conversation)
        conversation = serv_got_joined_chat(gc, id, name);
    mt_join_room(gc->proto_data, purple_conversation_get_chat_data(conversation));
}

static void mt_chat_send_cb(MeshtasticAccount *mta, meshtastic_MeshPacket *packet)
{
    int channel_id;
    const char *message;
    meshtastic_Routing routing = meshtastic_Routing_init_default;
    if (!mt_routing_from_packet(packet, &routing))
        return;
    channel_id = *((int *)mta->cb_data);
    message = mt_lookup_error_reason(routing.error_reason);
    serv_got_chat_in(mta->gc, channel_id, mta->me->user.id, routing.error_reason ? PURPLE_MESSAGE_ERROR : PURPLE_MESSAGE_SYSTEM, message, time(NULL));
    free(mta->cb_data);
    mta->cb_data = NULL;
    mta->cb_routing = NULL;
}

static int mt_chat_send(PurpleConnection *gc, int id, const char *message, PurpleMessageFlags flags)
{
    MeshtasticAccount *mta = gc->proto_data;
    purple_debug_info(PROTO_NAME, "Sending to channel %d\n", id);
    serv_got_chat_in(gc, id, mta->me->user.id, PURPLE_MESSAGE_SEND, message, time(NULL));
    mt_channel_message(mta, message, id);
    mta->cb_routing = &mt_chat_send_cb;
    mta->cb_data = malloc(sizeof(int));
    *((int *)mta->cb_data) = id;
    return 1;
}

PurpleChat *mt_find_blist_chat(PurpleAccount *account, const char *id)
{
    PurpleBlistNode *node;
    GHashTable *components;
    const gchar *chat_id;
    for (
        node = purple_blist_get_root();
        node != NULL;
        node = purple_blist_node_next(node, TRUE))
    {
        if (purple_blist_node_get_type(node) == PURPLE_BLIST_CHAT_NODE)
        {
            PurpleChat *chat = PURPLE_CHAT(node);
            if (purple_chat_get_account(chat) != account)
                continue;
            components = purple_chat_get_components(chat);
            chat_id = g_hash_table_lookup(components, "id");
            if (purple_strequal(chat_id, id))
            {
                return chat;
            }
        }
    }
    return NULL;
}

PurpleBuddy *mt_find_blist_buddy(PurpleAccount *account, uint32_t id)
{
    PurpleBlistNode *node;
    MeshtasticBuddy *mb;
    for (
        node = purple_blist_get_root();
        node != NULL;
        node = purple_blist_node_next(node, TRUE))
    {
        if (purple_blist_node_get_type(node) == PURPLE_BLIST_BUDDY_NODE)
        {
            PurpleBuddy *buddy = PURPLE_BUDDY(node);
            if (purple_buddy_get_account(buddy) != account)
                continue;
            mb = buddy->proto_data;
            if (!mb)
            {
                purple_debug_info(PROTO_NAME, "No node info yet for %s\n", buddy->alias);
                continue;
            }
            if (mb->id == id)
            {
                return buddy;
            }
        }
    }
    return mt_unknown_buddy(account, id);
}

void mt_blist_all_offine(PurpleAccount *account)
{
    PurpleBlistNode *node;
    MeshtasticBuddy *mb;
    PurpleConnection *gc = purple_account_get_connection(account);
    MeshtasticAccount *mta = gc->proto_data;

    for (
        node = purple_blist_get_root();
        node != NULL;
        node = purple_blist_node_next(node, TRUE))
    {
        if (purple_blist_node_get_type(node) == PURPLE_BLIST_BUDDY_NODE)
        {
            PurpleBuddy *buddy = PURPLE_BUDDY(node);
            if (purple_buddy_get_account(buddy) != account)
                continue;
            mb = buddy->proto_data;
            if (mb == NULL)
            {
                purple_blist_remove_buddy(buddy);
            }
            else
            {
                if (mb->id != mta->me->id)
                {
                    g_free(mb);
                    purple_blist_remove_buddy(buddy);
                }
            }
        }
    }
}

static gboolean mt_can_receive_file(PurpleConnection *gc, const char *who)
{
    return FALSE;
}

static PurplePluginProtocolInfo prpl_info = {
    OPT_PROTO_NO_PASSWORD | OPT_PROTO_REGISTER_NOSCREENNAME,   /* options */
    NULL,                                                      /* user_splits */
    NULL,                                                      /* protocol_options */
    {"png,gif,jpeg", 0, 0, 96, 96, 0, PURPLE_ICON_SCALE_SEND}, /* icon_spec */
    mt_list_icon,                                              /* list_icon */
    mt_list_emblem,                                            /* list_emblem */
    NULL,                                                      /* status_text */
    mt_tooltip_text,                                           /* tooltip_text */
    mt_status_types,                                           /* status_types */
    mt_blist_node_menu,                                        /* blist_node_menu */
    mt_chat_info,                                              /* chat_info */
    NULL,                                                      /* chat_info_defaults */
    mt_login,                                                  /* login */
    mt_close,                                                  /* close */
    mt_send_im,                                                /* send_im */
    NULL,                                                      /* set_info */
    NULL,                                                      /* send_typing */
    mt_get_info,                                               /* get_info */
    NULL,                                                      /* set_status */
    NULL,                                                      /* set_idle */
    NULL,                                                      /* change_passwd */
    NULL,                                                      /* add_buddy */
    NULL,                                                      /* add_buddies */
    NULL,                                                      /* remove_buddy */
    NULL,                                                      /* remove_buddies */
    NULL,                                                      /* add_permit */
    NULL,                                                      /* add_deny */
    NULL,                                                      /* rem_permit */
    NULL,                                                      /* rem_deny */
    NULL,                                                      /* set_permit_deny */
    mt_join_chat,                                              /* join_chat */
    NULL,                                                      /* reject chat invite */
    NULL,                                                      /* get_chat_name */
    NULL,                                                      /* chat_invite */
    NULL,                                                      /* chat_leave */
    NULL,                                                      /* chat_whisper */
    mt_chat_send,                                              /* chat_send */
    NULL /*mt_keepalive*/,                                     /* keepalive */
    NULL,                                                      /* register_user */
    NULL,                                                      /* get_cb_info */
    NULL,                                                      /* get_cb_away */
    NULL,                                                      /* alias_buddy */
    NULL,                                                      /* group_buddy */
    NULL,                                                      /* rename_group */
    NULL,                                                      /* buddy_free */
    NULL,                                                      /* convo_closed */
    NULL,                                                      /* normalize */
    NULL,                                                      /* set_buddy_icon */
    NULL,                                                      /* remove_group */
    NULL,                                                      /* get_cb_real_name */
    NULL,                                                      /* set_chat_topic */
    mt_find_blist_chat,                                        /* find_blist_chat */
    NULL,                                                      /* roomlist_get_list */
    NULL,                                                      /* roomlist_cancel */
    NULL,                                                      /* roomlist_expand_category */
    mt_can_receive_file,                                       /* can_receive_file */
    NULL,                                                      /* send_file */
    NULL,                                                      /* new_xfer */
    NULL,                                                      /* offline_message */
    NULL,                                                      /* whiteboard_prpl_ops */
    NULL,                                                      /* send_raw */
    NULL,                                                      /* roomlist_room_serialize */
    NULL,                                                      /* unregister_user */
    NULL,                                                      /* send_attention */
    NULL,                                                      /* attention_types */
#if PURPLE_MAJOR_VERSION == 2 && PURPLE_MINOR_VERSION == 1
    (gpointer)
#endif
        sizeof(PurplePluginProtocolInfo), /* struct_size */
    NULL /*MeshtasticAccount_text*/,      /* get_account_text_table */
    NULL,                                 /* initiate_media */
    NULL,                                 /* can_do_media */
    NULL,                                 /* get_moods */
    NULL,                                 /* set_public_alias */
    NULL                                  /* get_public_alias */
#if PURPLE_MAJOR_VERSION == 2 && PURPLE_MINOR_VERSION >= 8
    ,
    NULL, /* add_buddy_with_invite */
    NULL, /* add_buddies_with_invite */
#endif
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    /*	PURPLE_MAJOR_VERSION,
        PURPLE_MINOR_VERSION,
    */
    2,
    1,
    PURPLE_PLUGIN_PROTOCOL,               /* type */
    NULL,                                 /* ui_requirement */
    0,                                    /* flags */
    NULL,                                 /* dependencies */
    PURPLE_PRIORITY_DEFAULT,              /* priority */
    "prpl-dadecoza-meshtastic",           /* id */
    "Meshtastic",                         /* name */
    "1.0",                                /* version */
    "Meshtastic Plugin",                  /* summary */
    "Meshtastic Plugin",                  /* description */
    "Johannes Le Roux <dade@dade.co.za>", /* author */
    "",                                   /* homepage */
    plugin_load,                          /* load */
    plugin_unload,                        /* unload */
    NULL,                                 /* destroy */
    NULL,                                 /* ui_info */
    &prpl_info,                           /* extra_info */
    NULL,                                 /* prefs_info */
    NULL /*plugin_actions*/,              /* actions */
    NULL,                                 /* padding */
    NULL,
    NULL,
    NULL,
};

static void plugin_init(PurplePlugin *plugin)
{
    PurpleAccountUserSplit *split = purple_account_user_split_new(
        g_strdup("COM port or IP address"), /* text shown to user */
        "",                                 /* default value */
        '@');                               /* field separator */
    prpl_info.user_splits = g_list_append(NULL, split);
    _mt_protocol = plugin;
}

PURPLE_INIT_PLUGIN(meshtastic, plugin_init, info);
