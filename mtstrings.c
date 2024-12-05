#include "mtstrings.h"

const char *mt_lookup_hw(meshtastic_HardwareModel model)
{
    switch (model)
    {
    case meshtastic_HardwareModel_UNSET:
        return "Unset";
    case meshtastic_HardwareModel_TLORA_V2:
        return "TTGO Lora32 v2";
    case meshtastic_HardwareModel_TLORA_V1:
        return "TTGO Lora32 v1";
    case meshtastic_HardwareModel_TLORA_V2_1_1P6:
        return "TTGO Lora32 v2.1 1.6";
    case meshtastic_HardwareModel_TBEAM:
        return "TTGO T-Beam";
    case meshtastic_HardwareModel_HELTEC_V2_0:
        return "Heltec V2.0";
    case meshtastic_HardwareModel_TBEAM_V0P7:
        return "TTGO T-Beam v0.7";
    case meshtastic_HardwareModel_T_ECHO:
        return "LILYGO T-Echo";
    case meshtastic_HardwareModel_TLORA_V1_1P3:
        return "TTGO Lora32 v1 1.3";
    case meshtastic_HardwareModel_RAK4631:
        return "WisBlock Core";
    case meshtastic_HardwareModel_HELTEC_V2_1:
        return "Heltec v2.1";
    case meshtastic_HardwareModel_HELTEC_V1:
        return "Heltec v1";
    case meshtastic_HardwareModel_LILYGO_TBEAM_S3_CORE:
        return "LILYGO T-Beam S3 Core";
    case meshtastic_HardwareModel_RAK11200:
        return "RAK11200";
    case meshtastic_HardwareModel_NANO_G1:
        return "Nano G1";
    case meshtastic_HardwareModel_TLORA_V2_1_1P8:
        return "TTGO Lora V2.1 1.8";
    case meshtastic_HardwareModel_TLORA_T3_S3:
        return "TTGO Lora T3 S3";
    case meshtastic_HardwareModel_NANO_G1_EXPLORER:
        return "Nano G1 Explorer";
    case meshtastic_HardwareModel_NANO_G2_ULTRA:
        return "Nano G2 Ultra";
    case meshtastic_HardwareModel_LORA_TYPE:
        return "LoRAType";
    case meshtastic_HardwareModel_WIPHONE:
        return "WiPhone";
    case meshtastic_HardwareModel_WIO_WM1110:
        return "WIO Tracker WM1110";
    case meshtastic_HardwareModel_RAK2560:
        return "RAK2560 Solar base station";
    case meshtastic_HardwareModel_HELTEC_HRU_3601:
        return "Heltec HRU-3601";
    case meshtastic_HardwareModel_HELTEC_WIRELESS_BRIDGE:
        return "Heltec Wireless Bridge";
    case meshtastic_HardwareModel_STATION_G1:
        return "Station G1";
    case meshtastic_HardwareModel_RAK11310:
        return "RAK11310 (RP2040 + SX1262)";
    case meshtastic_HardwareModel_SENSELORA_RP2040:
        return "Makerfabs SenseLoRA Receiver (RP2040 + RFM96)";
    case meshtastic_HardwareModel_SENSELORA_S3:
        return "Makerfabs SenseLoRA Industrial Monitor (ESP32-S3 + RFM96)";
    case meshtastic_HardwareModel_CANARYONE:
        return "CanaryOne";
    case meshtastic_HardwareModel_RP2040_LORA:
        return "RP2040 LoRa";
    case meshtastic_HardwareModel_STATION_G2:
        return "Station G2";
    case meshtastic_HardwareModel_LORA_RELAY_V1:
        return "LoRa Relay v1";
    case meshtastic_HardwareModel_NRF52840DK:
        return "Nordic NRF52840DK";
    case meshtastic_HardwareModel_PPR:
        return "Nordic PPR";
    case meshtastic_HardwareModel_GENIEBLOCKS:
        return "GenieBlocks";
    case meshtastic_HardwareModel_NRF52_UNKNOWN:
        return "Unknown NRF52";
    case meshtastic_HardwareModel_PORTDUINO:
        return "Portduino on Linux";
    case meshtastic_HardwareModel_ANDROID_SIM:
        return "Android Simulator";
    case meshtastic_HardwareModel_DIY_V1:
        return "DIY @NanoVHF device";
    case meshtastic_HardwareModel_NRF52840_PCA10059:
        return "nRF52840 Dongle";
    case meshtastic_HardwareModel_DR_DEV:
        return "Custom Disaster Radio esp32 v3";
    case meshtastic_HardwareModel_M5STACK:
        return "M5 esp32 based MCU module";
    case meshtastic_HardwareModel_HELTEC_V3:
        return "Heltec V3";
    case meshtastic_HardwareModel_HELTEC_WSL_V3:
        return "Heltec Wireless Stick Lite";
    case meshtastic_HardwareModel_BETAFPV_2400_TX:
        return "BETAFPV ELRS Micro TX Module";
    case meshtastic_HardwareModel_BETAFPV_900_NANO_TX:
        return "BetaFPV ExpressLRS \"Nano\" TX Module";
    case meshtastic_HardwareModel_RPI_PICO:
        return "Raspberry Pi Pico (W) with Waveshare SX1262";
    case meshtastic_HardwareModel_HELTEC_WIRELESS_TRACKER:
        return "Heltec Wireless Tracker";
    case meshtastic_HardwareModel_HELTEC_WIRELESS_PAPER:
        return "Heltec Wireless Paper";
    case meshtastic_HardwareModel_T_DECK:
        return "LilyGo T-Deck";
    case meshtastic_HardwareModel_T_WATCH_S3:
        return "LilyGo T-Watch S3";
    case meshtastic_HardwareModel_PICOMPUTER_S3:
        return "Picomputer ESP32-S3";
    case meshtastic_HardwareModel_HELTEC_HT62:
        return "Heltec HT-CT62";
    case meshtastic_HardwareModel_EBYTE_ESP32_S3:
        return "EBYTE SPI LoRa module";
    case meshtastic_HardwareModel_ESP32_S3_PICO:
        return "Waveshare ESP32-S3-PICO";
    case meshtastic_HardwareModel_CHATTER_2:
        return "CircuitMess Chatter 2 LLCC68 Lora Module";
    case meshtastic_HardwareModel_HELTEC_WIRELESS_PAPER_V1_0:
        return "Heltec Wireless Paper v1.0";
    case meshtastic_HardwareModel_HELTEC_WIRELESS_TRACKER_V1_0:
        return "Heltec Wireless Tracker";
    case meshtastic_HardwareModel_UNPHONE:
        return "unPhone";
    case meshtastic_HardwareModel_TD_LORAC:
        return "Teledatics TD-LORAC NRF52840";
    case meshtastic_HardwareModel_CDEBYTE_EORA_S3:
        return "CDEBYTE EoRa-S3 board";
    case meshtastic_HardwareModel_TWC_MESH_V4:
        return "Adafruit NRF52840 feather express";
    case meshtastic_HardwareModel_NRF52_PROMICRO_DIY:
        return "Promicro NRF52840";
    case meshtastic_HardwareModel_RADIOMASTER_900_BANDIT_NANO:
        return "RadioMaster 900 Bandit Nano";
    case meshtastic_HardwareModel_HELTEC_CAPSULE_SENSOR_V3:
        return "Heltec Capsule Sensor V3";
    case meshtastic_HardwareModel_HELTEC_VISION_MASTER_T190:
        return "Heltec Vision Master T190";
    case meshtastic_HardwareModel_HELTEC_VISION_MASTER_E213:
        return "Heltec Vision Master E213";
    case meshtastic_HardwareModel_HELTEC_VISION_MASTER_E290:
        return "Heltec Vision Master E290";
    case meshtastic_HardwareModel_HELTEC_MESH_NODE_T114:
        return "Heltec Mesh Node T114 board";
    case meshtastic_HardwareModel_SENSECAP_INDICATOR:
        return "Sensecap Indicator";
    case meshtastic_HardwareModel_TRACKER_T1000_E:
        return "Seeed studio T1000-E tracker card";
    case meshtastic_HardwareModel_RAK3172:
        return "RAK3172 STM32WLE5 Module";
    case meshtastic_HardwareModel_WIO_E5:
        return "Seeed Studio Wio-E5";
    case meshtastic_HardwareModel_RADIOMASTER_900_BANDIT:
        return "RadioMaster 900 Bandit";
    case meshtastic_HardwareModel_ME25LS01_4Y10TD:
        return "Minewsemi ME25LS01";
    case meshtastic_HardwareModel_RP2040_FEATHER_RFM95:
        return "Adafruit Feather RP2040";
    case meshtastic_HardwareModel_M5STACK_COREBASIC:
        return "M5 esp32 based MCU";
    case meshtastic_HardwareModel_M5STACK_CORE2:
        return "M5Stack Core2 ESP32 IoT Development Kit";
    case meshtastic_HardwareModel_RPI_PICO2:
        return "Pico2 with Waveshare Hat";
    case meshtastic_HardwareModel_M5STACK_CORES3:
        return "M5Stack Core3 ESP32 IoT Development Kit";
    case meshtastic_HardwareModel_SEEED_XIAO_S3:
        return "Seeed XIAO S3 DK";
    case meshtastic_HardwareModel_MS24SF1:
        return "Nordic nRF52840 + Semtech SX1262";
    case meshtastic_HardwareModel_TLORA_C6:
        return "Lilygo TLora-C6";
    case meshtastic_HardwareModel_PRIVATE_HW:
        return "Homebrew";
    default:
        return "Unknown";
    }
}

