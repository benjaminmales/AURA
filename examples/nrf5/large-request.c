/*
 * Copyright (c) Blecon Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_scheduler.h"
#include "app_error.h"
#include "bsp.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "blecon/blecon.h"
#include "blecon/blecon_error.h"
#include "blecon_app.h"

#define CONCURRENT_SEND_OPS_COUNT 2
#ifndef LARGE_REQUEST_EXAMPLE_BUFFER_SZ
#define LARGE_REQUEST_EXAMPLE_BUFFER_SZ 32768
#endif

struct example_send_op_t {
    struct blecon_request_send_data_op_t op;
    bool busy;
};

static struct blecon_t _blecon = {0};
static struct blecon_request_t _request = {0};
static struct example_send_op_t _send_ops[CONCURRENT_SEND_OPS_COUNT] = {0};
static struct blecon_request_receive_data_op_t _receive_op = {0};
static uint8_t _outgoing_data_buffer[LARGE_REQUEST_EXAMPLE_BUFFER_SZ] = {0}; // Large buffer
static size_t _outgoing_data_buffer_pos = 0;
static uint8_t _incoming_data_buffer[64] = {0};

// Blecon callbacks
static void example_on_connection(struct blecon_t* blecon);
static void example_on_disconnection(struct blecon_t* blecon);
static void example_on_time_update(struct blecon_t* blecon);
static void example_on_ping_result(struct blecon_t* blecon);

const static struct blecon_callbacks_t blecon_callbacks = {
    .on_connection = example_on_connection,
    .on_disconnection = example_on_disconnection,
    .on_time_update = example_on_time_update,
    .on_ping_result = example_on_ping_result
};

// Requests callbacks
static void example_request_on_closed(struct blecon_request_t* request);
static void example_request_on_data_sent(struct blecon_request_send_data_op_t* send_data_op, bool data_sent);
static uint8_t* example_request_alloc_incoming_data_buffer(struct blecon_request_receive_data_op_t* receive_data_op, size_t sz);
static void example_request_on_data_received(struct blecon_request_receive_data_op_t* receive_data_op, bool data_received, const uint8_t* data, size_t sz, bool finished);

const static struct blecon_request_callbacks_t blecon_request_callbacks = {
    .on_closed = example_request_on_closed,
    .on_data_sent = example_request_on_data_sent,
    .alloc_incoming_data_buffer = example_request_alloc_incoming_data_buffer,
    .on_data_received = example_request_on_data_received
};

// Internal functions
static void example_send_data(void);

// Utility macro
static inline void blecon_check_error(enum blecon_ret_t code) { 
    if(code != blecon_ok) {
        NRF_LOG_ERROR("Blecon modem error: %x\r\n", code);
        blecon_fatal_error();
    }
}

static void request_from_irq(void * p_event_data, uint16_t event_size) {
    NRF_LOG_INFO("Requesting.");
        
    // Request Blecon Connection
    blecon_connection_initiate(&_blecon);

    bsp_board_leds_off();
    bsp_board_led_on(0);
}

static void announce_device(void * p_event_data, uint16_t event_size) {
    NRF_LOG_INFO("Announcing device ID.");

    // Announce device ID
    blecon_announce(&_blecon);
}

// Handle BSP events
void bsp_event_handler(bsp_event_t event)
{
    switch (event)
    {
        case BSP_EVENT_KEY_0:
            // This is called in IRQ context, so put in scheduler's queue
            app_sched_event_put(NULL, 0, request_from_irq);
            break;

        case BSP_EVENT_KEY_1:
            break;

        case BSP_EVENT_KEY_2:
            app_sched_event_put(NULL, 0, announce_device);
            break;

        default:
            break;
    }
}

// Initialise buttons and LEDs
static void buttons_leds_init(void)
{
    ret_code_t err_code;

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);
}

void example_on_connection(struct blecon_t* blecon) {
    bsp_board_leds_off();
    bsp_board_led_on(1);

    NRF_LOG_INFO("Connected, sending request.");

    // Clean-up request
    blecon_request_cleanup(&_request);

    // Reset outgoing buffer position
    _outgoing_data_buffer_pos = 0;

    // Reset send operations
    for(size_t p = 0; p < CONCURRENT_SEND_OPS_COUNT; p++) {
        _send_ops[p].busy = false;
    }
    
    // Queue initial send operations
    example_send_data();

    // Create receive data operation
    if(!blecon_request_receive_data(&_receive_op, &_request, NULL)) {
        printf("Failed to receive data\r\n");
        blecon_request_cleanup(&_request);
        return;
    }

    // Submit request
    blecon_submit_request(&_blecon, &_request);
}

void example_on_disconnection(struct blecon_t* blecon) {
    printf("Disconnected\r\n");
}

void example_on_time_update(struct blecon_t* blecon) {
    printf("Time update\r\n");
}

void example_on_ping_result(struct blecon_t* blecon) {}

void example_request_on_data_received(struct blecon_request_receive_data_op_t* receive_data_op, bool data_received, const uint8_t* data, size_t sz, bool finished) {
    bsp_board_leds_off();
    bsp_board_led_on(2);

    // Retrieve response
    if(!data_received) {
        printf("Failed to receive data\r\n");
        return;
    }
    printf("Data received\r\n");

    if(finished) {
        printf("All received\r\n");
    }
}

void example_request_on_data_sent(struct blecon_request_send_data_op_t* send_data_op, bool data_sent) {
    // Mark send operation as not busy
    struct example_send_op_t* op = (struct example_send_op_t*)blecon_request_send_data_op_get_user_data(send_data_op);
    op->busy = false;

    if(!data_sent) {
        printf("Failed to send data\r\n");
        return;
    }
    printf("Data sent\r\n");

    // Send more chunks if needed
    example_send_data();
}

uint8_t* example_request_alloc_incoming_data_buffer(struct blecon_request_receive_data_op_t* receive_data_op, size_t sz) {
    return _incoming_data_buffer;
}

void example_request_on_closed(struct blecon_request_t* request) {
    enum blecon_request_status_code_t status_code = blecon_request_get_status(request);

    if(status_code != blecon_request_status_ok) {
        bsp_board_leds_off();
        bsp_board_led_on(3);
        printf("Request failed with status code: %d\r\n", status_code);
    } else {
        printf("Request successful\r\n");
    }

    // Terminate connection
    blecon_connection_terminate(&_blecon);
}

void example_send_data(void) {
    for(size_t p = 0; p < CONCURRENT_SEND_OPS_COUNT; p++) {
        struct example_send_op_t* op = &_send_ops[p];
        if(op->busy) {
            continue;
        }
        op->busy = true;

        if(_outgoing_data_buffer_pos >= sizeof(_outgoing_data_buffer)) {
            // Already finished
            break;
        }

        size_t chunk_sz = 4096;
        if(_outgoing_data_buffer_pos + chunk_sz > sizeof(_outgoing_data_buffer)) {
            chunk_sz = sizeof(_outgoing_data_buffer) - _outgoing_data_buffer_pos;
        }

        bool finished = false;
        if(_outgoing_data_buffer_pos + chunk_sz >= sizeof(_outgoing_data_buffer)) {
            finished = true;
        }

        printf("Sending chunk of %u bytes\r\n", chunk_sz);

        // Create send data operation
        if(!blecon_request_send_data(&op->op, &_request, _outgoing_data_buffer + _outgoing_data_buffer_pos, chunk_sz, finished, op)) {
            printf("Failed to send data\r\n");
            blecon_request_cleanup(&_request);
            return;
        }

        _outgoing_data_buffer_pos += chunk_sz;

        if(finished) {
            printf("All sent\r\n");
            break;
        }
    }
}

void blecon_app_init(void) {
    // Initialise buttons and LEDs
    buttons_leds_init();
}

void blecon_app_start(struct blecon_modem_t* blecon_modem) {
    // Blecon
    blecon_init(&_blecon, blecon_modem);
    blecon_set_callbacks(&_blecon, &blecon_callbacks, NULL);
    if(!blecon_setup(&_blecon)) {
        printf("Failed to setup blecon\r\n");
        return;
    }

    // Populate buffer with pattern
    for(size_t p = 0; p < sizeof(_outgoing_data_buffer); p++) {
        _outgoing_data_buffer[p] = p & 0xffu;
    }

    // Init request
    const static struct blecon_request_parameters_t request_params = {
        .namespace = "mynamespace",
        .method = "upload",
        .oneway = true,
        .request_content_type = NULL,
        .response_content_type = NULL,
        .response_mtu = sizeof(_incoming_data_buffer),
        .callbacks = &blecon_request_callbacks,
        .user_data = NULL
    };
    blecon_request_init(&_request, &request_params);

    // Print device URL
    char blecon_url[BLECON_URL_SZ] = {0};
    if(!blecon_get_url(&_blecon, blecon_url, sizeof(blecon_url))) {
        printf("Failed to get device URL\r\n");
        return;
    }
    NRF_LOG_INFO("Device URL: %s", NRF_LOG_PUSH(blecon_url));

    NRF_LOG_INFO("Press button 1 to make a request");
    NRF_LOG_INFO("Press button 3 to identify the device");
}