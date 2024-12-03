#ifndef _MESHTASTIC_H
#define _MESHTASTIC_H

#include <glib.h>
#include <connection.h>
#include <account.h>

#define AWAY_TIME 3600
#define TCP_PORT 4403
#define MESHTASTIC_BUFFER_SIZE 512
#define START_BYTE_1 0x94
#define START_BYTE_2 0xc3

// from libpurple internal.h
#ifdef ENABLE_NLS
#  include <locale.h>
#  include <libintl.h>
#  define _(String) ((const char *)dgettext(PACKAGE, String))
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  include <locale.h>
#  define N_(String) (String)
#  ifndef _
#    define _(String) ((const char *)String)
#  endif
#  define ngettext(Singular, Plural, Number) ((Number == 1) ? ((const char *)Singular) : ((const char *)Plural))
#  define dngettext(Domain, Singular, Plural, Number) ((Number == 1) ? ((const char *)Singular) : ((const char *)Plural))
#endif

enum connection_type {
  meshtastic_serial_connection,
  meshtastic_socket_connection
};

typedef struct
{
    uint32_t id;
    uint32_t last_heard;
    meshtastic_MeshPacket packet;
    meshtastic_User user;
    meshtastic_NodeInfo info;
} MeshtasticBuddy;

typedef struct MeshtasticAccount_
{
    PurpleAccount *account;
    PurpleConnection *gc;
    int fd;
#ifdef _WIN32
    HANDLE handle;
    guint mt_handle_timeout;
#endif
    uint8_t protobuf[MESHTASTIC_BUFFER_SIZE];
    uint16_t protobuf_size;
    uint16_t protobuf_remaining;
    uint32_t id;
    meshtastic_Config *config;
    meshtastic_DeviceMetadata *metadata;
    guint mt_status_update_timeout;
    guint mt_heartbeat_timeout;
    MeshtasticBuddy *me;
    int got_config;
    int admin_channel;
    void (*cb_routing)(struct MeshtasticAccount_ *mta, meshtastic_MeshPacket *packet);
    void (*cb_config)(struct MeshtasticAccount_ *mta, meshtastic_MeshPacket *packet);
    void* cb_data;
    meshtastic_AdminMessage_session_passkey_t *session_passkey;
} MeshtasticAccount;

typedef struct MeshtasticConfigFields_
{
    MeshtasticAccount *mta;
    PurpleRequestFields *fields;
    PurpleBuddy *buddy;
    void* config;
} MeshtasticConfigFields;

typedef void (*cb_routing)(MeshtasticAccount *mta, meshtastic_MeshPacket *packet);
typedef void (*cb_config)(MeshtasticAccount *mta, meshtastic_MeshPacket *packet);

#ifdef _WIN32
static gboolean mt_read_handle(MeshtasticAccount *mta);
#endif

void mt_request_lora_region_config(MeshtasticAccount *mta);
static void mt_from_radio_cb(gpointer data, gint source, PurpleInputCondition cond);
void mt_blist_all_offine(PurpleAccount *account);
PurpleBuddy *mt_find_blist_buddy(PurpleAccount *account, uint32_t id);
PurpleChat *mt_find_blist_chat(PurpleAccount *account, const char *id);
#endif