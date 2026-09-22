# Changelog

## 2026-09-21 — startup layout

- Centred the Environmental Monitor title.
- Enlarged and centred the tagline, automatic build number, and attribution.

## 2026-09-21 — three-section UI

- Replaced the humidity badge with a dedicated light-green humidity section.
- Split the adaptive dashboard into temperature, pressure, and humidity thirds.
- Made the startup build identifier compile automatically as `BUILD# YYYYMMDD`.

## 2026-09-21

- Added DHT11 relative-humidity measurement on `A5`.
- Added live DHT11 humidity display to the dashboard.
- Added a compact built-in DHT11 reader with checksum validation.
- Updated hardware, user, firmware, and troubleshooting documentation.

## 2026-09-13

- Added independent eight-hour temperature and pressure charts.
- Expanded each panel to four modes: current, 1 hour, 8 hours, and 24 hours.
- Simplified graph labels to `1 HOUR`, `8 HOUR`, and `24 HOUR`.
- Removed the experimental 3D enclosure models from the project.

## 2026-09-11

- Detected ILI9341 TFT controller (`0x9341`).
- Calibrated resistive touchscreen.
- Added BME280 temperature and pressure acquisition over software I2C.
- Added independent current, one-hour, and 24-hour display modes.
- Added automatic chart scaling and ten-second refresh.
- Added four-orientation touch rotation control.
- Added branded ten-second startup page.
