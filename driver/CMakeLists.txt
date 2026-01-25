# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

# This file shows how to integrate the IS31FL3235A driver into
# Zephyr's build system. In the actual Zephyr tree, this content
# would be added to drivers/led/CMakeLists.txt

zephyr_library_sources_ifdef(CONFIG_LED_IS31FL3235A is31fl3235a.c)
