/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT issi_is31fl3235a

/**
 * @file
 * @brief IS31FL3235A LED driver
 *
 * Driver for the ISSI/Lumissil IS31FL3235A 28-channel LED driver
 * with I2C interface.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/is31fl3235a.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "is31fl3235a_regs.h"

LOG_MODULE_REGISTER(is31fl3235a, CONFIG_LED_LOG_LEVEL);

/**
 * @brief IS31FL3235A device configuration (read-only, in ROM)
 */
struct is31fl3235a_cfg {
	/** I2C bus specification */
	struct i2c_dt_spec i2c;
	/** Optional SDB (shutdown) GPIO pin */
	struct gpio_dt_spec sdb_gpio;
	/** PWM frequency: false=3kHz, true=22kHz */
	bool pwm_freq_22khz;
};

/**
 * @brief IS31FL3235A runtime data (read-write, in RAM)
 */
struct is31fl3235a_data {
	/** Mutex for thread-safe access */
	struct k_mutex lock;
	/** Initialization complete flag */
	bool initialized;
	/** Software shutdown state */
	bool sw_shutdown;
	/** Hardware shutdown state (if SDB pin configured) */
	bool hw_shutdown;
	/** Cached PWM values for all 28 channels */
	uint8_t pwm_cache[IS31FL3235A_NUM_CHANNELS];
	/** Cached LED control register values for all 28 channels */
	uint8_t ctrl_cache[IS31FL3235A_NUM_CHANNELS];
};

/**
 * @brief Write a single byte to a register
 *
 * @param dev Pointer to device structure
 * @param reg Register address
 * @param value Value to write
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct is31fl3235a_cfg *cfg = dev->config;
	uint8_t buf[2] = {reg, value};
	int ret;

	ret = i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to write register 0x%02x: %d", reg, ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Write multiple bytes starting at a register address
 *
 * @param dev Pointer to device structure
 * @param start_reg Starting register address
 * @param buf Buffer containing values to write
 * @param len Number of bytes to write
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_write_buffer(const struct device *dev,
				     uint8_t start_reg,
				     const uint8_t *buf,
				     size_t len)
{
	const struct is31fl3235a_cfg *cfg = dev->config;
	uint8_t write_buf[256];
	int ret;

	if (len > sizeof(write_buf) - 1) {
		return -EINVAL;
	}

	write_buf[0] = start_reg;
	memcpy(&write_buf[1], buf, len);

	ret = i2c_write_dt(&cfg->i2c, write_buf, len + 1);
	if (ret < 0) {
		LOG_ERR("Failed to write %zu bytes at register 0x%02x: %d",
			len, start_reg, ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Read a single byte from a register
 *
 * @param dev Pointer to device structure
 * @param reg Register address
 * @param value Pointer to store read value
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_read_reg(const struct device *dev, uint8_t reg, uint8_t *value)
{
	const struct is31fl3235a_cfg *cfg = dev->config;
	int ret;

	ret = i2c_write_read_dt(&cfg->i2c, &reg, 1, value, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read register 0x%02x: %d", reg, ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Trigger update of buffered PWM and control register values
 *
 * @param dev Pointer to device structure
 * @return 0 on success, negative errno on error
 */
static inline int is31fl3235a_trigger_update(const struct device *dev)
{
	return is31fl3235a_write_reg(dev, IS31FL3235A_REG_UPDATE,
				      IS31FL3235A_UPDATE_TRIGGER);
}

