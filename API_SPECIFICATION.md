# IS31FL3235A Driver API Specification

## Overview

This document defines the complete API for the IS31FL3235A Zephyr RTOS driver, including both the standard Zephyr LED API and extended IS31FL3235A-specific functions.

## Header Files

```c
/* Standard LED API */
#include <zephyr/drivers/led.h>

/* Extended IS31FL3235A API */
#include <zephyr/drivers/led/is31fl3235a.h>

/* Device access */
#include <zephyr/device.h>
```

## Device Access

### Getting Device Reference

```c
/* By device tree node label */
const struct device *led_dev = DEVICE_DT_GET(DT_NODELABEL(led_controller));

/* By device tree path */
const struct device *led_dev = DEVICE_DT_GET(DT_PATH(i2c0, is31fl3235a_3c));

/* By instance (if multiple devices) */
const struct device *led_dev = DEVICE_DT_GET(DT_INST(0, issi_is31fl3235a));

/* Check if device is ready */
if (!device_is_ready(led_dev)) {
    printk("LED device not ready\n");
    return -ENODEV;
}
```

## Standard Zephyr LED API

Defined in `<zephyr/drivers/led.h>`.

### led_set_brightness()

Set brightness for a single LED channel.

```c
int led_set_brightness(const struct device *dev, uint32_t led, uint8_t value);
```

**Parameters:**
- `dev`: Pointer to LED device structure
- `led`: LED channel number (0-27)
- `value`: Brightness value (0-255)
  - 0 = off
  - 255 = maximum brightness
  - Linear scale between

**Returns:**
- `0`: Success
- `-EINVAL`: Invalid channel number
- `-EIO`: I2C communication error
- `-ENODEV`: Device not ready

**Notes:**
- Automatically triggers update (writes to update register)
- Thread-safe (uses internal mutex)
- Changes take effect immediately

**Example:**
```c
/* Set channel 0 to 50% brightness */
int ret = led_set_brightness(led_dev, 0, 128);
if (ret < 0) {
    printk("Failed to set brightness: %d\n", ret);
}

/* Turn off channel 5 */
led_set_brightness(led_dev, 5, 0);

/* Maximum brightness on channel 10 */
led_set_brightness(led_dev, 10, 255);
```

### led_write_channels()

Write brightness values to multiple consecutive channels.

```c
int led_write_channels(const struct device *dev,
                       uint32_t start_channel,
                       uint32_t num_channels,
                       const uint8_t *buf);
```

**Parameters:**
- `dev`: Pointer to LED device structure
- `start_channel`: First channel number (0-27)
- `num_channels`: Number of consecutive channels to write
- `buf`: Array of brightness values (0-255)

**Returns:**
- `0`: Success
- `-EINVAL`: Invalid channel range
- `-EIO`: I2C communication error

**Notes:**
- More efficient than multiple `led_set_brightness()` calls
- All channels update simultaneously (single update trigger)
- Thread-safe

**Example:**
```c
/* Set RGB LED (channels 0-2) to purple (R=255, G=0, B=255) */
uint8_t rgb_values[] = {255, 0, 255};
led_write_channels(led_dev, 0, 3, rgb_values);

/* Set 10 channels to gradient */
uint8_t gradient[10];
for (int i = 0; i < 10; i++) {
    gradient[i] = i * 25;  /* 0, 25, 50, ..., 225 */
}
led_write_channels(led_dev, 0, 10, gradient);
```

### led_on()

Turn LED on to maximum brightness (optional, may not be implemented).

```c
int led_on(const struct device *dev, uint32_t led);
```

**Equivalent to:** `led_set_brightness(dev, led, 255)`

### led_off()

Turn LED off (optional, may not be implemented).

```c
int led_off(const struct device *dev, uint32_t led);
```

**Equivalent to:** `led_set_brightness(dev, led, 0)`

## Extended IS31FL3235A API

Defined in `<zephyr/drivers/led/is31fl3235a.h>`.

### Current Scaling

#### is31fl3235a_current_scale (enum)

Current scaling factors.

```c
enum is31fl3235a_current_scale {
    IS31FL3235A_SCALE_1X = 0,      /**< 100% of IMAX */
    IS31FL3235A_SCALE_1_2X = 1,    /**< 50% of IMAX */
    IS31FL3235A_SCALE_1_3X = 2,    /**< 33% of IMAX */
    IS31FL3235A_SCALE_1_4X = 3,    /**< 25% of IMAX */
};
```

