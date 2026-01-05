/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3235A_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3235A_H_

/**
 * @file
 * @brief Public API for IS31FL3235A LED driver
 *
 * This header defines the extended API for the IS31FL3235A LED driver.
 * The driver also implements the standard Zephyr LED API (see led.h).
 *
 * @defgroup is31fl3235a_interface IS31FL3235A LED Driver
 * @ingroup led_interface
 * @{
 */

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Current scaling factors
 *
 * The IS31FL3235A supports per-channel current scaling to reduce
 * the maximum output current from the hardware-configured IMAX value.
 */
enum is31fl3235a_current_scale {
	/** Full scale: 100% of IMAX */
	IS31FL3235A_SCALE_1X = 0,
	/** Half scale: 50% of IMAX */
	IS31FL3235A_SCALE_1_2X = 1,
	/** Third scale: 33% of IMAX */
	IS31FL3235A_SCALE_1_3X = 2,
	/** Quarter scale: 25% of IMAX */
	IS31FL3235A_SCALE_1_4X = 3,
};

/**
 * @brief Set current scaling for a channel
 *
 * The maximum output current (IMAX) is set by hardware via an external
 * resistor. This function allows software to scale the current down by
 * a factor of 1, 1/2, 1/3, or 1/4.
 *
 * This is useful for:
 * - Balancing brightness across different LED types
 * - Reducing power consumption
 * - Matching LED forward voltages
 *
 * @param dev Pointer to the device structure
 * @param channel Channel number (0-27)
 * @param scale Current scaling factor
 *
 * @retval 0 On success
 * @retval -EINVAL Invalid channel or scale value
 * @retval -EIO I2C communication error
 */
int is31fl3235a_set_current_scale(const struct device *dev,
				   uint8_t channel,
				   enum is31fl3235a_current_scale scale);

/**
 * @brief Enable or disable a channel
 *
 * Disabled channels produce no output regardless of PWM setting.
 * This is more efficient than setting brightness to 0 if the channel
 * will be disabled for an extended period.
 *
 * @param dev Pointer to the device structure
 * @param channel Channel number (0-27)
 * @param enable true to enable, false to disable
 *
 * @retval 0 On success
 * @retval -EINVAL Invalid channel number
 * @retval -EIO I2C communication error
 */
int is31fl3235a_channel_enable(const struct device *dev,
				uint8_t channel,
				bool enable);

/**
 * @brief Software shutdown control
 *
 * In software shutdown mode, all LED outputs are turned off but
 * register values are retained. This reduces power consumption
 * significantly while maintaining the ability to quickly resume.
 *
 * I2C communication continues to work in software shutdown mode.
 *
 * @param dev Pointer to the device structure
 * @param shutdown true to enter shutdown, false to wake
 *
 * @retval 0 On success
 * @retval -EIO I2C communication error
 */
int is31fl3235a_sw_shutdown(const struct device *dev, bool shutdown);

/**
 * @brief Hardware shutdown control via SDB pin
 *
 * Controls the SDB (shutdown) GPIO pin if configured in device tree.
 * When SDB is low, the chip enters hardware shutdown mode, which
 * provides the lowest power consumption.
 *
 * Hardware shutdown consumes less power than software shutdown but
 * requires a GPIO pin connection.
 *
 * @param dev Pointer to the device structure
 * @param shutdown true to shutdown (SDB low), false to wake (SDB high)
 *
 * @retval 0 On success
 * @retval -ENOTSUP SDB pin not configured in device tree
 * @retval -EIO GPIO control error
 */
int is31fl3235a_hw_shutdown(const struct device *dev, bool shutdown);

/**
 * @brief Manually trigger update of buffered register values
 *
 * The IS31FL3235A uses double-buffered registers for PWM and LED
 * control values. Changes are staged in temporary registers and
 * applied simultaneously when the update register is written.
 *
 * This function is for advanced use cases only. The standard LED API
 * functions (led_set_brightness, led_write_channels) automatically
 * trigger updates, so most applications will not need this function.
 *
 * Use this when you need fine control over update timing or are
 * bypassing the standard API.
 *
 * @param dev Pointer to the device structure
 *
 * @retval 0 On success
 * @retval -EIO I2C communication error
 */
int is31fl3235a_update(const struct device *dev);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3235A_H_ */