const char *mt_lookup_role(meshtastic_Config_DeviceConfig_Role role)
{
    switch (role)
    {
    case meshtastic_Config_DeviceConfig_Role_CLIENT:
        return "Client";
    case meshtastic_Config_DeviceConfig_Role_CLIENT_MUTE:
        return "Client Mute";
    case meshtastic_Config_DeviceConfig_Role_ROUTER:
        return "Router";
    case meshtastic_Config_DeviceConfig_Role_ROUTER_CLIENT:
        return "Router Client";
    case meshtastic_Config_DeviceConfig_Role_REPEATER:
        return "Repeater";
    case meshtastic_Config_DeviceConfig_Role_TRACKER:
        return "Tracker";
    case meshtastic_Config_DeviceConfig_Role_SENSOR:
        return "Sensor";
    case meshtastic_Config_DeviceConfig_Role_TAK:
        return "TAK";
    case meshtastic_Config_DeviceConfig_Role_CLIENT_HIDDEN:
        return "Client Hidden";
    case meshtastic_Config_DeviceConfig_Role_LOST_AND_FOUND:
        return "Lost and Found";
    case meshtastic_Config_DeviceConfig_Role_TAK_TRACKER:
        return "TAK Tracker";
    default:
        return "Unknown";
    }
}

const char *mt_lookup_error_reason(meshtastic_Routing_Error reason)
{
    switch (reason)
    {
    case meshtastic_Routing_Error_NONE:
        return "Acknowledged";
    case meshtastic_Routing_Error_NO_ROUTE:
        return "No Route";
    case meshtastic_Routing_Error_GOT_NAK:
        return "Got NAK";
    case meshtastic_Routing_Error_TIMEOUT:
        return "Timeout";
    case meshtastic_Routing_Error_NO_INTERFACE:
        return "No Interface";
    case meshtastic_Routing_Error_MAX_RETRANSMIT:
        return "Max Retransmit";
    case meshtastic_Routing_Error_NO_CHANNEL:
        return "No Channel";
    case meshtastic_Routing_Error_TOO_LARGE:
        return "Too Large";
    case meshtastic_Routing_Error_NO_RESPONSE:
        return "No Response";
    case meshtastic_Routing_Error_DUTY_CYCLE_LIMIT:
        return "Duty Cycle Limit";
    case meshtastic_Routing_Error_BAD_REQUEST:
        return "Bad Request";
    case meshtastic_Routing_Error_NOT_AUTHORIZED:
        return "Not Authorized";
    case meshtastic_Routing_Error_PKI_FAILED:
        return "PKI Failed";
    case meshtastic_Routing_Error_PKI_UNKNOWN_PUBKEY:
        return "Unknown Public Key";
    case meshtastic_Routing_Error_ADMIN_BAD_SESSION_KEY:
        return "Admin Bad Session Key";
    case meshtastic_Routing_Error_ADMIN_PUBLIC_KEY_UNAUTHORIZED:
        return "Admin Public Key Unauthorized";
    default:
        return "Error";
    }
}

