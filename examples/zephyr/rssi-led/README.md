# RSSI LED blinky

## Supported boards

- Nordic nRF52840DK (nrf52840dk/nrf52840)
- Nordic nRF54L15DK (nrf54l15/nrf54l15/cpuapp)

This application tracks the RSSIs of up to 4 devices and uses it to vary the blink rates of
the 4 LEDs on the Nordic dev boards. The higher the RSSI, the faster the corresponding LED blinks.

## Configuration
In the `main()` function in `main.c`, set the `device_ids[]` array to include the device IDs to monitor.
You can also adjust the maximum and minimum RSSI thresholds in `proximity.h`

## Project organization
In a standalone application, the additional configuration in `rssi-led.conf` would be placed in the `prj.conf` for the application.

In order to avoid duplicating the configuration that is already present for the examples, we instead use
`set(EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/rssi-led.conf)` line in `CMakeLists.txt` to append these
settings to the  application.
