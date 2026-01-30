/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IS31FL3235A LED driver sample application
 *
 * This sample demonstrates basic usage of the IS31FL3235A LED driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/is31fl3235a.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

/* Get LED device from device tree */
#define LED_DEV DT_NODELABEL(led_controller)

/* Number of channels to test */
#define NUM_TEST_CHANNELS 3

void main(void)
{
	const struct device *led_dev = DEVICE_DT_GET(LED_DEV);
	int ret;

	LOG_INF("IS31FL3235A LED Driver Sample");

	/* Check device ready */
	if (!device_is_ready(led_dev)) {
		LOG_ERR("LED device %s not ready", led_dev->name);
		return;
	}

	LOG_INF("LED device %s ready", led_dev->name);

	/* Test 1: Basic brightness control using standard LED API (0-100 range) */
	LOG_INF("Test 1: Setting individual channel brightness");
	for (int i = 0; i < NUM_TEST_CHANNELS; i++) {
		ret = led_set_brightness(led_dev, i, 50);  /* 50% brightness */
		if (ret < 0) {
			LOG_ERR("Failed to set channel %d brightness: %d", i, ret);
		}
		k_msleep(500);
	}

	/* Turn off all test channels */
	for (int i = 0; i < NUM_TEST_CHANNELS; i++) {
		led_set_brightness(led_dev, i, 0);
	}

	k_msleep(1000);

	/* Test 2: Multi-channel synchronized update using standard LED API (0-100 range) */
	LOG_INF("Test 2: Synchronized multi-channel update (RGB)");
	uint8_t rgb_values[3];

	/* Red - using standard API with 0-100 percentage */
	rgb_values[0] = 100;
	rgb_values[1] = 0;
	rgb_values[2] = 0;
	led_write_channels(led_dev, 0, 3, rgb_values);
	k_msleep(500);

	/* Green */
	rgb_values[0] = 0;
	rgb_values[1] = 100;
	rgb_values[2] = 0;
	led_write_channels(led_dev, 0, 3, rgb_values);
	k_msleep(500);

	/* Blue */
	rgb_values[0] = 0;
	rgb_values[1] = 0;
	rgb_values[2] = 100;
	led_write_channels(led_dev, 0, 3, rgb_values);
	k_msleep(500);

	/* White */
	rgb_values[0] = 100;
	rgb_values[1] = 100;
	rgb_values[2] = 100;
	led_write_channels(led_dev, 0, 3, rgb_values);
	k_msleep(500);

	/* Off */
	rgb_values[0] = 0;
	rgb_values[1] = 0;
	rgb_values[2] = 0;
	led_write_channels(led_dev, 0, 3, rgb_values);

	k_msleep(1000);

	/* Test 3: Current scaling - using extended API for full brightness */
	LOG_INF("Test 3: Testing current scaling");

	/* Set all channels to max brightness using extended API (0-255) */
	for (int i = 0; i < NUM_TEST_CHANNELS; i++) {
		is31fl3235a_set_brightness(led_dev, i, 255);
	}

	/* Test different scaling factors */
	enum is31fl3235a_current_scale scales[] = {
		IS31FL3235A_SCALE_1X,
		IS31FL3235A_SCALE_1_2X,
		IS31FL3235A_SCALE_1_3X,
		IS31FL3235A_SCALE_1_4X,
	};

	const char *scale_names[] = {"1x", "1/2x", "1/3x", "1/4x"};

	for (int s = 0; s < ARRAY_SIZE(scales); s++) {
		LOG_INF("Setting current scale to %s", scale_names[s]);
		for (int i = 0; i < NUM_TEST_CHANNELS; i++) {
			ret = is31fl3235a_set_current_scale(led_dev, i, scales[s]);
			if (ret < 0) {
				LOG_ERR("Failed to set current scale: %d", ret);
			}
		}
		k_msleep(1000);
	}

	/* Reset to 1x scaling */
	for (int i = 0; i < NUM_TEST_CHANNELS; i++) {
		is31fl3235a_set_current_scale(led_dev, i, IS31FL3235A_SCALE_1X);
		is31fl3235a_set_brightness(led_dev, i, 0);
	}

	k_msleep(1000);

	/* Test 4: Breathing effect - using extended API for smooth 8-bit animation */
	LOG_INF("Test 4: Breathing effect on channel 0");

	for (int cycle = 0; cycle < 3; cycle++) {
		/* Fade in using extended API for smooth 8-bit transitions */
		for (int brightness = 0; brightness <= 255; brightness += 5) {
			is31fl3235a_set_brightness(led_dev, 0, brightness);
			k_msleep(10);
		}

		/* Fade out */
		for (int brightness = 255; brightness >= 0; brightness -= 5) {
			is31fl3235a_set_brightness(led_dev, 0, brightness);
			k_msleep(10);
		}
	}

	k_msleep(1000);

	/* Test 5: Channel enable/disable */
	LOG_INF("Test 5: Testing channel enable/disable");

	is31fl3235a_set_brightness(led_dev, 0, 255);

	LOG_INF("Disabling channel 0 (LED should turn off)");
	ret = is31fl3235a_channel_enable(led_dev, 0, false);
	if (ret < 0) {
		LOG_ERR("Failed to disable channel: %d", ret);
	}
	k_msleep(1000);

	LOG_INF("Enabling channel 0 (LED should turn on)");
	ret = is31fl3235a_channel_enable(led_dev, 0, true);
	if (ret < 0) {
		LOG_ERR("Failed to enable channel: %d", ret);
	}
	k_msleep(1000);

	is31fl3235a_set_brightness(led_dev, 0, 0);

	/* Test 6: Software shutdown */
	LOG_INF("Test 6: Testing software shutdown");

	/* Turn on all test channels using extended API (0-255) */
	for (int i = 0; i < NUM_TEST_CHANNELS; i++) {
		is31fl3235a_set_brightness(led_dev, i, 255);
	}

	k_msleep(1000);

	LOG_INF("Entering software shutdown");
	ret = is31fl3235a_sw_shutdown(led_dev, true);
	if (ret < 0) {
		LOG_ERR("Failed to enter shutdown: %d", ret);
	}
	k_msleep(2000);

	LOG_INF("Waking from software shutdown");
	ret = is31fl3235a_sw_shutdown(led_dev, false);
	if (ret < 0) {
		LOG_ERR("Failed to wake from shutdown: %d", ret);
	}
	k_msleep(1000);

	/* Turn off all LEDs using extended API */
	for (int i = 0; i < NUM_TEST_CHANNELS; i++) {
		is31fl3235a_set_brightness(led_dev, i, 0);
	}

	LOG_INF("Sample complete");
}