**Note:** IMAX is set by external resistor (hardware).

#### is31fl3235a_set_current_scale()

Set current scaling factor for a channel.

```c
int is31fl3235a_set_current_scale(const struct device *dev,
                                   uint8_t channel,
                                   enum is31fl3235a_current_scale scale);
```

**Parameters:**
- `dev`: Pointer to LED device structure
- `channel`: Channel number (0-27)
- `scale`: Current scaling factor

**Returns:**
- `0`: Success
- `-EINVAL`: Invalid channel or scale value
- `-EIO`: I2C communication error

**Notes:**
- Automatically triggers update
- Affects maximum current, not PWM brightness
- Use to balance different LED types on same driver

**Example:**
```c
/* Reduce channel 0 current to 50% */
is31fl3235a_set_current_scale(led_dev, 0, IS31FL3235A_SCALE_1_2X);

/* Balance RGB LED currents (common use case) */
is31fl3235a_set_current_scale(led_dev, 0, IS31FL3235A_SCALE_1_4X);  /* Red */
is31fl3235a_set_current_scale(led_dev, 1, IS31FL3235A_SCALE_1_2X);  /* Green */
is31fl3235a_set_current_scale(led_dev, 2, IS31FL3235A_SCALE_1X);    /* Blue */
```

### Channel Enable/Disable

#### is31fl3235a_channel_enable()

Enable or disable a channel.

```c
int is31fl3235a_channel_enable(const struct device *dev,
                                uint8_t channel,
                                bool enable);
```

**Parameters:**
- `dev`: Pointer to LED device structure
- `channel`: Channel number (0-27)
- `enable`: `true` to enable, `false` to disable

**Returns:**
- `0`: Success
- `-EINVAL`: Invalid channel
- `-EIO`: I2C communication error

**Notes:**
- Disabled channels produce no output regardless of PWM setting
- More efficient than setting brightness to 0 if temporarily disabling
- Automatically triggers update

**Example:**
```c
/* Disable channel 5 */
is31fl3235a_channel_enable(led_dev, 5, false);

/* Re-enable channel 5 */
is31fl3235a_channel_enable(led_dev, 5, true);
```

### Power Management

#### is31fl3235a_sw_shutdown()

Software shutdown control.

```c
int is31fl3235a_sw_shutdown(const struct device *dev, bool shutdown);
```

**Parameters:**
- `dev`: Pointer to LED device structure
- `shutdown`: `true` to enter shutdown, `false` to wake

**Returns:**
- `0`: Success
- `-EIO`: I2C communication error

**Notes:**
- In shutdown: all outputs off, registers retain values, low power
- Resume: outputs return to previous state
- Does not require update trigger
- I2C communication still works in shutdown mode

**Example:**
```c
/* Enter low power mode */
is31fl3235a_sw_shutdown(led_dev, true);

/* Wake from shutdown */
is31fl3235a_sw_shutdown(led_dev, false);
```

#### is31fl3235a_hw_shutdown()

Hardware shutdown control via SDB pin.

```c
int is31fl3235a_hw_shutdown(const struct device *dev, bool shutdown);
```

**Parameters:**
- `dev`: Pointer to LED device structure
- `shutdown`: `true` to shutdown (SDB low), `false` to wake (SDB high)

**Returns:**
- `0`: Success
- `-ENOTSUP`: SDB pin not configured in device tree
- `-EIO`: GPIO control error

**Notes:**
- Lowest power consumption mode
- SDB pin must be defined in device tree
- All outputs off when in hardware shutdown
- Registers retain values

**Example:**
```c
/* Hardware shutdown (if SDB configured) */
int ret = is31fl3235a_hw_shutdown(led_dev, true);
if (ret == -ENOTSUP) {
    printk("SDB pin not configured, using software shutdown\n");
    is31fl3235a_sw_shutdown(led_dev, true);
}

/* Wake from hardware shutdown */
is31fl3235a_hw_shutdown(led_dev, false);
```

### Manual Update Control

#### is31fl3235a_update()

Manually trigger update of buffered register values.

