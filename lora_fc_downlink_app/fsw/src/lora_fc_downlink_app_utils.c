#include "lora_fc_downlink_app_utils.h"
#include "lora_fc_downlink_app.h"

void LORA_FC_DOWNLINK_APP_ReportHousekeeping(void)
{
    LORA_FC_DOWNLINK_APP_Data.HkTlm.Payload.CommandCounter          = LORA_FC_DOWNLINK_APP_Data.CmdCounter;
    LORA_FC_DOWNLINK_APP_Data.HkTlm.Payload.CommandErrorCounter     = LORA_FC_DOWNLINK_APP_Data.ErrCounter;
    LORA_FC_DOWNLINK_APP_Data.HkTlm.Payload.DownlinkCount           = LORA_FC_DOWNLINK_APP_Data.DownlinkCount;
    LORA_FC_DOWNLINK_APP_Data.HkTlm.Payload.LastAttitudeTimestampMs = LORA_FC_DOWNLINK_APP_Data.LastAttitudeTimestampMs;
    LORA_FC_DOWNLINK_APP_Data.HkTlm.Payload.LastLocalTimestampMs    = LORA_FC_DOWNLINK_APP_Data.LastLocalTimestampMs;
    LORA_FC_DOWNLINK_APP_Data.HkTlm.Payload.AttitudeValid           = LORA_FC_DOWNLINK_APP_Data.AttitudeValid;
    LORA_FC_DOWNLINK_APP_Data.HkTlm.Payload.LocalValid              = LORA_FC_DOWNLINK_APP_Data.LocalValid;
    LORA_FC_DOWNLINK_APP_Data.HkTlm.Payload.PacketType              = LORA_FC_DOWNLINK_APP_Data.PacketType;

    CFE_SB_TransmitMsg(CFE_MSG_PTR(LORA_FC_DOWNLINK_APP_Data.HkTlm.TelemetryHeader), true);
}
