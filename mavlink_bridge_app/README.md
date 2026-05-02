# mavlink_bridge_app

`mavlink_bridge_app` is a companion cFS application skeleton for receiving
MAVLink data from an ArduPilot-compatible flight controller and converting
selected messages into cFS Software Bus outputs.

## Intended Responsibilities

- receive MAVLink over UART or USB from the FC
- parse selected baseline MAVLink messages
- publish FC EKF local state, attitude state, GPS raw state, and EKF status
- classify failures as link, parse, or data-quality failures
- attempt reconnects before escalating to higher-level recovery

## Baseline MAVLink Inputs

- `LOCAL_POSITION_NED`
- `ATTITUDE`
- `GPS_RAW_INT`
- `EKF_STATUS_REPORT`
- optional `ODOMETRY`

## Baseline cFS Outputs

- `FC_EKF_LOCAL_STATE_MID`
- `FC_ATTITUDE_STATE_MID`
- `FC_GPS_RAW_STATE_MID`
- `FC_EKF_STATUS_MID`
- housekeeping packet

## Current Skeleton Scope

- cFS app lifecycle scaffold
- command pipe and HK request handling
- NOOP and Reset Counters command scaffold
- baseline message structures and publish helpers
- recovery placeholders for reconnect, parse discard, and stale-data marking