const char *mt_lookup_region(meshtastic_Config_LoRaConfig_RegionCode code)
{
    switch (code)
    {
    case meshtastic_Config_LoRaConfig_RegionCode_UNSET:
        return "Please set a region";
    case meshtastic_Config_LoRaConfig_RegionCode_US:
        return "United States";
    case meshtastic_Config_LoRaConfig_RegionCode_EU_433:
        return "European Union 433Mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_EU_868:
        return "European Union 868Mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_CN:
        return "China";
    case meshtastic_Config_LoRaConfig_RegionCode_JP:
        return "Japan";
    case meshtastic_Config_LoRaConfig_RegionCode_ANZ:
        return "Australia / New Zealand";
    case meshtastic_Config_LoRaConfig_RegionCode_KR:
        return "Korea";
    case meshtastic_Config_LoRaConfig_RegionCode_TW:
        return "Taiwan";
    case meshtastic_Config_LoRaConfig_RegionCode_RU:
        return "Russia";
    case meshtastic_Config_LoRaConfig_RegionCode_IN:
        return "India";
    case meshtastic_Config_LoRaConfig_RegionCode_NZ_865:
        return "New Zealand 865mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_TH:
        return "Thailand";
    case meshtastic_Config_LoRaConfig_RegionCode_LORA_24:
        return "WLAN Band";
    case meshtastic_Config_LoRaConfig_RegionCode_UA_433:
        return "Ukraine 433mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_UA_868:
        return "Ukraine 868mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_MY_433:
        return "Malaysia 433mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_MY_919:
        return "Malaysia 919mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_SG_923:
        return "Singapore 923mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_PH_433:
        return "Philippines 433mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_PH_868:
        return "Philippines 868mhz";
    case meshtastic_Config_LoRaConfig_RegionCode_PH_915:
        return "Philippines 915mhz";
    }
    return "Unknown";
}

