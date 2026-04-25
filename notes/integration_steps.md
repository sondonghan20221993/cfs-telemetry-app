Integration notes for telemetry_app into official nasa/cFS workspace:

1. Copy telemetry_app into apps/telemetry_app
2. Update sample_defs/targets.cmake to include telemetry_app
3. Update sample_defs/cpu1_cfe_es_startup.scr to load telemetry_app
4. Build with:
   make SIMULATION=native prep
   make -j$(nproc)
   make install
5. E2E validation used tools/telemetry_app_e2e_sender.py through CI_LAB UDP passthrough
