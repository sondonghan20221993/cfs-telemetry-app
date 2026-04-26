# Mission App Runtime Specification Draft

## 1. Purpose

This document defines the first-pass mission application runtime specification for
the cFS-based system. It is intended to guide implementation after the minimum
`telemetry_app` bring-up.

The main goals are:

- Define application responsibilities and Software Bus message contracts.
- Define which runtime values may be changed during mission operation.
- Define how hardware faults, app faults, restart, and reboot behavior are handled.
- Define persistent state that must survive abnormal shutdown and reboot.
- Define active/pending double-buffering rules for critical runtime changes.

This document is a draft. It captures the current intended architecture and must
be reconciled with implementation as each app is added.

## 2. Scope and Current State

The current repository contains a minimum runnable `telemetry_app` derived from
the cFS sample app pattern. It currently demonstrates:

- cFS app load and startup.
- Software Bus pipe creation and subscription.
- Housekeeping and command handling.
- Monitor input handling through `TELEMETRY_APP_MONITOR_MID`.
- Link state reporting through `TELEMETRY_STATUS_MID`.
- `ALIVE`, `DEGRADED`, `LOST`, and recovery state transitions.

The broader mission app set described below is not yet fully implemented in this
repository. This spec is the target contract for subsequent implementation.

## 3. Application Responsibility Model

Each application shall own one clear responsibility and publish its state through
defined MID contracts. Apps shall not directly overwrite another app's owned
state.

| App | Responsibility | Publish MID | Subscribe MID |
| --- | --- | --- | --- |
| `imu_app` | Read IMU and publish inertial state | `IMU_STATE_MID` | TBD |
| `gps_app` | Read GPS/GNSS and publish navigation fix | `GPS_STATE_MID` | TBD |
| `telemetry_app` | Monitor communication or telemetry input health | `TELEMETRY_STATUS_MID` | `TELEMETRY_MONITOR_MID` or external monitor input |
| `img_app` | Capture image and publish image metadata | `IMAGE_META_MID` | TBD |
| `uwb_app` | Estimate UWB position | `UWB_POS_MID` | TBD |
| `recon_app` | Run 3D reconstruction and publish reconstruction status/result references | `RECON_STATUS_MID`, `RECON_RESULT_REF_MID` | `IMAGE_META_MID`, `RECON_REQUEST_MID` |
| `align_app` | Align GPS, IMU, UWB, and reconstruction outputs | `ALIGN_STATE_MID` | `IMU_STATE_MID`, `GPS_STATE_MID`, `UWB_POS_MID`, `RECON_RESULT_REF_MID` |
| `map_app` | Manage accumulated map and map updates | `MAP_UPDATE_MID` | `ALIGN_STATE_MID`, `RECON_RESULT_REF_MID` |
| `cfs_core_app` | Monitor system health and recovery policy | `SYSTEM_HEALTH_MID` | App health/status MIDs |

## 4. MID Contract Rules

Every mission MID shall have a documented owner, producer, consumer list,
payload layout, publication rate, validity rules, and fault behavior.

Each state or telemetry payload should include, unless explicitly exempted:

- `Timestamp`: Measurement or generation time.
- `TimeValid`: Whether the timestamp is trusted.
- `SequenceCounter`: Monotonic counter for stale, duplicate, and dropped data detection.
- `Valid`: Whether the payload may be used by consumers.
- `HealthState`: `NOMINAL`, `DEGRADED`, `LOST`, or `FAILED`.
- `FaultCode`: Last or current fault reason.
- `SourceId`: Sensor, transport, or app instance identifier.
- `AgeMs`: Age of last known good value, where applicable.

The timestamp basis shall be defined before sensor fusion implementation. The
candidate bases are cFS mission elapsed time, GPS time, Unix time, or a monotonic
platform clock. `align_app` shall not fuse inputs whose time basis or time
validity is unknown.

### 4.1 MID Contract Table Template

Each mission MID shall be specified using the following contract fields before
the producing app is implemented.

