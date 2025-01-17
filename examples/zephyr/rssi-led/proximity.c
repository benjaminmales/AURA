#include "proximity.h"
#include "blecon/blecon_util.h"
#include "zephyr/drivers/led.h"
#include "zephyr/timing/timing.h"

struct proximity_t {
    const struct device* leds;
    char device_ids[MAX_DEVICES][BLECON_UUID_STR_SZ];
    bool device_seen[MAX_DEVICES];
    size_t num_devices;
    size_t num_leds;

    bool led_state[MAX_DEVICES];
    uint16_t led_period[MAX_DEVICES];
    uint16_t led_counter[MAX_DEVICES];
};

static struct proximity_t proximity;

static void scan_report_iterator(const struct blecon_modem_peer_scan_report_t* report, void* user_data);
static void clear_seen_devices();

static uint16_t rssi_to_period(int8_t rssi);

// LED blink timer
// Every 10ms we check whether an LED's state needs to change
// The blink period is specified as multiples of these 10ms intervals.
// A period of 0 turns the LED off.

// It would be more convenient to just use Zephyr's
// led_blink() function with PWM outputs but not all
// of the NRF54L15DK's LEDs are PWM-able

static void update_led(struct k_timer *timer_id);
K_TIMER_DEFINE(blink_timer, update_led, NULL);

void proximity_init(const struct device* leds, size_t num_leds) {
    proximity.leds = leds;
    proximity.num_devices = 0;
    proximity.num_leds = MIN(num_leds, MAX_DEVICES);

    for(int i = 0; i < num_leds; i++) {
        proximity.led_period[i] = 0;
        proximity.led_counter[i] = 0;
    }

    k_timer_start(&blink_timer, K_MSEC(10), K_MSEC(10));
}

void proximity_on_scan_complete(struct blecon_t* blecon) {
    for(int i=0; i<proximity.num_devices; i++) {
        if(!proximity.device_seen[i]) {
            proximity.led_period[i] = 0;
        }
    }
    clear_seen_devices();

    // Kick off another scan
    if(!blecon_scan_start(blecon, true, false, 5000)) {
        printk("Failed to scan for nearby devices\r\n");
        return;
    }
}

void proximity_on_scan_report(struct blecon_t* blecon) {
    // Stream results
    bool overflow = false;

    blecon_scan_get_data(blecon,
        scan_report_iterator,
        NULL,
        &overflow, NULL);
}

void scan_report_iterator(const struct blecon_modem_peer_scan_report_t* report, void* user_data) {
    // Get UUID as string
    char uuid_str[BLECON_UUID_STR_SZ] = {0};
    blecon_util_append_uuid_string(report->blecon_id, uuid_str);

    for(int i=0; i<proximity.num_devices; i++) {
        if(strcmp(uuid_str, proximity.device_ids[i]) == 0) {
            if(report->rssi > RSSI_MIN_THRESHOLD && report->rssi < 0) {
                proximity.device_seen[i] = true;
                proximity.led_period[i] = rssi_to_period(report->rssi);
            }
        }
    }
}

static uint16_t rssi_to_period(int8_t rssi) {

    // clamp to the maximum expected RSSI
    if(rssi > RSSI_MAX_THRESHOLD) {
        rssi = RSSI_MAX_THRESHOLD;
    }

    uint16_t rssi_range = (RSSI_MIN_THRESHOLD - RSSI_MAX_THRESHOLD) * -1;
    uint16_t shifted_rssi = (rssi - RSSI_MAX_THRESHOLD) * -1;

    // Map to a range of 5 - 100 (50ms - 1000ms) blink period
    return ((shifted_rssi * 95 / rssi_range) + 5);
}

bool proximity_add_device(char* device_id) {
    size_t index = proximity.num_devices;

    if (strlen(device_id) != BLECON_UUID_STR_SZ - 1) {
        return false;
    }

    if (index + 1 >= MIN(MAX_DEVICES, proximity.num_leds)) {
        return false;
    }

    strcpy(proximity.device_ids[index], device_id);
    proximity.num_devices = index + 1;

    return true;
}

void clear_seen_devices() {
    for(size_t i=0; i<proximity.num_devices; i++) {
        proximity.device_seen[i] = false;
    }
}

void update_led(struct k_timer *timer_id) {
    for(int i=0; i<proximity.num_devices; i++) {
        proximity.led_counter[i] += 1;

        if (proximity.led_counter[i] >= proximity.led_period[i] &&
            proximity.led_period[i] != 0) {

            proximity.led_state[i] = !(proximity.led_state[i]);
            proximity.led_counter[i] = 0;

            if (proximity.led_state[i]) {
                led_on(proximity.leds, i);
            }
            else {
                led_off(proximity.leds, i);
            }
        }
        else if(proximity.led_period[i] == 0) {
            led_off(proximity.leds, i);
        }
    }
}
