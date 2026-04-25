# cfs-telemetry-app

`telemetry_app` backup repository extracted from a working cFS integration.

## Contents

- `telemetry_app/`
  - cFS app implementation based on `sample_app`
  - HK/CMD handling
  - `TELEMETRY_STATUS_MID`
  - `TELEMETRY_MONITOR_MID`
  - `ALIVE` / `DEGRADED` / `LOST` / recovery state machine
  - unit-test scaffolding updated for current implementation
- `tools/telemetry_app_e2e_sender.py`
  - helper script used for CI_LAB UDP passthrough end-to-end validation
- `notes/integration_steps.md`
  - notes for reintegrating into an official `nasa/cFS` workspace

## Validation Status

Validated in an official `nasa/cFS` workspace for:

- build/install success
- runtime load of `telemetry_app.so`
- operational startup
- monitor-driven end-to-end state transitions
- telemetry_app unit-test target build and execution

## Re-integration Outline

1. Clone or fork official `nasa/cFS`
2. Copy `telemetry_app/` into `apps/telemetry_app`
3. Register `telemetry_app` in mission/sample defs
4. Build and install the cFS workspace
5. Use `tools/telemetry_app_e2e_sender.py` for E2E validation
