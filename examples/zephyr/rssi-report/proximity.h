#pragma once

#include <zephyr/kernel.h>
#include <blecon/blecon.h>

#define MAX_DEVICES 15
#define RSSI_MIN_THRESHOLD  -80
#define RSSI_MAX_THRESHOLD  -30
#define PROXIMITY_SCAN_TIME 2000

/** Initialise LEDs to be used for tracking RSSI
 *  Arguments:
 *  leds: Zephyr LED device (e.g., gpio-leds)
 *  num_leds: number of LEDs the LED device contains
 */
void proximity_init(const struct device* leds, const struct device* uart, size_t num_leds);

/** Register a device ID to monitor the RSSI
 *  Arguments:
 *  device_id: Device ID string (including dashes)
 *
 *  Returns:
 *  false if no more available LEDs can be used to monitor
 *  or if device_id is malformed
 * */
bool proximity_add_device(char* device_id);

/** Blecon scan report callback handler */
void proximity_on_scan_report(struct blecon_t* blecon);

/** Blecon scan complete callback handler */
void proximity_on_scan_complete(struct blecon_t* blecon);