| Field | Meaning |
| --- | --- |
| MID Name | Symbolic message ID |
| Owner App | App responsible for defining the payload |
| Producer | App or component that publishes the MID |
| Consumers | Apps that subscribe to the MID |
| Command MID | MID used to send commands to the owning app, if applicable |
| Publication Rate | Periodic rate or event-driven condition |
| Payload Layout | Struct definition or field list reference |
| Validity Rule | Required condition for `Valid=true` |
| Fault Behavior | Behavior when producer is degraded, lost, or failed |
| Time Basis | Timestamp source and unit |
| Sequence Rule | Counter increment and wrap behavior |

The complete per-MID contract table shall be populated before implementing each
app. Until finalized, the MID contract fields are listed as TBD in Section 3 and
tracked in Section 16.

### 4.2 Time Basis Validity Policy

A payload time basis shall be considered known only when the producer declares
the timestamp source, timestamp unit, and `TimeValid=true`.

`align_app` shall reject any individual fusion input when:

- `TimeValid=false` for that input.
- Timestamp source is unknown.
- Timestamp unit is unknown.
- Timestamp age exceeds the configured validity window.
- Sequence counter indicates stale, duplicate, or out-of-order data beyond the
  configured tolerance.
- Required inputs use incompatible time bases without a defined conversion rule.

Rejecting an input with `TimeValid=false` does not automatically invalidate the
fused output. The fused output may remain valid if the remaining inputs satisfy
the active mode and source-mask policy. If the remaining inputs are insufficient
for the current mode, `align_app` shall publish a degraded or invalid fused
state.

A monotonic platform clock may be used for relative timing if the mission only
requires local ordering and age checks. GPS time or another absolute time basis
shall be required only when absolute timing is needed by the mission policy.

The final time basis selection and validity rules shall be defined before
`align_app` implementation begins and are tracked in Section 16.

## 5. Minimum Payload Candidates

### 5.1 `IMU_STATE_MID`

Minimum fields:

- Timestamp and time validity.
- Sequence counter.
- Valid flag and health state.
- Acceleration X/Y/Z.
- Gyro X/Y/Z.
- Optional attitude estimate: quaternion or roll/pitch/yaw.
- Sensor quality or covariance indicator.
- Fault code.

### 5.2 `GPS_STATE_MID`

Minimum fields:

- Timestamp and time validity.
- Sequence counter.
- Valid flag and health state.
- Latitude, longitude, altitude.
- Fix type.
- Satellite count.
- Accuracy or HDOP/VDOP.
- Fault code.

### 5.3 `TELEMETRY_STATUS_MID`

Minimum fields:

- Link state.
- Active transport ID.
- Last valid update age.
- Degraded transition count.
- Lost transition count.
- Recovery count.
- Fault code.

The meaning of `TELEMETRY_STATUS_MID` shall be kept narrow: it represents
telemetry or communication health, not the full system health state.

### 5.4 `ALIGN_STATE_MID`

Minimum fields:

- Timestamp and time validity.
- Sequence counter.
- Valid flag and health state.
- Fused pose or position.
- Attitude, if produced by alignment.
- Source mask showing whether IMU, GPS, UWB, and reconstruction were used.
- Quality or covariance.
- Fault code.

### 5.5 `MAP_UPDATE_MID`

Minimum fields:

- Timestamp.
- Map version.
- Checkpoint or update reference.
- Valid flag and health state.
- Update result code.
- Fault code.

### 5.6 `RECON_RESULT_REF_MID`

`RECON_RESULT_REF_MID` shall carry a reference to a reconstruction result, not
raw reconstruction data. Large reconstruction outputs shall not be transported
directly through Software Bus messages.

The payload shall identify where the result can be found and whether it is safe
for consumers to use.

Minimum fields:

- Timestamp and time validity.
- Sequence counter.
- Valid flag and health state.
- Result ID.
- Result type.
- Result storage backend.
- Result reference, such as file path, checkpoint ID, shared-memory key, or
  ring-buffer index.
- Result size.
- Result version.
- Producer job ID.
- Fault code.

The final large-result transfer mechanism shall be defined before implementing
`recon_app` and `map_app` and is tracked in Section 16.

