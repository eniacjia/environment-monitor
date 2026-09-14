# Hardware configuration

## Bill of materials

- Arduino Uno or fully compatible ATmega328P board
- 2.4-inch MCUFRIEND-style TFT LCD shield with resistive touchscreen
- BME280 breakout module
- Four jumper wires
- USB cable or suitable regulated Uno power supply

## Detected hardware

| Component | Detected/configured value |
|---|---|
| Arduino | Arduino Uno, ATmega328P |
| TFT controller | ILI9341, ID `0x9341` |
| TFT resolution | 240 x 320 pixels |
| Touch technology | Four-wire, single-touch resistive |
| Environmental sensor | BME280, chip ID `0x60` |
| Supported sensor addresses | `0x76` and `0x77` |

## BME280 wiring

The Uno hardware I2C pins cannot be used because the shield uses `A4` for LCD
reset. The firmware therefore implements software I2C.

| BME280 pin | Arduino Uno pin | Notes |
|---|---|---|
| `VIN` or `VCC` | `3.3V` | Safest supply for the sensor |
| `GND` | `GND` | Common ground |
| `SDA` or `SDI` | `D11` | Software-I2C data |
| `SCL` or `SCK` | `D13` | Software-I2C clock |

Leave `CS` and `SDO` unconnected for this configuration. Some breakout boards
require `CS` pulled high to select I2C mode; use the breakout manufacturer's
instructions if it is not already pulled up.

## TFT and touchscreen pin use

| Pins | Function |
|---|---|
| `D2-D9` | TFT parallel data bus; `D8-D9` are shared with touch |
| `A0-A4` | TFT control; `A2-A3` are shared with touch |
| `D10-D13` | Normally connected to the microSD slot |
| `D0-D1` | USB serial and sketch upload |
| `A5` | Not used by this project |

Because the sensor uses `D11` and `D13`, do not insert a microSD card while this
firmware is running.

## Measured touchscreen calibration

```cpp
const int XP = 8, XM = A2, YP = A3, YM = 9;
const int TS_LEFT = 125, TS_RT = 903;
const int TS_TOP = 129, TS_BOT = 878;
```

These values were measured for the specific shield used during development.
Recalibrate if a replacement shield has inaccurate or reversed touch input.

## Power notes

- Start with the BME280 powered from the Uno's `3.3V` output.
- The bare BME280 chip is not 5 V tolerant.
- A breakout labelled `VIN` may include a 5 V regulator and level shifting, but
  this must be confirmed from that module's documentation or schematic.
- Do not power the TFT backlight directly from a GPIO pin.
