# Copyright (c) Blecon Ltd
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

include(../common/config.cmake)
include(../common/flash_debug.cmake)

set(EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/rssi-led.conf)
set(EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/rssi-led.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(rssi-led)
target_sources(app PRIVATE main.c proximity.c)
