# User guide

## Startup

After power-on or reset, the display shows the startup page for ten seconds:

- Environmental Monitor
- Local climate at a glance
- `BUILD# YYYYMMDD`, generated automatically from the firmware compile date
- Built by DJIA using Codex

All startup text uses a large, centred layout for easier reading.

The monitor then detects the BME280 at address `0x76` or `0x77` and opens the
dashboard.

## Display sections

The screen is divided into three equal sections:

1. **TEMPERATURE** in yellow
2. **PRESSURE** in cyan
3. **HUMIDITY** in light green

Measurements update every ten seconds. Each of the three sections has four
independent selectable modes: current value, 1 hour, 8 hours, and 24 hours.

Tap any section to cycle it independently through four modes:

1. Large, centred current value
2. Rolling one-hour chart
3. Rolling eight-hour chart
4. Rolling 24-hour chart

The unit appears on the label line in the same colour as the reading.

## Rotation

A small two-ring target is drawn at the centre of the screen. Tap the target to
rotate the display 90 degrees clockwise. Four taps return it to the original
orientation. Touch coordinates and panel layouts adjust to each orientation.

## Charts

- Sensor acquisition interval: 10 seconds
- One-hour chart: 60 one-minute averages
- Eight-hour chart: 60 eight-minute averages
- 24-hour chart: 60 24-minute averages
- Vertical axis: automatically padded and rescaled
- New readings enter from the right side

The graph labels are `1 HOUR`, `8 HOUR`, and `24 HOUR`. It takes the full
selected period to fill each chart. History starts again after every restart
because persistence has not yet been enabled.

## Sensor error screen

If `BME280 not found` appears, verify:

- `SDA` is connected to `D11`
- `SCL` is connected to `D13`
- sensor power is connected to `3.3V`
- grounds are connected
- the microSD slot is empty

If the humidity section shows `--`, verify the DHT11 data wire is connected
to `A5`, check its power and ground, and confirm that a bare sensor has a data
pull-up resistor.
