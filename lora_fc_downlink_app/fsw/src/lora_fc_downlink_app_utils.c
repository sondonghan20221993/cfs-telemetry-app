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

void LORA_FC_DOWNLINK_APP_ProcessAttitude(const MAVLINK_BRIDGE_APP_AttitudeTlm_t *msg)
{
    memcpy(&LORA_FC_DOWNLINK_APP_Data.LastAttitude, msg, sizeof(*msg));
    LORA_FC_DOWNLINK_APP_Data.AttitudeValid       = msg->Valid;
    LORA_FC_DOWNLINK_APP_Data.LastAttitudeTimestampMs = msg->TimestampMs;
    LORA_FC_DOWNLINK_APP_BuildPacket();
}

void LORA_FC_DOWNLINK_APP_ProcessLocalState(const MAVLINK_BRIDGE_APP_EkfLocalTlm_t *msg)
{
    memcpy(&LORA_FC_DOWNLINK_APP_Data.LastLocal, msg, sizeof(*msg));
    LORA_FC_DOWNLINK_APP_Data.LocalValid       = msg->Valid;
    LORA_FC_DOWNLINK_APP_Data.LastLocalTimestampMs = msg->TimestampMs;
    LORA_FC_DOWNLINK_APP_BuildPacket();
}

void LORA_FC_DOWNLINK_APP_BuildPacket(void)
{
    LORA_FC_DOWNLINK_APP_FcStatePacket_t *packet = &LORA_FC_DOWNLINK_APP_Data.LastPacket;

    packet->PacketType          = LORA_FC_DOWNLINK_APP_Data.PacketType;
    packet->Seq                 = ++LORA_FC_DOWNLINK_APP_Data.DownlinkSequence;
    packet->AttitudeTimestampMs = LORA_FC_DOWNLINK_APP_Data.LastAttitude.TimestampMs;
    packet->LocalTimestampMs    = LORA_FC_DOWNLINK_APP_Data.LastLocal.TimestampMs;
    packet->AttitudeValid       = LORA_FC_DOWNLINK_APP_Data.LastAttitude.Valid;
    packet->LocalValid          = LORA_FC_DOWNLINK_APP_Data.LastLocal.Valid;
    packet->RollRad             = LORA_FC_DOWNLINK_APP_Data.LastAttitude.RollRad;
    packet->PitchRad            = LORA_FC_DOWNLINK_APP_Data.LastAttitude.PitchRad;
    packet->YawRad              = LORA_FC_DOWNLINK_APP_Data.LastAttitude.YawRad;
    packet->X_m                 = LORA_FC_DOWNLINK_APP_Data.LastLocal.X_m;
    packet->Y_m                 = LORA_FC_DOWNLINK_APP_Data.LastLocal.Y_m;
    packet->Z_m                 = LORA_FC_DOWNLINK_APP_Data.LastLocal.Z_m;
    packet->Vx_mps              = LORA_FC_DOWNLINK_APP_Data.LastLocal.Vx_mps;
    packet->Vy_mps              = LORA_FC_DOWNLINK_APP_Data.LastLocal.Vy_mps;
    packet->Vz_mps              = LORA_FC_DOWNLINK_APP_Data.LastLocal.Vz_mps;

    LORA_FC_DOWNLINK_APP_Data.DownlinkCount++;
}
