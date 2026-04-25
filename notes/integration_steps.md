# telemetry_app reintegration steps

## 1. Prepare official cFS workspace

Clone or fork official `nasa/cFS`, then build the baseline mission first.

## 2. Copy app into workspace

Copy this repository's `telemetry_app/` directory into:

apps/telemetry_app

Copy the helper script into:

tools/telemetry_app_e2e_sender.py

## 3. Register app in sample mission defs

Update:

- `sample_defs/targets.cmake`
- `sample_defs/cpu1_cfe_es_startup.scr`

Add `telemetry_app` to the app list and startup script.

## 4. Build

Typical build flow:

make SIMULATION=native prep
make -j$(nproc)
make install

## 5. Runtime validation

Run:

build/exe/cpu1/core-cpu1

Expected:
- `telemetry_app.so` loads
- `Telemetry App Initialized...`
- system reaches `OPERATIONAL`

## 6. E2E validation

Use:

python3 tools/telemetry_app_e2e_sender.py

Validation target:
- `TELEMETRY_MONITOR_MID` injection
- `ALIVE -> DEGRADED -> LOST -> recovery`
- HK/status payload updates

## 7. Unit-test validation

Configure with:

cmake -S cfe -B build/unit_telemetry -DENABLE_UNIT_TESTS=ON

Then build/run telemetry_app-related testrunners.
