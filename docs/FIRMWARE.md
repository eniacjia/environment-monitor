# Firmware notes

## Target

- Board FQBN: `arduino:avr:uno`
- MCU: ATmega328P
- Serial upload port used during development: `COM13`
- TFT controller: ILI9341 (`0x9341`)
- BME280 chip ID: `0x60`
- DHT11 data pin: `A5` (digital pin 19)

## Arduino libraries

Install these libraries through Arduino Library Manager:

- `MCUFRIEND_kbv` 3.0.0 or compatible
- `Adafruit GFX Library`
- `TouchScreen`

The sketch contains its own small software-I2C/BME280 driver and DHT11 protocol
reader. Adafruit BME280, BMP280, and DHT libraries are not required.

## Build with Arduino CLI

```powershell
arduino-cli compile --fqbn arduino:avr:uno firmware/EnvironmentMonitor
arduino-cli upload -p COM13 --fqbn arduino:avr:uno firmware/EnvironmentMonitor
```

Change `COM13` if the Uno appears on another port.

## Architecture

- `beginBMP280()` probes `0x76` and `0x77`, verifies BME280 ID `0x60`, loads
  temperature/pressure calibration coefficients, and configures normal mode.
- `readBMP280()` reads and compensates raw temperature and pressure values.
- `readDHT11()` performs the timed single-wire transaction, validates its
  checksum, and reports relative humidity.
- Software I2C uses open-drain-style pin switching on `D11/D13`.
- `collectHistory()` creates one-minute, eight-minute, and 24-minute averages
  from ten-second measurements.
- `drawPanel()` renders dynamically scaled history charts.
- `drawCurrentPanel()` renders centred, overprinted bold-style current values.
- `drawHumidityBadge()` keeps the latest humidity visible in every mode.
- `readTouch()` maps the measured resistive panel calibration for all four
  display rotations.

## RAM strategy

The Uno has only 2 KB of SRAM. Six arrays of 60 signed 16-bit values retain
temperature and pressure data for the one-hour, eight-hour, and 24-hour views.
Values are stored in tenths of their displayed units. This avoids storing
thousands of individual ten-second samples.

## Persistence status

History is currently volatile. A future EEPROM implementation should use a
circular record layout, sequence counters, and checksums. Avoid rewriting a
single metadata byte every minute because ATmega328P EEPROM endurance is rated
at approximately 100,000 write/erase cycles per cell.
