#include <stdio.h>
#include "proximity.h"
#include "blecon/blecon_util.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/led.h>
#include <zephyr/timing/timing.h>

//RSSI goes from -128 to 127 + null terminator
#define RSSI_STR_SZ (4 + 1)

struct proximity_t {
    const struct device* leds;
    const struct device* uart;
    char device_ids[MAX_DEVICES][BLECON_UUID_STR_SZ];
    int8_t device_rssi[MAX_DEVICES];
    size_t num_devices;
    size_t num_leds;
};

static struct proximity_t proximity;

static void scan_report_iterator(const struct blecon_modem_peer_scan_report_t* report, void* user_data);
static void clear_rssi();

void proximity_init(const struct device* leds, const struct device* uart, size_t num_leds) {
    proximity.leds = leds;
    proximity.uart = uart;
    proximity.num_devices = 0;
    proximity.num_leds = num_leds;

    clear_rssi();
}

void proximity_on_scan_complete(struct blecon_t* blecon) {
    // Build RSSI report
    static char tx_buf[RSSI_STR_SZ * MAX_DEVICES + 3];  // RSSIs + CRLF + terminator
    char rssi_num_buf[RSSI_STR_SZ];

    tx_buf[0] = '\0';

    for(size_t i=0; i<proximity.num_devices; i++) {
        if(i > 0) {
            strcat(tx_buf, ",");
        }
        snprintf(rssi_num_buf, RSSI_STR_SZ, "%d", proximity.device_rssi[i]);
        strcat(tx_buf, rssi_num_buf);
    }
    strcat(tx_buf, "\r\n");

    // Send it out the UART
    size_t len = strlen(tx_buf);
    if(len > 0) {
        for(size_t i=0; i<len; i++) {
            uart_poll_out(proximity.uart, tx_buf[i]);
        }
    }

    // Update debug LEDs
    uint8_t count = 0;
    for(int i=0; i<proximity.num_devices; i++) {
        if(proximity.device_rssi[i] < 0) {
            count += 1;
        }
    }
    for(int i=0; i<proximity.num_leds; i++) {
        if ((count >> i) & 0x01) {
            led_on(proximity.leds, i);
        }
        else {
            led_off(proximity.leds, i);
        }
    }

    // Kick off another scan
    clear_rssi();
    if(!blecon_scan_start(blecon, true, false, PROXIMITY_SCAN_TIME)) {
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
                proximity.device_rssi[i] = report->rssi;
            }
        }
    }
}

bool proximity_add_device(char* device_id) {
    size_t index = proximity.num_devices;

    if (strlen(device_id) != BLECON_UUID_STR_SZ - 1) {
        return false;
    }

    if (index + 1 >= MAX_DEVICES) {
        return false;
    }

    strcpy(proximity.device_ids[index], device_id);
    proximity.num_devices = index + 1;

    return true;
}

void clear_rssi() {
    for(size_t i=0; i<proximity.num_devices; i++) {
        proximity.device_rssi[i] = 0;
    }
}
