#ifndef LORA_FC_DOWNLINK_APP_UTILS_H
#define LORA_FC_DOWNLINK_APP_UTILS_H

#include "mavlink_bridge_app_msg.h"

void LORA_FC_DOWNLINK_APP_ReportHousekeeping(void);
void LORA_FC_DOWNLINK_APP_ProcessAttitude(const MAVLINK_BRIDGE_APP_AttitudeTlm_t *msg);
void LORA_FC_DOWNLINK_APP_ProcessLocalState(const MAVLINK_BRIDGE_APP_EkfLocalTlm_t *msg);
void LORA_FC_DOWNLINK_APP_BuildPacket(void);

#endif