## 6. Runtime-Changeable Parameters

Runtime changes shall be divided into three mechanisms:

| Mechanism | Purpose | Examples |
| --- | --- | --- |
| Command buffer | Immediate or one-shot action | enable/disable, reset request, diagnostic capture |
| Table/config buffer | Validated runtime configuration | threshold, timeout, calibration, retry limit |
| Persistent storage | State/config that survives reboot | boot counters, selected config version, map checkpoint |

Apps shall not apply unvalidated runtime parameters directly from incoming
buffers. Each app shall validate range, version, checksum or CRC, and current
mode compatibility before activation.

Each app shall define a command MID for receiving ground or system commands.
Command routing shall be defined before command authorization is finalized.
Until the command MID per app is assigned, command routing is tracked as an open
item in Section 16.

Runtime-changeable examples:

- Sensor enable/disable.
- Sensor timeout thresholds.
- Quality gates.
- Recovery retry limits.
- Telemetry link timeout thresholds.
- Diagnostic capture settings.
- Calibration values, if mission-approved.

Runtime-restricted or boot-time-only examples:

- App task priority.
- Stack size.
- MID assignments.
- Pipe depth.
- Startup order.
- Critical app classification.

Changing a restricted value shall require app restart, processor reset, reboot,
or mission rebuild according to the final operations policy.

## 7. App Period and Priority Policy

App period and app priority shall be specified separately.

App period may be controlled by:

- SCH wakeup messages.
- App-local timeout or polling loop.
- Event-driven input messages.

If an app is driven by SCH, period changes shall be managed by SCH table or
schedule policy, not by changing only the app-local state. If an app is driven by
an internal loop, period changes may be managed by validated app configuration.

App priority shall be treated as a restricted operational parameter. Runtime
priority changes are not part of the baseline policy unless explicitly approved
by a later mission safety review.

## 8. Fault Classification

Faults shall be classified before recovery action is selected.

| Fault Class | Meaning | Example |
| --- | --- | --- |
| `APP_FAULT` | App logic or runtime failure | pipe error, invalid command handling, memory error |
| `HW_FAULT` | Hardware device failure | IMU read timeout, GPS receiver no response |
| `DATA_FAULT` | Device responds but data is unusable | stale value, outlier, invalid fix |
| `LINK_FAULT` | Communication path failure | telemetry link timeout, packet loss |
| `SYSTEM_FAULT` | Shared platform failure | CPU, filesystem, scheduler, power issue |

Sensor apps shall not directly trigger processor reset for a single hardware
fault. A sensor app shall first publish invalid, degraded, lost, or failed state.
Reset, power cycle, failover, safe mode, or system reboot decisions shall be made
by a recovery authority such as `cfs_core_app` or a dedicated health/safety app.

## 9. Hardware Fault Response

Hardware-related fault handling shall follow staged recovery:

| Stage | Condition | App Behavior |
| --- | --- | --- |
| 0 | Normal | Publish valid state |
| 1 | Data quality degraded | Publish valid or degraded with quality/fault detail |
| 2 | Timeout or stale data | Publish invalid or lost state and last-good age |
| 3 | Soft recovery | Reinitialize driver, reopen device, or resubscribe |
| 4 | Hard recovery request | Request bus reset or device power cycle through recovery authority |
| 5 | Failover | Use backup sensor or alternate source, if available |
| 6 | Degraded mode | Continue mission with reduced function |
| 7 | Safe mode request | Request system-level safe mode when mission continuation is unsafe |

Example hardware responses:

- `imu_app`: detect read timeout, saturated values, self-test failure, or sample
  rate drop; publish degraded/lost and request recovery after retry threshold.
- `gps_app`: detect no fix, timestamp jump, low satellite count, or serial
  timeout; publish no-fix/degraded and allow `align_app` to operate without GPS.
- `img_app`: detect frame timeout or corrupt image; publish invalid image
  metadata and prevent `recon_app` from using the frame.
- `uwb_app`: detect insufficient anchors or outlier ranges; publish degraded
  quality and allow `align_app` fallback.

