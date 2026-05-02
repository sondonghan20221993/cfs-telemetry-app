# img_app

Prototype cFS metadata-only image app.

This prototype does not transport raw image bytes. It publishes `IMAGE_META_MID`
records periodically so the rest of the system can exercise image/telemetry
correlation, timestamp separation, and external payload-reference handling.
