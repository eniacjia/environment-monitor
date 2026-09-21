# Troubleshooting

## LCD is blank after connecting the sensor

Do not connect the sensor to the Uno's normal `A4/A5` I2C pins. This TFT shield
uses `A4` as its reset line, so hardware-I2C traffic can reset or disrupt the
display. Use `D11` for SDA and `D13` for SCL with this firmware.

## BME280 not found

1. Remove any microSD card.
2. Check `SDA -> D11` and `SCL -> D13`.
3. Check `3.3V` and `GND`.
4. Confirm the chip is a BME280. The firmware expects chip ID `0x60`.
5. Check whether the breakout requires `CS` pulled high for I2C operation.
6. Restart the Uno after correcting the wiring.

## Touch does not respond accurately

The installed calibration is specific to the development shield. Run the
`MCUFRIEND_kbv/TouchScreen_Calibr_native` example, touch every target, and copy
the resulting pin and calibration constants into the sketch.

## Humidity shows `--%`

- Connect DHT11 data to `A5`, not to the BME280 software-I2C pins.
- Verify DHT11 power and common ground.
- A bare four-pin sensor needs a 4.7-10 kOhm pull-up from data to VCC.
- DHT11 sensors should not be read more frequently than once per second; this
  firmware reads every ten seconds.

## Upload reports “programmer is not responding”

- Close Serial Monitor and any application using the COM port.
- Disconnect the sensor temporarily and retry.
- Verify the selected board is Arduino Uno and select the detected COM port.
- Press Reset immediately before retrying.
- Try another known-good USB data cable.

## Display is too bright

This shield normally powers its backlight directly. Software dimming is not
available without modifying the backlight circuit. Use a transistor or MOSFET
controlled by PWM; never power the backlight directly from an Uno GPIO pin.

## Chart history disappears

This is expected in the current release. History is stored in SRAM and resets
after power loss or reset. EEPROM persistence is documented as a future option
in `FIRMWARE.md`.