```c
int is31fl3235a_update(const struct device *dev);
```

**Parameters:**
- `dev`: Pointer to LED device structure

**Returns:**
- `0`: Success
- `-EIO`: I2C communication error

**Notes:**
- For advanced use cases only
- Standard API functions automatically call this
- Use when you need fine control over update timing
- Applies all pending PWM and control register changes

**Example:**
```c
/* Advanced: batch updates without triggering */
/* (This would require internal API access - not normally exposed) */

/* Normally, just use standard API which handles updates automatically */
led_set_brightness(led_dev, 0, 100);  /* Auto-updates */
```

## Complete Usage Examples

### Example 1: Simple Brightness Control

```c
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>

void simple_led_control(void)
{
    const struct device *led = DEVICE_DT_GET(DT_NODELABEL(led_controller));

    if (!device_is_ready(led)) {
        printk("LED device not ready\n");
        return;
    }

    /* Turn on LED at 50% */
    led_set_brightness(led, 0, 128);

    /* Wait 1 second */
    k_sleep(K_SECONDS(1));

    /* Turn off */
    led_set_brightness(led, 0, 0);
}
```

### Example 2: RGB LED Control

```c
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>

struct rgb_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

void set_rgb_led(const struct device *led, uint8_t base_channel,
                  struct rgb_color *color)
{
    uint8_t rgb[3] = {color->r, color->g, color->b};
    led_write_channels(led, base_channel, 3, rgb);
}

void rgb_demo(void)
{
    const struct device *led = DEVICE_DT_GET(DT_NODELABEL(led_controller));
    struct rgb_color colors[] = {
        {255, 0, 0},    /* Red */
        {0, 255, 0},    /* Green */
        {0, 0, 255},    /* Blue */
        {255, 255, 0},  /* Yellow */
        {255, 0, 255},  /* Magenta */
        {0, 255, 255},  /* Cyan */
        {255, 255, 255} /* White */
    };

    for (int i = 0; i < ARRAY_SIZE(colors); i++) {
        set_rgb_led(led, 0, &colors[i]);
        k_sleep(K_MSEC(500));
    }
}
```

### Example 3: Current Scaling for LED Balance

```c
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/is31fl3235a.h>

void balance_rgb_currents(const struct device *led)
{
    /*
     * Common RGB LEDs have different brightness at same current.
     * Scale currents to balance perceived brightness.
     */

    /* Red is typically brightest - reduce to 25% */
    is31fl3235a_set_current_scale(led, 0, IS31FL3235A_SCALE_1_4X);

    /* Green is medium - reduce to 50% */
    is31fl3235a_set_current_scale(led, 1, IS31FL3235A_SCALE_1_2X);

    /* Blue is dimmest - keep at 100% */
    is31fl3235a_set_current_scale(led, 2, IS31FL3235A_SCALE_1X);

    /* Now all at max brightness (255) appear balanced */
    uint8_t white[3] = {255, 255, 255};
    led_write_channels(led, 0, 3, white);
}
```

### Example 4: Breathing LED Effect

```c
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>

void breathing_effect(const struct device *led, uint8_t channel)
{
    while (1) {
        /* Fade in */
        for (int brightness = 0; brightness <= 255; brightness += 5) {
            led_set_brightness(led, channel, brightness);
            k_sleep(K_MSEC(20));
        }

        /* Fade out */
        for (int brightness = 255; brightness >= 0; brightness -= 5) {
            led_set_brightness(led, channel, brightness);
            k_sleep(K_MSEC(20));
        }
    }
}
```

### Example 5: Multi-Channel Animation

```c
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>

#define NUM_LEDS 28

void knight_rider_effect(const struct device *led)
{
    uint8_t brightness[NUM_LEDS];

    while (1) {
        /* Sweep right */
        for (int pos = 0; pos < NUM_LEDS; pos++) {
            memset(brightness, 0, sizeof(brightness));

            /* Current LED at max */
            brightness[pos] = 255;

            /* Trail behind */
            if (pos > 0) brightness[pos - 1] = 128;
            if (pos > 1) brightness[pos - 2] = 64;

            led_write_channels(led, 0, NUM_LEDS, brightness);
            k_sleep(K_MSEC(50));
        }

        /* Sweep left */
        for (int pos = NUM_LEDS - 1; pos >= 0; pos--) {
            memset(brightness, 0, sizeof(brightness));

            brightness[pos] = 255;
            if (pos < NUM_LEDS - 1) brightness[pos + 1] = 128;
            if (pos < NUM_LEDS - 2) brightness[pos + 2] = 64;

            led_write_channels(led, 0, NUM_LEDS, brightness);
            k_sleep(K_MSEC(50));
        }
    }
}
```