## 10. Recovery Limit and Escalation Policy

cFS does not define fixed retry, restart, or reboot timing values. Recovery
limits shall be mission-defined based on hardware characteristics, fault
criticality, power budget, operational risk, and safe-mode policy.

Each recovery action shall define:

- Trigger condition.
- Retry interval.
- Maximum retry count.
- Escalation target.
- Persistent counter update rule.
- Counter reset condition.
- Required mode or authorization condition.

Recovery actions shall be staged. Apps shall not immediately request processor
reset or reboot for recoverable local faults. A local app shall first report its
fault state through telemetry and housekeeping. System-level recovery decisions
shall be made by the recovery authority.

Default recovery flow:

1. General fault: perform local retry.
2. Repeated fault: request hard recovery through the recovery authority.
3. Continued failure: enter degraded operation, recovery mode, or safe mode.
4. System hang: allow watchdog reset.
5. Repeated reboot: force recovery-mode or safe-mode boot.

### 10.1 Recommended First-Pass Recovery Limits

The following values are first-pass mission defaults. They are not cFS-defined
standards and shall be revised after hardware testing.

| Target | Fault Condition | First Recovery | Retry Interval | Max Retry Count | Escalation |
| --- | --- | --- | --- | --- | --- |
| `imu_app` | IMU read timeout, stale sample, invalid sample rate | Reinitialize driver or reopen device | 10 s | 3 | Mark IMU `FAILED`; request sensor power-cycle through recovery authority |
| `gps_app` | Serial timeout, no receiver response | Reopen serial device or reinitialize receiver interface | 30 s | 5 | Mark GPS `FAILED`; continue navigation without GPS if allowed |
| `uwb_app` | Insufficient anchors, invalid range set, stale range data | Clear range window and restart ranging cycle | 5 s | 5 | Mark UWB `DEGRADED` or `FAILED`; allow `align_app` fallback |
| `img_app` | Frame timeout, corrupt frame, capture failure | Restart capture pipeline | 10 s | 3 | Mark camera `FAILED`; block image-dependent reconstruction |
| `recon_app` | Reconstruction job failure, timeout, invalid output | Abort current job and restart next requested job | Per job boundary | 2 | Mark reconstruction `FAILED`; preserve last valid result reference |
| `telemetry_app` | Monitor input timeout, link lost | Restart transport or monitor input path | 5 s | 3 | Mark link `LOST`; request recovery authority evaluation |
| `align_app` | Required input unavailable or time basis invalid | Reject current fusion frame | Next alignment frame | N/A | Publish invalid/degraded fused state |
| `map_app` | Map update commit failure or checkpoint failure | Retry commit from last valid pending update | 30 s | 3 | Roll back to last valid checkpoint; if rollback target is also invalid or corrupt, mark map `FAILED` and continue with last known valid map or enter degraded mode |
| `cfs_core_app` | Repeated app failure or system fault | Restart failed app if allowed | 60 s | 3 per app | Enter `RECOVERY` or `SAFE` mode |
| Processor | System-level unrecoverable fault | Processor reset | N/A | 1 per 1800 s recovery window | Safe-mode boot if fault repeats |

For frame-based consumers such as `align_app`, retry count is N/A because the
app rejects the current frame and evaluates the next frame independently. Each
frame is processed on its own merits; there is no per-frame retry loop.

The retry interval is counted from the completion or failure detection time of
the previous recovery attempt.

A retry count shall be reset only after the target has remained in a valid or
nominal state for the configured stable period.

Recommended first-pass stable periods:

| Target | Stable Period Before Retry Counter Reset |
| --- | --- |
| Sensor app | 300 s |
| Telemetry link | 300 s |
| Reconstruction job path | 1 successful job |
| Map checkpoint path | 1 successful checkpoint |
| System-level recovery | 1800 s |

### 10.2 Escalation Rule

Recovery escalation shall follow the lowest-risk action first.

Default escalation order:

1. Report fault through event and housekeeping telemetry.
2. Mark current output invalid, degraded, lost, or failed.
3. Retry local operation.
4. Reinitialize local driver, pipe, file handle, or transport.
5. Request hardware reset or device power-cycle through the recovery authority.
6. Fail over to backup device or alternate data source, if available.
7. Continue in degraded mode.
8. Enter recovery mode.
9. Enter safe mode.
10. Request processor reset only for system-level or unrecoverable repeated faults.

Apps shall not skip directly from local fault detection to processor reset unless
explicitly allowed by mission safety policy.

#### 10.2.1 Escalation Decision Rules

| Condition | Required Escalation |
| --- | --- |
| Single invalid sample | Reject sample; keep app running |
| Consecutive invalid samples below retry threshold | Publish degraded state |
| Timeout exceeds retry threshold | Perform local soft recovery |
| Local soft recovery limit exceeded | Request hard recovery through recovery authority |
| Hard recovery limit exceeded | Mark target failed and enter degraded operation |
| Required mission function unavailable | Request `RECOVERY` or `SAFE` mode |
| Repeated app crash within recovery window | Stop restarting app and request recovery authority action |
| Repeated processor reset within reboot loop window | Boot into safe mode and block unsafe actions |

### 10.3 Watchdog Reset Policy

The watchdog shall be used only for failures where the system cannot reliably
continue execution or cannot rely on normal software recovery.

The watchdog shall not be used as the primary recovery method for ordinary
sensor faults, invalid data, missing GPS fix, temporary link loss, or single app
errors.

Watchdog reset may be allowed for:

- Main executive loop hang.
- Scheduler or timebase failure.
- Recovery authority hang.
- Critical app unresponsive beyond configured heartbeat timeout.
- Memory corruption indication.
- Deadlock or task starvation affecting system-level control.
- Repeated failure to service required health checks.

#### 10.3.1 Watchdog Heartbeat Rules

Each critical app shall publish or update a heartbeat at a defined interval.
The recovery authority shall monitor heartbeat age and health state.

Long-running apps shall separate heartbeat liveness from job completion. For
example, `recon_app` may run a long reconstruction job, but it shall continue to
publish heartbeat state while the job is in progress.

| Component | Heartbeat Timeout | First Action | Escalation |
| --- | --- | --- | --- |
| Critical app | 3 missed heartbeats | Restart app if restart is allowed | Enter `RECOVERY` if restart fails |
| Recovery authority | 2 missed health cycles | Watchdog reset | Safe-mode boot if repeated |
| Scheduler/time service | 2 missed cycles | Watchdog reset | Safe-mode boot |
| Non-critical app | 5 missed heartbeats | Mark failed or restart app | Continue degraded if allowed |

Missed heartbeat counts shall be converted to timeout durations using the
configured heartbeat period of each component. For example, a critical app with
a 5 s heartbeat period and a 3-missed-heartbeat threshold has an effective
timeout of 15 s.

The watchdog service interval shall be shorter than the watchdog timeout.
The watchdog timeout shall be long enough to avoid reset during expected peak
processing, file I/O, or reconstruction job boundaries.

Recommended first-pass values:

| Item | Value |
| --- | --- |
| Recovery authority health cycle | 1 s |
| Critical app heartbeat period | 1 s to 5 s |
| Watchdog service period | 1 s |
| Watchdog timeout | 10 s to 30 s |
| Reboot loop detection window | 1800 s |

These values are first-pass defaults and shall be revised after timing tests.

### 10.4 Reboot Loop Prevention Policy

The system shall prevent repeated automatic reboot from keeping the platform or
system in an unsafe or unusable state.

A reboot loop is detected when the number of processor resets exceeds the
allowed limit within a configured time window.

Recommended first-pass rule:

| Condition | Action |
| --- | --- |
| 2 processor resets within 1800 s | Boot into `RECOVERY` mode |
| 3 processor resets within 1800 s | Boot into `SAFE` mode |
| Safe-mode boot caused by reboot loop | Load minimum config and block nonessential app startup |
| Persistent state validation failure | Ignore corrupted record and load default safe config |
| Same app causes 3 restarts within 600 s | Stop restarting that app and mark it `FAILED` |
| Same hardware target fails 3 hard recoveries within 1800 s | Disable target and continue degraded if allowed |

