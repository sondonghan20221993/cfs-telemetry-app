/************************************************************************
 * LoRa FC Downlink App
 ************************************************************************/

#ifndef LORA_FC_DOWNLINK_APP_H
#define LORA_FC_DOWNLINK_APP_H

#include "cfe.h"
#include "cfe_config.h"

#include "lora_fc_downlink_app_mission_cfg.h"
#include "lora_fc_downlink_app_platform_cfg.h"
#include "lora_fc_downlink_app_perfids.h"
#include "lora_fc_downlink_app_msg.h"
#include "lora_fc_downlink_app_msgids.h"

typedef struct
{
    uint8                           CmdCounter;
    uint8                           ErrCounter;
    uint16                          Reserved;
    uint32                          RunStatus;
    uint32                          DownlinkSequence;
    uint32                          DownlinkCount;
    uint32                          LastAttitudeTimestampMs;
    uint32                          LastLocalTimestampMs;
    uint8                           PacketType;
    uint8                           AttitudeValid;
    uint8                           LocalValid;
    uint8                           Spare;
    CFE_SB_PipeId_t                 CommandPipe;
    LORA_FC_DOWNLINK_APP_HkTlm_t     HkTlm;
} LORA_FC_DOWNLINK_APP_Data_t;

extern LORA_FC_DOWNLINK_APP_Data_t LORA_FC_DOWNLINK_APP_Data;

void         LORA_FC_DOWNLINK_APP_Main(void);
CFE_Status_t LORA_FC_DOWNLINK_APP_Init(void);
bool         LORA_FC_DOWNLINK_APP_VerifyCmdLength(const CFE_MSG_Message_t *msg_ptr, size_t expected_length);
void         LORA_FC_DOWNLINK_APP_ReportHousekeeping(void);

#endif
