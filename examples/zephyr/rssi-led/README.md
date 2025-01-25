# RSSI LED blinky

## Supported boards

The current version of the app hardcodes a specific nRF54L15 UART so only the
Nordic nRF54L15DK (nrf54l15/nrf54l15/cpuapp) is supported.

This application tracks the RSSIs of up to 4 devices and uses it to vary the blink rates of
the 4 LEDs on the Nordic dev boards. The higher the RSSI, the faster the corresponding LED blinks.

## Configuration

In the `main()` function in `main.c`, set the `device_ids[]` array to include the device IDs to monitor.
You can also adjust the maximum and minimum RSSI thresholds in `proximity.h`

## RSSI UART Output
The application outputs all detected Blecon RSSIs on UART0.

This UART operates at 115200bps 8,N,1 and has the following pinout:
- TX: P0.00
- RX: P0.01

Refer to the [nRF54L15 Connector interface](https://docs.nordicsemi.com/bundle/ug_nrf54l15_dk/page/UG/nRF54L15_DK/hw_desription/connector_if.html)
to see where these pins are routed on the DK PCB.

The format of the output is `<DEVICE_ID> <RSSI>`, with a space separating the two fields. `RSSI` is a negative number between -128 to 0.

Sample Output:

```
38540b39-4c13-49e2-8eb1-d994778dd2b2 -37
```

## Connecting to UARTs over USB from a computer

The UARTs on the nRF54L15 are also connected to the USB Debug/Programming interface.

The USB debugger exposes 2 virtual serial ports `/dev/tty.usbmodem<board_id><x>` and
`/dev/tty.usbmodem<board_id><y`. `x` is 1 and `y` is 3 on my board (but maybe not all boards?).

In any case, the lower numbered port is the UART for the RSSI and the higher numbered one
is the debug console.

## Project organization
In a standalone application, the additional configuration in `rssi-led.conf` would be placed in
the `prj.conf` for the application. Similarly the `rssi-led.overlay` would normally go into a
board-specific overlay file.

In order to avoid duplicating the configuration that is already present for the examples, we instead use
`set(EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/rssi-led.conf)` line in `CMakeLists.txt` to append these
settings to the  application.
