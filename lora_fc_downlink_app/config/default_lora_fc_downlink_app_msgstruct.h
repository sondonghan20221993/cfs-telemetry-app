#ifndef DEFAULT_LORA_FC_DOWNLINK_APP_MSGSTRUCT_H
#define DEFAULT_LORA_FC_DOWNLINK_APP_MSGSTRUCT_H

#include "common_types.h"
#include "cfe_msg_hdr.h"
#include "lora_fc_downlink_app_msgdefs.h"

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader;
} LORA_FC_DOWNLINK_APP_NoopCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader;
} LORA_FC_DOWNLINK_APP_ResetCountersCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader;
} LORA_FC_DOWNLINK_APP_SendHkCmd_t;

typedef struct
{
    uint8  CommandErrorCounter;
    uint8  CommandCounter;
    uint8  AttitudeValid;
    uint8  LocalValid;
    uint8  PacketType;
    uint8  Reserved[3];
    uint32 DownlinkCount;
    uint32 LastAttitudeTimestampMs;
    uint32 LastLocalTimestampMs;
} LORA_FC_DOWNLINK_APP_HkPayload_t;

typedef struct
{
    CFE_MSG_TelemetryHeader_t     TelemetryHeader;
    LORA_FC_DOWNLINK_APP_HkPayload_t Payload;
} LORA_FC_DOWNLINK_APP_HkTlm_t;

#endif