/**
 * @brief Set brightness for a single LED channel (standard LED API)
 *
 * @param dev Pointer to device structure
 * @param led Channel number (0-27)
 * @param value Brightness value (0-255)
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_led_set_brightness(const struct device *dev,
					   uint32_t led,
					   uint8_t value)
{
	struct is31fl3235a_data *data = dev->data;
	int ret;

	if (led >= IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Invalid channel %u (max %u)", led,
			IS31FL3235A_NUM_CHANNELS - 1);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Write PWM value to register */
	ret = is31fl3235a_write_reg(dev, IS31FL3235A_PWM_REG(led), value);
	if (ret < 0) {
		goto unlock;
	}

	/* Update cache */
	data->pwm_cache[led] = value;

	/* Trigger update to apply change */
	ret = is31fl3235a_trigger_update(dev);
	if (ret < 0) {
		goto unlock;
	}

	LOG_DBG("Set channel %u brightness to %u", led, value);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Write brightness values to multiple consecutive channels (standard LED API)
 *
 * @param dev Pointer to device structure
 * @param start_channel First channel number
 * @param num_channels Number of consecutive channels
 * @param buf Buffer containing brightness values
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_led_write_channels(const struct device *dev,
					   uint32_t start_channel,
					   uint32_t num_channels,
					   const uint8_t *buf)
{
	struct is31fl3235a_data *data = dev->data;
	int ret;

	if (start_channel >= IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Invalid start channel %u", start_channel);
		return -EINVAL;
	}

	if (start_channel + num_channels > IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Channel range %u-%u exceeds maximum %u",
			start_channel, start_channel + num_channels - 1,
			IS31FL3235A_NUM_CHANNELS - 1);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Write PWM values to consecutive registers */
	ret = is31fl3235a_write_buffer(dev, IS31FL3235A_PWM_REG(start_channel),
					buf, num_channels);
	if (ret < 0) {
		goto unlock;
	}

	/* Update cache */
	memcpy(&data->pwm_cache[start_channel], buf, num_channels);

	/* Trigger update to apply all changes simultaneously */
	ret = is31fl3235a_trigger_update(dev);
	if (ret < 0) {
		goto unlock;
	}

	LOG_DBG("Set channels %u-%u (%u channels)",
		start_channel, start_channel + num_channels - 1, num_channels);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Turn on LED to maximum brightness (standard LED API)
 *
 * @param dev Pointer to device structure
 * @param led Channel number
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_led_on(const struct device *dev, uint32_t led)
{
	return is31fl3235a_led_set_brightness(dev, led, IS31FL3235A_PWM_MAX);
}

/**
 * @brief Turn off LED (standard LED API)
 *
 * @param dev Pointer to device structure
 * @param led Channel number
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_led_off(const struct device *dev, uint32_t led)
{
	return is31fl3235a_led_set_brightness(dev, led, IS31FL3235A_PWM_MIN);
}

/* Standard LED driver API */
static const struct led_driver_api is31fl3235a_led_api = {
	.set_brightness = is31fl3235a_led_set_brightness,
	.write_channels = is31fl3235a_led_write_channels,
	.on = is31fl3235a_led_on,
	.off = is31fl3235a_led_off,
};

/**
 * @brief Set current scaling for a channel (extended API)
 */
int is31fl3235a_set_current_scale(const struct device *dev,
				   uint8_t channel,
				   enum is31fl3235a_current_scale scale)
{
	struct is31fl3235a_data *data = dev->data;
	uint8_t ctrl_val;
	int ret;

	if (channel >= IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	if (scale > IS31FL3235A_SCALE_1_4X) {
		LOG_ERR("Invalid scale %u", scale);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Read cached control register value */
	ctrl_val = data->ctrl_cache[channel];

	/* Clear scale bits and set new value */
	ctrl_val &= ~IS31FL3235A_CTRL_SL_MASK;
	ctrl_val |= (scale << IS31FL3235A_CTRL_SL_SHIFT);

	/* Write to control register */
	ret = is31fl3235a_write_reg(dev, IS31FL3235A_CTRL_REG(channel), ctrl_val);
	if (ret < 0) {
		goto unlock;
	}

	/* Update cache */
	data->ctrl_cache[channel] = ctrl_val;

	/* Trigger update */
	ret = is31fl3235a_trigger_update(dev);
	if (ret < 0) {
		goto unlock;
	}

	LOG_DBG("Set channel %u current scale to %u", channel, scale);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Enable or disable a channel (extended API)
 */
int is31fl3235a_channel_enable(const struct device *dev,
				uint8_t channel,
				bool enable)
{
	struct is31fl3235a_data *data = dev->data;
	uint8_t ctrl_val;
	int ret;

	if (channel >= IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Read cached control register value */
	ctrl_val = data->ctrl_cache[channel];

	/* Set or clear enable bit */
	if (enable) {
		ctrl_val |= IS31FL3235A_CTRL_OUT_ENABLE;
	} else {
		ctrl_val &= ~IS31FL3235A_CTRL_OUT_ENABLE;
	}

	/* Write to control register */
	ret = is31fl3235a_write_reg(dev, IS31FL3235A_CTRL_REG(channel), ctrl_val);
	if (ret < 0) {
		goto unlock;
	}

	/* Update cache */
	data->ctrl_cache[channel] = ctrl_val;

	/* Trigger update */
	ret = is31fl3235a_trigger_update(dev);
	if (ret < 0) {
		goto unlock;
	}

	LOG_DBG("Channel %u %s", channel, enable ? "enabled" : "disabled");

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Enable or disable multiple consecutive channels (extended API)
 */
int is31fl3235a_channels_enable(const struct device *dev,
				 uint8_t start_channel,
				 uint8_t num_channels,
				 const bool *enable)
{
	struct is31fl3235a_data *data = dev->data;
	uint8_t ctrl_buf[IS31FL3235A_NUM_CHANNELS];
	int ret;

	if (start_channel >= IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Invalid start channel %u", start_channel);
		return -EINVAL;
	}

	if (start_channel + num_channels > IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Channel range %u-%u exceeds maximum %u",
			start_channel, start_channel + num_channels - 1,
			IS31FL3235A_NUM_CHANNELS - 1);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Build control register buffer, preserving current scale settings */
	for (uint8_t i = 0; i < num_channels; i++) {
		uint8_t ctrl_val = data->ctrl_cache[start_channel + i];

		if (enable[i]) {
			ctrl_val |= IS31FL3235A_CTRL_OUT_ENABLE;
		} else {
			ctrl_val &= ~IS31FL3235A_CTRL_OUT_ENABLE;
		}

		ctrl_buf[i] = ctrl_val;
	}

	/* Write control values to consecutive registers */
	ret = is31fl3235a_write_buffer(dev, IS31FL3235A_CTRL_REG(start_channel),
					ctrl_buf, num_channels);
	if (ret < 0) {
		goto unlock;
	}

	/* Update cache */
	memcpy(&data->ctrl_cache[start_channel], ctrl_buf, num_channels);

	/* Trigger update to apply all changes simultaneously */
	ret = is31fl3235a_trigger_update(dev);
	if (ret < 0) {
		goto unlock;
	}

	LOG_DBG("Set channels %u-%u enable states (%u channels)",
		start_channel, start_channel + num_channels - 1, num_channels);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Software shutdown control (extended API)
 */
int is31fl3235a_sw_shutdown(const struct device *dev, bool shutdown)
{
	struct is31fl3235a_data *data = dev->data;
	uint8_t value;
	int ret;

	value = shutdown ? IS31FL3235A_SHUTDOWN_MODE : IS31FL3235A_SHUTDOWN_NORMAL;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = is31fl3235a_write_reg(dev, IS31FL3235A_REG_SHUTDOWN, value);
	if (ret < 0) {
		LOG_ERR("Failed to %s software shutdown: %d",
			shutdown ? "enter" : "exit", ret);
		goto unlock;
	}

	data->sw_shutdown = shutdown;

	LOG_INF("Software shutdown %s", shutdown ? "enabled" : "disabled");

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Hardware shutdown control via SDB pin (extended API)
 */
int is31fl3235a_hw_shutdown(const struct device *dev, bool shutdown)
{
	const struct is31fl3235a_cfg *cfg = dev->config;
	struct is31fl3235a_data *data = dev->data;
	int ret;

	/* Check if SDB pin is configured */
	if (!cfg->sdb_gpio.port) {
		LOG_WRN("SDB pin not configured");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Set GPIO: low=shutdown, high=normal */
	ret = gpio_pin_set_dt(&cfg->sdb_gpio, shutdown ? 0 : 1);
	if (ret < 0) {
		LOG_ERR("Failed to set SDB pin: %d", ret);
		goto unlock;
	}

	data->hw_shutdown = shutdown;

	/* Brief delay for chip to respond */
	if (!shutdown) {
		k_msleep(IS31FL3235A_STARTUP_DELAY_MS);
	}

	LOG_INF("Hardware shutdown %s", shutdown ? "enabled" : "disabled");

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Manually trigger update (extended API)
 */
int is31fl3235a_update(const struct device *dev)
{
	struct is31fl3235a_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = is31fl3235a_trigger_update(dev);
	k_mutex_unlock(&data->lock);

	return ret;
}

/**
 * @brief Set brightness for a single LED channel without triggering update (extended API)
 *
 * @param dev Pointer to device structure
 * @param led Channel number (0-27)
 * @param value Brightness value (0-255)
 * @return 0 on success, negative errno on error
 */
int is31fl3235a_set_brightness_no_update(const struct device *dev,
					  uint32_t led,
					  uint8_t value)
{
	struct is31fl3235a_data *data = dev->data;
	int ret;

	if (led >= IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Invalid channel %u (max %u)", led,
			IS31FL3235A_NUM_CHANNELS - 1);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Write PWM value to register */
	ret = is31fl3235a_write_reg(dev, IS31FL3235A_PWM_REG(led), value);
	if (ret < 0) {
		goto unlock;
	}

	/* Update cache */
	data->pwm_cache[led] = value;

	LOG_DBG("Set channel %u brightness to %u (no update)", led, value);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Write brightness values to multiple consecutive channels without triggering update (extended API)
 *
 * @param dev Pointer to device structure
 * @param start_channel First channel number
 * @param num_channels Number of consecutive channels
 * @param buf Buffer containing brightness values
 * @return 0 on success, negative errno on error
 */
int is31fl3235a_write_channels_no_update(const struct device *dev,
					  uint32_t start_channel,
					  uint32_t num_channels,
					  const uint8_t *buf)
{
	struct is31fl3235a_data *data = dev->data;
	int ret;

	if (start_channel >= IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Invalid start channel %u", start_channel);
		return -EINVAL;
	}

	if (start_channel + num_channels > IS31FL3235A_NUM_CHANNELS) {
		LOG_ERR("Channel range %u-%u exceeds maximum %u",
			start_channel, start_channel + num_channels - 1,
			IS31FL3235A_NUM_CHANNELS - 1);
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Write PWM values to consecutive registers */
	ret = is31fl3235a_write_buffer(dev, IS31FL3235A_PWM_REG(start_channel),
					buf, num_channels);
	if (ret < 0) {
		goto unlock;
	}

	/* Update cache */
	memcpy(&data->pwm_cache[start_channel], buf, num_channels);

	LOG_DBG("Set channels %u-%u (%u channels, no update)",
		start_channel, start_channel + num_channels - 1, num_channels);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Initialize the IS31FL3235A device
 *
 * @param dev Pointer to device structure
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_init(const struct device *dev)
{
	const struct is31fl3235a_cfg *cfg = dev->config;
	struct is31fl3235a_data *data = dev->data;
	uint8_t freq_val;
	int ret;

	LOG_INF("Initializing IS31FL3235A");

	/* Initialize mutex */
	k_mutex_init(&data->lock);

	/* Check I2C bus ready */
	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Configure SDB pin if present */
	if (cfg->sdb_gpio.port) {
		if (!device_is_ready(cfg->sdb_gpio.port)) {
			LOG_ERR("SDB GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->sdb_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure SDB pin: %d", ret);
			return ret;
		}

		/* Enable chip by setting SDB high */
		ret = gpio_pin_set_dt(&cfg->sdb_gpio, 1);
		if (ret < 0) {
			LOG_ERR("Failed to set SDB pin high: %d", ret);
			return ret;
		}

		data->hw_shutdown = false;

		/* Wait for chip to start up */
		k_msleep(IS31FL3235A_STARTUP_DELAY_MS);

		LOG_DBG("SDB pin configured and set high");
	}

	/* Reset chip to known state */
	ret = is31fl3235a_write_reg(dev, IS31FL3235A_REG_RESET,
				     IS31FL3235A_RESET_TRIGGER);
	if (ret < 0) {
		LOG_ERR("Failed to reset chip: %d", ret);
		return ret;
	}

	/* Wait for reset to complete */
	k_msleep(IS31FL3235A_RESET_DELAY_MS);

	LOG_DBG("Chip reset complete");

	/* Wake from software shutdown */
	ret = is31fl3235a_write_reg(dev, IS31FL3235A_REG_SHUTDOWN,
				     IS31FL3235A_SHUTDOWN_NORMAL);
	if (ret < 0) {
		LOG_ERR("Failed to wake from shutdown: %d", ret);
		return ret;
	}

	data->sw_shutdown = false;

	/* Configure PWM frequency */
	freq_val = cfg->pwm_freq_22khz ? IS31FL3235A_FREQ_22KHZ :
					  IS31FL3235A_FREQ_3KHZ;
	ret = is31fl3235a_write_reg(dev, IS31FL3235A_REG_FREQ, freq_val);
	if (ret < 0) {
		LOG_ERR("Failed to set PWM frequency: %d", ret);
		return ret;
	}

	LOG_DBG("PWM frequency set to %s", cfg->pwm_freq_22khz ? "22kHz" : "3kHz");

	/* Initialize all channels: enabled, 1x current, 0 brightness */
	for (int i = 0; i < IS31FL3235A_NUM_CHANNELS; i++) {
		/* Set PWM to 0 */
		ret = is31fl3235a_write_reg(dev, IS31FL3235A_PWM_REG(i), 0);
		if (ret < 0) {
			LOG_ERR("Failed to init PWM register %d: %d", i, ret);
			return ret;
		}
		data->pwm_cache[i] = 0;

		/* Set control: enabled, 1x current */
		uint8_t ctrl = IS31FL3235A_CTRL_ENABLE_1X;
		ret = is31fl3235a_write_reg(dev, IS31FL3235A_CTRL_REG(i), ctrl);
		if (ret < 0) {
			LOG_ERR("Failed to init control register %d: %d", i, ret);
			return ret;
		}
		data->ctrl_cache[i] = ctrl;
	}

	/* Trigger update to apply all initialization settings */
	ret = is31fl3235a_trigger_update(dev);
	if (ret < 0) {
		LOG_ERR("Failed to trigger initial update: %d", ret);
		return ret;
	}

	/* Mark as initialized */
	data->initialized = true;

	LOG_INF("IS31FL3235A initialized successfully (PWM: %s, I2C: 0x%02x)",
		cfg->pwm_freq_22khz ? "22kHz" : "3kHz", cfg->i2c.addr);

	return 0;
}

/* Device instantiation macro */
#define IS31FL3235A_DEFINE(inst)						\
	static struct is31fl3235a_data is31fl3235a_data_##inst;			\
										\
	static const struct is31fl3235a_cfg is31fl3235a_cfg_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),				\
		.sdb_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, sdb_gpios, {0}),	\
		.pwm_freq_22khz = (DT_INST_PROP(inst, pwm_frequency) == 22000),\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst,						\
			      is31fl3235a_init,					\
			      NULL,						\
			      &is31fl3235a_data_##inst,				\
			      &is31fl3235a_cfg_##inst,				\
			      POST_KERNEL,					\
			      CONFIG_LED_INIT_PRIORITY,				\
			      &is31fl3235a_led_api);

/* Instantiate all enabled devices */
DT_INST_FOREACH_STATUS_OKAY(IS31FL3235A_DEFINE)
