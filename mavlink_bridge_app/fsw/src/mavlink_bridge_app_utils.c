#include "mavlink_bridge_app_utils.h"
#include "mavlink_bridge_app_eventids.h"

void MAVLINK_BRIDGE_APP_ReportHousekeeping(void)
{
    MAVLINK_BRIDGE_APP_Data.HkTlm.CommandCounter       = MAVLINK_BRIDGE_APP_Data.CmdCounter;
    MAVLINK_BRIDGE_APP_Data.HkTlm.CommandErrorCounter  = MAVLINK_BRIDGE_APP_Data.ErrCounter;
    MAVLINK_BRIDGE_APP_Data.HkTlm.LinkState            = MAVLINK_BRIDGE_APP_Data.LinkState;
    MAVLINK_BRIDGE_APP_Data.HkTlm.LastErrorCode        = MAVLINK_BRIDGE_APP_Data.LastErrorCode;
    MAVLINK_BRIDGE_APP_Data.HkTlm.ParseErrorCount      = MAVLINK_BRIDGE_APP_Data.ParseErrorCount;
    MAVLINK_BRIDGE_APP_Data.HkTlm.LastRxTimestampMs    = MAVLINK_BRIDGE_APP_Data.LastRxTimestampMs;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(MAVLINK_BRIDGE_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(MAVLINK_BRIDGE_APP_Data.HkTlm.TelemetryHeader), true);
}

bool MAVLINK_BRIDGE_APP_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    size_t ActualLength;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);
    if (ActualLength != ExpectedLength)
    {
        MAVLINK_BRIDGE_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(MAVLINK_BRIDGE_APP_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                          "MAVLINK_BRIDGE_APP: Invalid cmd length expected=%lu actual=%lu",
                          (unsigned long)ExpectedLength, (unsigned long)ActualLength);
        return false;
    }

    return true;
}

void MAVLINK_BRIDGE_APP_SetLinkState(MAVLINK_BRIDGE_APP_LinkState_t NewState)
{
    MAVLINK_BRIDGE_APP_Data.LinkState = (uint8)NewState;
}

void MAVLINK_BRIDGE_APP_RecordParseError(MAVLINK_BRIDGE_APP_ErrorCode_t ErrorCode)
{
    MAVLINK_BRIDGE_APP_Data.LastErrorCode = (uint8)ErrorCode;
    MAVLINK_BRIDGE_APP_Data.ParseErrorCount++;
    CFE_EVS_SendEvent(MAVLINK_BRIDGE_APP_PARSE_EID, CFE_EVS_EventType_INFORMATION,
                      "MAVLINK_BRIDGE_APP: Parse/data error code=%u", (unsigned int)ErrorCode);
}