### Example 6: Power Management

```c
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/is31fl3235a.h>

void power_saving_demo(const struct device *led)
{
    /* Normal operation */
    led_set_brightness(led, 0, 255);
    k_sleep(K_SECONDS(5));

    /* Enter low power mode */
    printk("Entering shutdown...\n");
    is31fl3235a_sw_shutdown(led, true);
    k_sleep(K_SECONDS(10));

    /* Wake and resume */
    printk("Waking up...\n");
    is31fl3235a_sw_shutdown(led, false);

    /* LED automatically returns to previous state (channel 0 at 255) */
}
```

## Thread Safety

All API functions are thread-safe and can be called from multiple threads or ISR contexts (with caution).

**Considerations:**
- Internal mutex protects device access
- I2C operations may block
- Avoid calling from time-critical ISRs
- Consider using work queue for LED updates from ISRs

**Example:**
```c
/* Safe: Call from multiple threads */
void thread_a(void *led_dev, void *unused1, void *unused2)
{
    while (1) {
        led_set_brightness(led_dev, 0, 255);
        k_sleep(K_MSEC(500));
        led_set_brightness(led_dev, 0, 0);
        k_sleep(K_MSEC(500));
    }
}

void thread_b(void *led_dev, void *unused1, void *unused2)
{
    while (1) {
        led_set_brightness(led_dev, 1, 255);
        k_sleep(K_MSEC(250));
        led_set_brightness(led_dev, 1, 0);
        k_sleep(K_MSEC(250));
    }
}
```

## Error Handling

All functions return negative errno codes on failure:

```c
int ret = led_set_brightness(led_dev, 0, 128);
if (ret < 0) {
    switch (ret) {
    case -EINVAL:
        printk("Invalid parameter\n");
        break;
    case -EIO:
        printk("I2C communication error\n");
        break;
    case -ENODEV:
        printk("Device not ready\n");
        break;
    case -ENOTSUP:
        printk("Feature not supported\n");
        break;
    default:
        printk("Unknown error: %d\n", ret);
    }
}
```

## Performance Considerations

### Prefer Batch Operations

```c
/* Less efficient: 3 I2C transactions + 3 updates */
led_set_brightness(led_dev, 0, 255);
led_set_brightness(led_dev, 1, 128);
led_set_brightness(led_dev, 2, 64);

/* More efficient: 1 I2C transaction + 1 update */
uint8_t values[] = {255, 128, 64};
led_write_channels(led_dev, 0, 3, values);
```

### I2C Speed

Configure I2C bus to 400kHz for best performance:

```dts
&i2c0 {
    clock-frequency = <I2C_BITRATE_FAST>;  /* 400kHz */
};
```

### Update Frequency

Consider the PWM frequency when updating LEDs:
- 3kHz PWM: Updates visible every ~0.3ms
- 22kHz PWM: Updates visible every ~0.045ms

Updating faster than PWM period provides no visual benefit.

## Summary

### Standard LED API (always use when possible)
- `led_set_brightness()` - Set single channel
- `led_write_channels()` - Set multiple channels

### Extended API (IS31FL3235A-specific)
- `is31fl3235a_set_current_scale()` - Adjust current scaling
- `is31fl3235a_channel_enable()` - Enable/disable channel
- `is31fl3235a_sw_shutdown()` - Software power control
- `is31fl3235a_hw_shutdown()` - Hardware power control (requires SDB pin)
- `is31fl3235a_update()` - Manual update (advanced)

### Best Practices
1. Use standard LED API when possible for portability
2. Use extended API for IS31FL3235A-specific features
3. Prefer `led_write_channels()` for multi-channel updates
4. Check `device_is_ready()` before use
5. Handle error codes appropriately
6. Configure I2C to 400kHz for performance
7. Use current scaling to balance LED brightness
8. Use shutdown for power savings