const char *mt_lookup_modem_preset(meshtastic_Config_LoRaConfig_ModemPreset preset)
{
    switch (preset)
    {
    case meshtastic_Config_LoRaConfig_ModemPreset_LONG_FAST:
        return "LongFast";
    case meshtastic_Config_LoRaConfig_ModemPreset_LONG_SLOW:
        return "LongSlow";
    case meshtastic_Config_LoRaConfig_ModemPreset_VERY_LONG_SLOW:
        return "VeryLongSlow";
    case meshtastic_Config_LoRaConfig_ModemPreset_MEDIUM_SLOW:
        return "MediumSlow";
    case meshtastic_Config_LoRaConfig_ModemPreset_MEDIUM_FAST:
        return "MediumFast";
    case meshtastic_Config_LoRaConfig_ModemPreset_SHORT_SLOW:
        return "ShortSlow";
    case meshtastic_Config_LoRaConfig_ModemPreset_SHORT_FAST:
        return "ShortFast";
    case meshtastic_Config_LoRaConfig_ModemPreset_LONG_MODERATE:
        return "LongModerate";
    case meshtastic_Config_LoRaConfig_ModemPreset_SHORT_TURBO:
        return "ShortTurbo";
    }
    return "Unknown";
}

const char *mt_lookup_rebroadcast_mode(meshtastic_Config_DeviceConfig_RebroadcastMode mode)
{
    switch (mode)
    {
    case meshtastic_Config_DeviceConfig_RebroadcastMode_ALL:
        return "All";
    case meshtastic_Config_DeviceConfig_RebroadcastMode_ALL_SKIP_DECODING:
        return "All Skip Decoding";
    case meshtastic_Config_DeviceConfig_RebroadcastMode_LOCAL_ONLY:
        return "Local Only";
    case meshtastic_Config_DeviceConfig_RebroadcastMode_KNOWN_ONLY:
        return "Known Only";
    case meshtastic_Config_DeviceConfig_RebroadcastMode_NONE:
        return "None";
    case meshtastic_Config_DeviceConfig_RebroadcastMode_CORE_PORTNUMS_ONLY:
        return "Core Ports Only";
    }
    return "Unknown";
}

