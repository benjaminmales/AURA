# Copyright (c) Blecon Ltd
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

include(../common/config.cmake)
include(../common/flash_debug.cmake)

set(EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/rssi-report.conf)
set(EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/rssi-report.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(rssi-report)
target_sources(app PRIVATE main.c proximity.c)