#### 10.4.1 Persistent Counters Required for Reboot Loop Prevention

The following counters shall be stored persistently:

- Boot count.
- Processor reset count.
- Watchdog reset count.
- Last reset reason.
- Last reset timestamp.
- Reboot loop window start timestamp.
- Per-app restart count.
- Per-app last fault code.
- Per-hardware-target recovery attempt count.
- Safe-mode entry reason.
- Last successful nominal-mode timestamp.

Each persistent counter record shall include version, size, CRC/checksum,
timestamp, and generation counter.

#### 10.4.2 Counter Reset Rules

Persistent recovery counters shall not be cleared immediately after reboot.
Counters may be cleared only after the system remains stable for the configured
stable period.

Recommended first-pass reset rules:

| Counter | Reset Condition |
| --- | --- |
| App restart count | App remains `NOMINAL` for 300 s |
| Hardware recovery count | Target remains valid for 300 s |
| Processor reset window | System remains out of reset loop for 1800 s |
| Watchdog reset count | No watchdog reset for 1800 s |
| Safe-mode entry reason | Operator command or validated mission policy |

Manual counter clearing shall require explicit command authorization.

## 11. Persistent State and Reboot Recovery

Persistent state shall be limited to mission state, fault and recovery counters,
last known valid navigation or map references, and operator-modified
configuration. High-rate raw sensor samples shall not be written directly to
persistent storage except through bounded logs or explicit diagnostic capture.

Candidate persistent values:

| Category | Persistent Values |
| --- | --- |
| Boot/fault | Last reset reason, boot count, processor reset count, watchdog reset count, watchdog indication, reboot loop window start timestamp |
| App health | App state, restart count, last fault code |
| Hardware health | Sensor health state, last hardware fault code, recovery attempt count |
| Mission state | Mission phase, active mode, degraded/safe mode entry |
| Navigation | Last valid GPS fix, last valid pose, last valid timestamp |
| Mapping | Map checkpoint ID, map version, last committed update |
| Telemetry | Last link state, last good contact time, active transport ID |
| Config | Operator-modified config version and validated table version |
| Recovery | Pending recovery action, last recovery result |

Reboot handling:

| Event | Recovery Behavior |
| --- | --- |
| App restart | Restore app-local state only if valid and compatible |
| Processor reset or soft boot | Restore persistent config, mission mode, last health, and checkpoints |
| Hard boot or power cycle | Restore only validated persistent state and default safe config |
| Safe mode boot | Load minimum config and expose fault logs; block unsafe recovery actions |

Every persistent record shall include enough integrity metadata for validation,
such as version, size, CRC/checksum, timestamp, and generation counter.

## 12. Active/Pending Configuration Model

Critical apps shall maintain separate active and pending configuration buffers.
Runtime configuration updates shall be written to the pending buffer first and
shall not affect active operation until validation and activation complete.

If validation fails, the app shall continue using the previous active
configuration and report the rejection through event and housekeeping telemetry.

Recommended config state:

- `active_config`: current validated configuration.
- `pending_config`: newly loaded or commanded configuration under validation.
- `previous_config`: optional rollback target for critical apps.
- `config_generation`: incremented on each successful activation.
- `active_config_version`.
- `pending_config_version`.
- `last_config_result`.
- `last_config_error`.

Activation boundaries shall be app-specific:

| App | Activation Boundary |
| --- | --- |
| `imu_app` | Next sample cycle or device reconfiguration boundary |
| `gps_app` | Next fix processing boundary |
| `telemetry_app` | Next monitor evaluation cycle |
| `recon_app` | After current reconstruction job completes |
| `align_app` | Next alignment frame boundary |
| `map_app` | Checkpoint or map commit boundary |
| `cfs_core_app` | Operator-approved activation or safe system boundary |

## 13. Double-Buffered Runtime Data Model

Producer/consumer data shared inside an app or between app tasks shall use a
double-buffered or equivalent atomic handoff model when partial updates would be
unsafe.