const char *mt_lookup_position_flags(meshtastic_Config_PositionConfig_PositionFlags flag)
{
    switch (flag)
    {
    case meshtastic_Config_PositionConfig_PositionFlags_UNSET:
        return "Unset";
    case meshtastic_Config_PositionConfig_PositionFlags_ALTITUDE:
        return "Altitude";
    case meshtastic_Config_PositionConfig_PositionFlags_ALTITUDE_MSL:
        return "Altitude MSL";
    case meshtastic_Config_PositionConfig_PositionFlags_GEOIDAL_SEPARATION:
        return "Geoidal Separation";
    case meshtastic_Config_PositionConfig_PositionFlags_DOP:
        return "DOP";
    case meshtastic_Config_PositionConfig_PositionFlags_HVDOP:
        return "HVDOP";
    case meshtastic_Config_PositionConfig_PositionFlags_SATINVIEW:
        return "Satellites in View";
    case meshtastic_Config_PositionConfig_PositionFlags_SEQ_NO:
        return "Sequence Number";
    case meshtastic_Config_PositionConfig_PositionFlags_TIMESTAMP:
        return "Timestamp";
    case meshtastic_Config_PositionConfig_PositionFlags_HEADING:
        return "Heading";
    case meshtastic_Config_PositionConfig_PositionFlags_SPEED:
        return "Speed";
    }
    return "Unknown";
}

const char *mt_lookup_pairing_mode(meshtastic_Config_BluetoothConfig_PairingMode mode)
{
    switch (mode)
    {
    case meshtastic_Config_BluetoothConfig_PairingMode_RANDOM_PIN:
        return "Random PIN";
    case meshtastic_Config_BluetoothConfig_PairingMode_FIXED_PIN:
        return "Fixed PIN";
    case meshtastic_Config_BluetoothConfig_PairingMode_NO_PIN:
        return "No PIN";
    }
    return "Unkown";
}

