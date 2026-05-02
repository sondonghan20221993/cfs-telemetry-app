#include "mavlink_bridge_app_utils.h"
#include "mavlink_bridge_app_eventids.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static uint32 MAVLINK_BRIDGE_APP_GetTimeMs(void)
{
    CFE_TIME_SysTime_t TimeNow = CFE_TIME_GetTime();
    uint64             TimeMs;

    TimeMs = ((uint64)TimeNow.Seconds * 1000ULL) + ((uint64)TimeNow.Subseconds * 1000ULL / 0x100000000ULL);
    return (uint32)TimeMs;
}

static speed_t MAVLINK_BRIDGE_APP_GetBaudConstant(uint32 Baudrate)
{
    switch (Baudrate)
    {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        default:
            return 0;
    }
}

static void MAVLINK_BRIDGE_APP_CloseSerial(void)
{
    if (MAVLINK_BRIDGE_APP_Data.SerialFd >= 0)
    {
        close(MAVLINK_BRIDGE_APP_Data.SerialFd);
        MAVLINK_BRIDGE_APP_Data.SerialFd = -1;
    }
}

static CFE_Status_t MAVLINK_BRIDGE_APP_OpenSerial(void)
{
    int             Fd;
    struct termios  Tio;
    speed_t         BaudConstant;

    BaudConstant = MAVLINK_BRIDGE_APP_GetBaudConstant(MAVLINK_BRIDGE_APP_SERIAL_BAUDRATE);
    if (BaudConstant == 0)
    {
        MAVLINK_BRIDGE_APP_Data.LastErrorCode = MAVLINK_BRIDGE_ERROR_INVALID_VALUE;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    Fd = open(MAVLINK_BRIDGE_APP_SERIAL_PATH, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (Fd < 0)
    {
        MAVLINK_BRIDGE_APP_Data.LastErrorCode = MAVLINK_BRIDGE_ERROR_SERIAL_OPEN_FAIL;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    if (tcgetattr(Fd, &Tio) != 0)
    {
        close(Fd);
        MAVLINK_BRIDGE_APP_Data.LastErrorCode = MAVLINK_BRIDGE_ERROR_SERIAL_OPEN_FAIL;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    Tio.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    Tio.c_oflag &= ~OPOST;
    Tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    Tio.c_cflag &= ~(CSIZE | PARENB);
    Tio.c_cflag |= CS8;
    cfsetispeed(&Tio, BaudConstant);
    cfsetospeed(&Tio, BaudConstant);
    Tio.c_cflag |= (CLOCAL | CREAD);
#ifdef CRTSCTS
    Tio.c_cflag &= ~CRTSCTS;
#endif
    Tio.c_cc[VMIN]  = 0;
    Tio.c_cc[VTIME] = 0;

    if (tcsetattr(Fd, TCSANOW, &Tio) != 0)
    {
        close(Fd);
        MAVLINK_BRIDGE_APP_Data.LastErrorCode = MAVLINK_BRIDGE_ERROR_SERIAL_OPEN_FAIL;
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    MAVLINK_BRIDGE_APP_Data.SerialFd = Fd;
    MAVLINK_BRIDGE_APP_Data.LastErrorCode = MAVLINK_BRIDGE_ERROR_NONE;
    MAVLINK_BRIDGE_APP_SetLinkState(MAVLINK_BRIDGE_LINK_CONNECTED);
    CFE_EVS_SendEvent(MAVLINK_BRIDGE_APP_LINK_EID, CFE_EVS_EventType_INFORMATION,
                      "MAVLINK_BRIDGE_APP: opened serial path %s at %lu baud",
                      MAVLINK_BRIDGE_APP_SERIAL_PATH, (unsigned long)MAVLINK_BRIDGE_APP_SERIAL_BAUDRATE);
    return CFE_SUCCESS;
}

static void MAVLINK_BRIDGE_APP_HandleReceivedBytes(const uint8 *Buffer, ssize_t Length, uint32 TimestampMs)
{
    (void)Buffer;

    MAVLINK_BRIDGE_APP_Data.LastRxTimestampMs = TimestampMs;
    MAVLINK_BRIDGE_APP_Data.BytesReceived += (uint32)Length;
    MAVLINK_BRIDGE_APP_Data.SequenceCounter++;
    MAVLINK_BRIDGE_APP_Data.LastErrorCode = MAVLINK_BRIDGE_ERROR_NONE;
    MAVLINK_BRIDGE_APP_SetLinkState(MAVLINK_BRIDGE_LINK_CONNECTED);

    /*
    ** Placeholder until MAVLink parsing is integrated. The receive path is
    ** already active and maintains link state plus byte counters.
    */
}

void MAVLINK_BRIDGE_APP_ReportHousekeeping(void)
{
    MAVLINK_BRIDGE_APP_Data.HkTlm.CommandCounter      = MAVLINK_BRIDGE_APP_Data.CmdCounter;
    MAVLINK_BRIDGE_APP_Data.HkTlm.CommandErrorCounter = MAVLINK_BRIDGE_APP_Data.ErrCounter;
    MAVLINK_BRIDGE_APP_Data.HkTlm.LinkState           = MAVLINK_BRIDGE_APP_Data.LinkState;
    MAVLINK_BRIDGE_APP_Data.HkTlm.LastErrorCode       = MAVLINK_BRIDGE_APP_Data.LastErrorCode;
    MAVLINK_BRIDGE_APP_Data.HkTlm.BytesReceived       = MAVLINK_BRIDGE_APP_Data.BytesReceived;
    MAVLINK_BRIDGE_APP_Data.HkTlm.ReconnectAttemptCount = MAVLINK_BRIDGE_APP_Data.ReconnectAttemptCount;
    MAVLINK_BRIDGE_APP_Data.HkTlm.ParseErrorCount     = MAVLINK_BRIDGE_APP_Data.ParseErrorCount;
    MAVLINK_BRIDGE_APP_Data.HkTlm.LastRxTimestampMs   = MAVLINK_BRIDGE_APP_Data.LastRxTimestampMs;

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

void MAVLINK_BRIDGE_APP_ServiceSerial(void)
{
    uint32  NowMs;
    uint8   RxBuffer[MAVLINK_BRIDGE_APP_READ_CHUNK_SIZE];
    ssize_t ReadSize;

    NowMs = MAVLINK_BRIDGE_APP_GetTimeMs();

    if (MAVLINK_BRIDGE_APP_Data.SerialFd < 0)
    {
        if ((NowMs - MAVLINK_BRIDGE_APP_Data.LastReconnectAttemptMs) >= MAVLINK_BRIDGE_APP_Data.ReconnectIntervalMs)
        {
            MAVLINK_BRIDGE_APP_Data.LastReconnectAttemptMs = NowMs;
            MAVLINK_BRIDGE_APP_Data.ReconnectAttemptCount++;
            MAVLINK_BRIDGE_APP_SetLinkState(MAVLINK_BRIDGE_LINK_DISCONNECTED);

            if (MAVLINK_BRIDGE_APP_OpenSerial() != CFE_SUCCESS)
            {
                CFE_EVS_SendEvent(MAVLINK_BRIDGE_APP_LINK_EID, CFE_EVS_EventType_INFORMATION,
                                  "MAVLINK_BRIDGE_APP: serial open failed path=%s errno=%d",
                                  MAVLINK_BRIDGE_APP_SERIAL_PATH, errno);
            }
        }

        return;
    }

    ReadSize = read(MAVLINK_BRIDGE_APP_Data.SerialFd, RxBuffer, sizeof(RxBuffer));
    if (ReadSize > 0)
    {
        MAVLINK_BRIDGE_APP_HandleReceivedBytes(RxBuffer, ReadSize, NowMs);
        return;
    }

    if (ReadSize == 0 || (ReadSize < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)))
    {
        if ((NowMs - MAVLINK_BRIDGE_APP_Data.LastRxTimestampMs) >= MAVLINK_BRIDGE_APP_STALE_TIMEOUT_MS)
        {
            MAVLINK_BRIDGE_APP_Data.LastErrorCode = MAVLINK_BRIDGE_ERROR_READ_TIMEOUT;
            MAVLINK_BRIDGE_APP_SetLinkState(MAVLINK_BRIDGE_LINK_DEGRADED);
        }
        return;
    }

    MAVLINK_BRIDGE_APP_Data.LastErrorCode = MAVLINK_BRIDGE_ERROR_LINK_DOWN;
    MAVLINK_BRIDGE_APP_SetLinkState(MAVLINK_BRIDGE_LINK_DISCONNECTED);
    CFE_EVS_SendEvent(MAVLINK_BRIDGE_APP_LINK_EID, CFE_EVS_EventType_INFORMATION,
                      "MAVLINK_BRIDGE_APP: serial read failed errno=%d, forcing reconnect", errno);
    MAVLINK_BRIDGE_APP_CloseSerial();
}
