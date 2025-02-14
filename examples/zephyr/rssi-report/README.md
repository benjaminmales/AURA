# RSSI Reporter

## Supported boards

The current version of the app hardcodes a specific nRF54L15 UART so only the
Nordic nRF54L15DK (nrf54l15/nrf54l15/cpuapp) is supported.

This application tracks the RSSIs of up to 15 devices and outputs their RSSIs out the UART.

## Configuration

In the `main()` function in `main.c`, set the `device_ids[]` array to include the device IDs to monitor.
You can also adjust the maximum and minimum RSSI thresholds in `proximity.h`

## LED debug indicator
The 4 LEDs on the dev board output the number of detected devices in binary, where LED0 is LSB and
LED3 is MSB.

Example: LED0: on, LED1: off, LED2: on, LED3: off = 5 devices detected

## RSSI UART Output
The application outputs the RSSIs of monitored devices on UART0

This UART operates at 115200bps 8,N,1 and has the following pinout:
- TX: P0.00
- RX: P0.01

Refer to the [nRF54L15 Connector interface](https://docs.nordicsemi.com/bundle/ug_nrf54l15_dk/page/UG/nRF54L15_DK/hw_desription/connector_if.html)
to see where these pins are routed on the DK PCB.

The format of the output is `RSSI_0,RSSI_1, ... RSSI_N `, with a comma between RSSIs. Where `N` is the number of devices monitored.
`RSSI_N` is a negative number between -128 to -1. An RSSI of 0 means the device was not detected in the last scan cycle.

Sample Output (2 Devices):

```
-51,-47
-52,-47
-52,-47
-51,-46
-50,-46
-53,-50
-49,-50
-55,-49
-49,-48
```

## Connecting to UARTs over USB from a computer

The UARTs on the nRF54L15 are also connected to the USB Debug/Programming interface.

The USB debugger exposes 2 virtual serial ports `/dev/tty.usbmodem<board_id><x>` and
`/dev/tty.usbmodem<board_id><y`. `x` is 1 and `y` is 3 on my board (but maybe not all boards?).

In any case, the lower numbered port is the UART for the RSSI and the higher numbered one
is the debug console.
