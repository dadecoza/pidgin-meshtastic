#ifndef _MTSTRINGS_H
#define _MTSTRINGS_H
#include <glib.h>
#include "meshtastic/mesh.pb.h"

const char *mt_lookup_hw(meshtastic_HardwareModel model);
const char *mt_lookup_role(meshtastic_Config_DeviceConfig_Role role);
const char *mt_lookup_error_reason(meshtastic_Routing_Error reason);
const char *mt_lookup_region(meshtastic_Config_LoRaConfig_RegionCode code);
const char *mt_lookup_modem_preset(meshtastic_Config_LoRaConfig_ModemPreset preset);
const char *mt_lookup_position_flags(meshtastic_Config_PositionConfig_PositionFlags flag);
const char *mt_lookup_rebroadcast_mode(meshtastic_Config_DeviceConfig_RebroadcastMode mode);
const char *mt_lookup_pairing_mode(meshtastic_Config_BluetoothConfig_PairingMode mode);
GList *mt_lookup_timezones();
#endif