Typical model:

- Writer fills `write_buffer`.
- Writer validates the complete sample or result.
- Writer swaps the active readable buffer at a short activation point.
- Reader uses `read_buffer` and never observes a partially written value.

This model is recommended for:

- Latest IMU sample.
- Latest GPS fix.
- Latest fused pose.
- Reconstruction result references.
- Map update references.
- Critical health/recovery policy snapshots.

## 14. Mode and Command Safety

The system shall define mission modes before final command authorization rules:

- `BOOT`
- `INIT`
- `NOMINAL`
- `DEGRADED`
- `RECOVERY`
- `SAFE`
- `SHUTDOWN`

Each command shall define the modes in which it is allowed. Dangerous commands
such as hardware reset, power cycle, table activation, priority change, and safe
mode exit shall require explicit authorization policy in a later command safety
spec.

### 14.1 Per-Mode App Operation Policy

Each app shall define whether it is enabled, suspended, degraded, or blocked in
each mission mode. The table below defines the default system-level policy.
Per-app deviations shall be documented when each app is implemented.

| Mode | Default Policy |
| --- | --- |
| `BOOT` | Start minimum required apps only; sensor and mission apps are not yet running |
| `INIT` | Initialize apps and validate persistent state; apps may publish invalid output during initialization |
| `NOMINAL` | Run all mission-approved apps; all outputs expected valid |
| `DEGRADED` | Run apps whose dependencies remain valid; apps with unavailable inputs shall publish degraded outputs and continue if the active source-mask policy allows |
| `RECOVERY` | Run recovery authority, telemetry, and required diagnostic apps; suspend nonessential mission apps |
| `SAFE` | Run minimum safe configuration; block nonessential or unsafe actions; expose fault logs |
| `SHUTDOWN` | Stop nonessential apps in defined order; preserve required persistent state before power-off |

The final per-app mode table shall be defined before command authorization is
finalized and is tracked in Section 16.

## 15. Testing Requirements

Each app shall support or be testable through:

- Nominal message input.
- Invalid payload length handling.
- Stale, duplicate, and out-of-order sequence handling.
- Timeout and degraded input handling.
- Hardware fault injection, where practical.
- Runtime config load, reject, activate, and rollback.
- App restart and soft boot recovery.
- Persistent state CRC/checksum failure.

The existing `telemetry_app` E2E sender only validates monitor input and link
state behavior. Additional test tools are required for IMU, GPS, UWB, image,
reconstruction, alignment, map, and system health message flows.

## 16. Open Items

- Final MID numeric assignments.
- Final timestamp basis and units.
- Payload type precision and endian policy.
- App startup order, priority, and stack size.
- SCH-driven versus app-local period control for each app.
- Persistent storage backend and write budget.
- Recovery authority implementation: `cfs_core_app`, health/safety app, or
  existing cFS app integration.
- Exact command authorization and table validation rules.
- Mapping between current `telemetry_app` implementation and final
  `TELEMETRY_STATUS_MID` definition.
- Complete MID contract table per app, including owner, producer, consumers,
  command MID, publication rate, payload layout, validity rules, and fault
  behavior.
- Define command MID per app and command routing policy.
- Define time basis and time validity rules for fusion inputs, including the
  minimum condition for a time basis to be considered known by `align_app`.
- Define `RECON_RESULT_REF_MID` payload and large-result transfer policy,
  including the storage backend and reference mechanism.
- Define per-mode app operation policy, including which apps run, suspend, or
  publish invalid output in `BOOT`, `INIT`, `NOMINAL`, `DEGRADED`, `RECOVERY`,
  `SAFE`, and `SHUTDOWN`.
- Define map rollback failure behavior when the last valid checkpoint is missing,
  corrupt, or incompatible.
- Define app-level correctness properties for sequence handling, timeout
  handling, recovery behavior, config activation, rollback, and reboot-loop
  prevention.
- Final numeric recovery constants for retry intervals, retry limits, heartbeat
  periods, watchdog timeout, and reboot loop windows, to be confirmed after
  hardware testing.