GList *mt_lookup_timezones()
{
    GList *tzdefs = NULL;
    tzdefs = g_list_append(tzdefs, g_strdup("<+00>0<+02>-2,M3.5.0/1,M10.5.0/3"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+01>-1"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+02>-2"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+0330>-3:30"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+03>-3"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+0430>-4:30"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+04>-4"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+0530>-5:30"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+0545>-5:45"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+05>-5"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+0630>-6:30"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+06>-6"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+07>-7"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+0845>-8:45"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+08>-8"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+09>-9"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+1030>-10:30<+11>-11,M10.1.0,M4.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+10>-10"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+11>-11"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+11>-11<+12>,M10.1.0,M4.1.0/3"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+12>-12"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+13>-13"));
    tzdefs = g_list_append(tzdefs, g_strdup("<+14>-14"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-01>1"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-01>1<+00>,M3.5.0/0,M10.5.0/1"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-02>2"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-02>2<-01>,M3.5.0/-1,M10.5.0/0"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-03>3"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-03>3<-02>,M3.2.0,M11.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-04>4"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-04>4<-03>,M10.1.0/0,M3.4.0/0"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-04>4<-03>,M9.1.6/24,M4.1.6/24"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-05>5"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-06>6"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-06>6<-05>,M9.1.6/22,M4.1.6/22"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-07>7"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-08>8"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-0930>9:30"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-09>9"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-10>10"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-11>11"));
    tzdefs = g_list_append(tzdefs, g_strdup("<-12>12"));
    tzdefs = g_list_append(tzdefs, g_strdup("ACST-9:30"));
    tzdefs = g_list_append(tzdefs, g_strdup("ACST-9:30ACDT,M10.1.0,M4.1.0/3"));
    tzdefs = g_list_append(tzdefs, g_strdup("AEST-10"));
    tzdefs = g_list_append(tzdefs, g_strdup("AEST-10AEDT,M10.1.0,M4.1.0/3"));
    tzdefs = g_list_append(tzdefs, g_strdup("AKST9AKDT,M3.2.0,M11.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("AST4"));
    tzdefs = g_list_append(tzdefs, g_strdup("AST4ADT,M3.2.0,M11.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("AWST-8"));
    tzdefs = g_list_append(tzdefs, g_strdup("CAT-2"));
    tzdefs = g_list_append(tzdefs, g_strdup("CET-1"));
    tzdefs = g_list_append(tzdefs, g_strdup("CET-1CEST,M3.5.0,M10.5.0/3"));
    tzdefs = g_list_append(tzdefs, g_strdup("CST-8"));
    tzdefs = g_list_append(tzdefs, g_strdup("CST5CDT,M3.2.0/0,M11.1.0/1"));
    tzdefs = g_list_append(tzdefs, g_strdup("CST6"));
    tzdefs = g_list_append(tzdefs, g_strdup("CST6CDT,M3.2.0,M11.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("ChST-10"));
    tzdefs = g_list_append(tzdefs, g_strdup("EAT-3"));
    tzdefs = g_list_append(tzdefs, g_strdup("EET-2"));
    tzdefs = g_list_append(tzdefs, g_strdup("EET-2EEST,M3.4.4/50,M10.4.4/50"));
    tzdefs = g_list_append(tzdefs, g_strdup("EET-2EEST,M3.5.0,M10.5.0/3"));
    tzdefs = g_list_append(tzdefs, g_strdup("EET-2EEST,M3.5.0/0,M10.5.0/0"));
    tzdefs = g_list_append(tzdefs, g_strdup("EET-2EEST,M3.5.0/3,M10.5.0/4"));
    tzdefs = g_list_append(tzdefs, g_strdup("EET-2EEST,M4.5.5/0,M10.5.4/24"));
    tzdefs = g_list_append(tzdefs, g_strdup("EST5"));
    tzdefs = g_list_append(tzdefs, g_strdup("EST5EDT,M3.2.0,M11.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("GMT0"));
    tzdefs = g_list_append(tzdefs, g_strdup("GMT0BST,M3.5.0/1,M10.5.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("HKT-8"));
    tzdefs = g_list_append(tzdefs, g_strdup("HST10"));
    tzdefs = g_list_append(tzdefs, g_strdup("HST10HDT,M3.2.0,M11.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("IST-1GMT0,M10.5.0,M3.5.0/1"));
    tzdefs = g_list_append(tzdefs, g_strdup("IST-2IDT,M3.4.4/26,M10.5.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("IST-5:30"));
    tzdefs = g_list_append(tzdefs, g_strdup("JST-9"));
    tzdefs = g_list_append(tzdefs, g_strdup("KST-9"));
    tzdefs = g_list_append(tzdefs, g_strdup("MSK-3"));
    tzdefs = g_list_append(tzdefs, g_strdup("MST7"));
    tzdefs = g_list_append(tzdefs, g_strdup("MST7MDT,M3.2.0,M11.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("NST3:30NDT,M3.2.0,M11.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("NZST-12NZDT,M9.5.0,M4.1.0/3"));
    tzdefs = g_list_append(tzdefs, g_strdup("PKT-5"));
    tzdefs = g_list_append(tzdefs, g_strdup("PST-8"));
    tzdefs = g_list_append(tzdefs, g_strdup("PST8PDT,M3.2.0,M11.1.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("SAST-2"));
    tzdefs = g_list_append(tzdefs, g_strdup("SST11"));
    tzdefs = g_list_append(tzdefs, g_strdup("UTC0"));
    tzdefs = g_list_append(tzdefs, g_strdup("WAT-1"));
    tzdefs = g_list_append(tzdefs, g_strdup("WET0WEST,M3.5.0/1,M10.5.0"));
    tzdefs = g_list_append(tzdefs, g_strdup("WIB-7"));
    tzdefs = g_list_append(tzdefs, g_strdup("WIT-9"));
    tzdefs = g_list_append(tzdefs, g_strdup("WITA-8"));
    return tzdefs;
}