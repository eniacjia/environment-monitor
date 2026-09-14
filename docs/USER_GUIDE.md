# User guide

## Startup

After power-on or reset, the display shows the startup page for ten seconds:

- Environmental Monitor
- Local climate at a glance
- Build: Sep 11 2026
- Built by DJIA using Codex

The monitor then detects the BME280 at address `0x76` or `0x77` and opens the
dashboard.

## Display sections

The upper section shows **TEMPERATURE** in yellow. The lower section shows
**PRESSURE** in cyan. Measurements update every ten seconds.

Tap a section to cycle it independently through four modes:

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
