# DJIA Environmental Monitor

A compact environmental monitor built with an Arduino Uno, a 2.4-inch
MCUFRIEND TFT touchscreen shield, a BME280 sensor, and a DHT11 humidity module.
The device displays temperature and atmospheric pressure as large live readings
or rolling one-hour, eight-hour, and 24-hour charts, with live humidity.

> Local climate at a glance

Built by **DJIA using Codex**. The startup page generates `BUILD# YYYYMMDD`
automatically from the latest firmware compilation date.

## Features

- 2.4-inch 240 x 320 colour touchscreen
- BME280 temperature and pressure readings every 10 seconds
- DHT11 relative-humidity readings every 10 seconds
- Independent display mode for each measurement:
  - large current value
  - one-hour chart
  - eight-hour chart
  - 24-hour chart
- Automatic vertical chart scaling
- Touch the centre target to rotate the screen 90 degrees clockwise
- Portrait and landscape layouts
- Three-section dashboard: temperature, pressure, and humidity
- Ten-second branded startup page with large, centred title, tagline, build
  number, and attribution

## Project contents

| Path | Purpose |
|---|---|
| `firmware/EnvironmentMonitor/EnvironmentMonitor.ino` | Arduino source code |
| `firmware/release/` | Compiled Uno firmware files |
| `hardware/HARDWARE.md` | Wiring, pin use, and bill of materials |
| `docs/USER_GUIDE.md` | Operating instructions |
| `docs/FIRMWARE.md` | Firmware design and build instructions |
| `docs/TROUBLESHOOTING.md` | Diagnostic guidance |

## Quick start

1. Keep the TFT shield mounted on the Arduino Uno.
2. Keep the shield's microSD slot empty.
3. Connect BME280 `SDA` to `D11` and `SCL` to `D13`.
4. Connect BME280 power to `3.3V` and ground to `GND`.
5. Connect DHT11 `DATA` to `A5`, power to `5V`, and ground to `GND`.
6. Upload the sketch for an `arduino:avr:uno` target.
7. After startup, tap the temperature or pressure section to change its mode.

See [hardware/HARDWARE.md](hardware/HARDWARE.md) before connecting power.

## Important limitations

- Chart data currently resides in RAM and is lost after reset or power failure.
- The microSD interface cannot be used because `D11` and `D13` are repurposed
  for software I2C.
- The TFT backlight is wired directly to power on this shield. Software
  brightness control requires a transistor/MOSFET hardware modification.
