# LoRa Telemetry Bridge Design

## 1. Purpose

Define the bridge that converts LoRa serial heartbeat input into the telemetry
monitor input contract consumed by `telemetry_app`.

## 2. External Input

### 2.1 Serial Device

Baseline Linux serial device rule:

- use `/dev/serial/by-id/...` when available
- do not depend on `/dev/ttyUSB*` as the primary configured path

### 2.2 Serial Configuration

Baseline settings:

- baud rate: 57600
- transport type: LoRa serial
- active transport identifier: 1

### 2.3 Canonical Heartbeat Payload

Minimum canonical heartbeat payload fields:

- `node_id`
- `seq`
- `tx_time_ms`
- `sensor_ok`
- `crc`

Canonical frame format:

`HB,<node_id>,<seq>,<tx_time_ms>,<sensor_ok>,<crc16_hex>`

Example:

`HB,1,42,12345,1,ABCD`

### 2.4 Manual Bring-Up Payload

For operator testing through PuTTY or equivalent serial terminals, the bridge
shall also accept the following simplified manual test frames:

- `HB`
- `HB,<seq>`
- `HELLO`
- `HELLO,<seq>`

Manual test frames are a bring-up feature only. They are converted into a valid
heartbeat using bridge-local defaults and the current receiver time.

## 3. Valid Frame Rule

A received heartbeat frame shall be treated as valid only when:

- frame parsing succeeds
- for canonical frames, CRC validation succeeds
- sequence progression is acceptable when a sequence value is present
- `tx_time_ms` does not violate receiver-side invalid jump policy

Invalid frames shall not refresh link-health timing.

## 4. Bridge Internal State

The bridge shall maintain at minimum:

- configured serial path
- configured baud rate
- `active_transport_id`
- `last_valid_rx_time`
- `last_valid_seq`
- serial connection status
- optional bridge-local fault counter

## 5. Bridge Output Contract

The bridge shall produce the telemetry monitor input fields required by
`telemetry_app`:

- `active_transport_id`
- `valid`
- `update_age_ms`

Output rules:

- `active_transport_id = 1` for baseline LoRa operation
- `valid = True` only for accepted valid heartbeat frames
- `update_age_ms = current_time - last_valid_rx_time`

## 6. Timing Behavior

Baseline timing constants:

- nominal heartbeat period: 500 ms
- monitor evaluation period: 100 ms
- degraded timeout: 1000 ms
- lost timeout: 3000 ms

The bridge does not classify final link state. Final `ALIVE`, `DEGRADED`, and
`LOST` classification remains the responsibility of `telemetry_app`.

## 7. Error Handling

The bridge shall:

- discard invalid frames without refreshing nominal link timing
- continue running after isolated invalid-frame events
- attempt serial reconnection after disconnect or read failure
- preserve the last valid receive timestamp until a new valid frame is accepted

## 8. Reset and Recovery Boundary

The bridge is responsible for serial-session recovery only.

Bridge responsibilities:

- serial reopen
- parser recovery
- invalid-frame isolation

`telemetry_app` responsibilities:

- `ALIVE` / `DEGRADED` / `LOST` classification
- recovery transition handling
- housekeeping and status publication

System-level reset policy remains outside the bridge and is governed by system
requirements and cFS recovery policy.

## 9. Initial Implementation Steps

1. open serial port
2. read LoRa heartbeat frames
3. validate frame
4. update `last_valid_rx_time`
5. generate `active_transport_id`, `valid`, and `update_age_ms`
6. inject telemetry monitor input toward `telemetry_app`
