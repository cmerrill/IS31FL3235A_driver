# IS31FL3235A Driver Architecture

## Overview

This document details the software architecture for the IS31FL3235A Zephyr RTOS driver, including data structures, API design, and implementation patterns.

## Directory Structure

```
zephyr/
├── drivers/led/
│   ├── CMakeLists.txt           # Add is31fl3235a.c when CONFIG_LED_IS31FL3235A=y
│   ├── Kconfig                  # Add source for Kconfig.is31fl3235a
│   ├── Kconfig.is31fl3235a      # New: Driver configuration options
│   ├── is31fl3235a.c            # New: Main driver implementation
│   └── is31fl3235a_regs.h       # New: Register definitions (private)
├── dts/bindings/led/
│   └── issi,is31fl3235a.yaml    # New: Device tree binding
└── include/zephyr/drivers/led/
    ├── is31fl3235a.h            # New: Public extended API header
    └── led.h                    # Existing: Standard LED API
```

## Data Structures

### Configuration Structure (Read-Only)

Stored in flash/ROM, populated from device tree at compile time.

```c
/**
 * @brief IS31FL3235A device configuration
 *
 * This structure contains compile-time configuration from device tree.
 * It is stored in ROM and never modified at runtime.
 */
struct is31fl3235a_cfg {
    /** I2C bus specification from device tree */
    struct i2c_dt_spec i2c;

    /** Optional SDB (shutdown) GPIO pin specification */
    struct gpio_dt_spec sdb_gpio;

    /** PWM frequency setting: false=3kHz, true=22kHz */
    bool pwm_freq_22khz;
};
```

**Device Tree Mapping:**
- `i2c`: Extracted from I2C bus and `reg` property
- `sdb_gpio`: Extracted from `sdb-gpios` property (if present)
- `pwm_freq_22khz`: Derived from `pwm-frequency` property (3000→false, 22000→true)

### Runtime Data Structure (Read-Write)

Stored in RAM, modified during driver operation.

```c
/**
 * @brief IS31FL3235A runtime data
 *
 * This structure contains runtime state and cached register values.
 * It is stored in RAM and protected by the mutex.
 */
struct is31fl3235a_data {
    /** Mutex to protect concurrent access to device */
    struct k_mutex lock;

    /** Initialization complete flag */
    bool initialized;

    /** Software shutdown state (true=shutdown) */
    bool sw_shutdown;

    /** Hardware shutdown state (true=shutdown) */
    bool hw_shutdown;

    /** Cached PWM values (0-255) for all 28 channels */
    uint8_t pwm_cache[IS31FL3235A_NUM_CHANNELS];

    /** Cached LED control register values for all 28 channels */
    uint8_t ctrl_cache[IS31FL3235A_NUM_CHANNELS];
};
```

**Purpose of Caching:**
- Allows read-modify-write of control registers
- Enables synchronization (write multiple, update once)
- Reduces I2C bus traffic (only write when changed)
- Supports state queries

## Register Definitions

### Register Map

```c
/* Configuration and Control Registers */
#define IS31FL3235A_REG_SHUTDOWN     0x00  /* Software shutdown control */
#define IS31FL3235A_REG_PWM_BASE     0x05  /* PWM registers base (CH1) */
#define IS31FL3235A_REG_PWM_END      0x20  /* PWM registers end (CH28) */
#define IS31FL3235A_REG_UPDATE       0x25  /* Update/load register */
#define IS31FL3235A_REG_CTRL_BASE    0x2A  /* LED control registers base */
#define IS31FL3235A_REG_CTRL_END     0x45  /* LED control registers end */
#define IS31FL3235A_REG_FREQ         0x4B  /* PWM frequency select */
#define IS31FL3235A_REG_RESET        0x4F  /* Reset register */

/* Helper macros for register access */
#define IS31FL3235A_PWM_REG(ch)      (IS31FL3235A_REG_PWM_BASE + (ch))
#define IS31FL3235A_CTRL_REG(ch)     (IS31FL3235A_REG_CTRL_BASE + (ch))

/* Constants */
#define IS31FL3235A_NUM_CHANNELS     28
#define IS31FL3235A_MAX_BRIGHTNESS   255
```

### Shutdown Register (0x00)

```c
/* Shutdown register bits */
#define IS31FL3235A_SHUTDOWN_NORMAL   BIT(0)  /* Set=normal, Clear=shutdown */
```

**Values:**
- `0x00`: Software shutdown mode (all outputs off, registers retain data)
- `0x01`: Normal operation

### Update Register (0x25)

```c
/* Update register - write 0x00 to load buffered values */
#define IS31FL3235A_UPDATE_TRIGGER    0x00
```

**Operation:**
Writing `0x00` to this register transfers all buffered PWM and LED Control values to the active registers.

### LED Control Register (0x2A-0x45)

**TBD: Need exact bit layout from datasheet**

Preliminary structure:
```c
/* LED Control register bit fields (VERIFY FROM DATASHEET) */
#define IS31FL3235A_CTRL_ENABLE       BIT(?)  /* Enable output */
#define IS31FL3235A_CTRL_SCALE_SHIFT  ?       /* Current scale field position */
#define IS31FL3235A_CTRL_SCALE_MASK   (0x?? << IS31FL3235A_CTRL_SCALE_SHIFT)

/* Current scaling values */
#define IS31FL3235A_SCALE_1X          (0x?? << IS31FL3235A_CTRL_SCALE_SHIFT)
#define IS31FL3235A_SCALE_HALF        (0x?? << IS31FL3235A_CTRL_SCALE_SHIFT)
#define IS31FL3235A_SCALE_THIRD       (0x?? << IS31FL3235A_CTRL_SCALE_SHIFT)
#define IS31FL3235A_SCALE_QUARTER     (0x?? << IS31FL3235A_CTRL_SCALE_SHIFT)
```

### Frequency Register (0x4B)

```c
/* PWM frequency selection */
#define IS31FL3235A_FREQ_3KHZ         0x00    /* Default: 3kHz */
#define IS31FL3235A_FREQ_22KHZ        0x01    /* High frequency: 22kHz */
```

### Reset Register (0x4F)

**TBD: Need to verify reset mechanism from datasheet**

Possible implementations:
- Write any value to trigger reset
- Write specific value (e.g., 0x00)

## API Design

### Standard LED API (include/zephyr/drivers/led.h)

The driver implements the standard Zephyr LED API:

```c
static const struct led_driver_api is31fl3235a_led_api = {
    .set_brightness = is31fl3235a_led_set_brightness,
    .write_channels = is31fl3235a_led_write_channels,
    .on = is31fl3235a_led_on,      /* Optional: set brightness to max */
    .off = is31fl3235a_led_off,    /* Optional: set brightness to 0 */
};
```

#### Standard API Functions

**led_set_brightness()**
```c
/**
 * Set brightness of a single LED channel
 *
 * @param dev Device structure pointer
 * @param led Channel number (0-27)
 * @param value Brightness value (0-255)
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_led_set_brightness(const struct device *dev,
                                           uint32_t led,
                                           uint8_t value);
```

**Implementation:**
1. Validate channel number (0-27)
2. Acquire mutex
3. Write PWM value to register
4. Update cache
5. Trigger update (write 0x00 to update register)
6. Release mutex

**led_write_channels()**
```c
/**
 * Write brightness values to multiple channels simultaneously
 *
 * @param dev Device structure pointer
 * @param start_channel First channel number
 * @param num_channels Number of consecutive channels
 * @param buf Buffer containing brightness values
 * @return 0 on success, negative errno on error
 */
static int is31fl3235a_led_write_channels(const struct device *dev,
                                           uint32_t start_channel,
                                           uint32_t num_channels,
                                           const uint8_t *buf);
```

**Implementation:**
1. Validate channel range
2. Acquire mutex
3. Write all PWM values to registers (buffered)
4. Update cache
5. Trigger single update at end
6. Release mutex

### Extended API (include/zephyr/drivers/led/is31fl3235a.h)

IS31FL3235A-specific functions not in standard LED API.

#### Current Scaling API

```c
/**
 * @brief Current scaling factors
 */
enum is31fl3235a_current_scale {
    IS31FL3235A_SCALE_1X = 0,      /* 100% of max current */
    IS31FL3235A_SCALE_1_2X = 1,    /* 50% of max current */
    IS31FL3235A_SCALE_1_3X = 2,    /* 33% of max current */
    IS31FL3235A_SCALE_1_4X = 3,    /* 25% of max current */
};

/**
 * @brief Set current scaling for a channel
 *
 * The maximum output current is set by hardware (external resistor).
 * This function scales the output current by the specified factor.
 *
 * @param dev Device structure pointer
 * @param channel Channel number (0-27)
 * @param scale Current scaling factor
 * @return 0 on success, negative errno on error
 * @retval -EINVAL Invalid channel or scale value
 * @retval -EIO I2C communication error
 */
int is31fl3235a_set_current_scale(const struct device *dev,
                                   uint8_t channel,
                                   enum is31fl3235a_current_scale scale);
```

**Implementation:**
1. Validate channel and scale
2. Acquire mutex
3. Read cached control register value
4. Modify scale bits
5. Write to control register
6. Update cache
7. Trigger update
8. Release mutex

#### Channel Enable/Disable API

```c
/**
 * @brief Enable or disable a channel
 *
 * Disabled channels produce no output regardless of PWM setting.
 *
 * @param dev Device structure pointer
 * @param channel Channel number (0-27)
 * @param enable true to enable, false to disable
 * @return 0 on success, negative errno on error
 */
int is31fl3235a_channel_enable(const struct device *dev,
                                uint8_t channel,
                                bool enable);
```

#### Shutdown APIs

```c
/**
 * @brief Software shutdown control
 *
 * In software shutdown mode, all LED outputs are turned off but
 * register values are retained. This reduces power consumption.
 *
 * @param dev Device structure pointer
 * @param shutdown true to enter shutdown, false to wake
 * @return 0 on success, negative errno on error
 */
int is31fl3235a_sw_shutdown(const struct device *dev, bool shutdown);

/**
 * @brief Hardware shutdown control via SDB pin
 *
 * Controls the SDB (shutdown) GPIO pin if configured in device tree.
 * When SDB is low, chip enters hardware shutdown (lowest power mode).
 *
 * @param dev Device structure pointer
 * @param shutdown true to shutdown (SDB low), false to wake (SDB high)
 * @return 0 on success, negative errno on error
 * @retval -ENOTSUP SDB pin not configured in device tree
 */
int is31fl3235a_hw_shutdown(const struct device *dev, bool shutdown);
```

#### Manual Update API

```c
/**
 * @brief Manually trigger update of buffered register values
 *
 * Normally, brightness and control register updates are automatic.
 * This function allows manual control for advanced use cases.
 *
 * Note: Most users should use the standard LED API which handles
 * updates automatically.
 *
 * @param dev Device structure pointer
 * @return 0 on success, negative errno on error
 */
int is31fl3235a_update(const struct device *dev);
```

## I2C Communication Layer

### Helper Functions

```c
/**
 * @brief Write a single byte to a register
 */
static int is31fl3235a_write_reg(const struct device *dev,
                                  uint8_t reg,
                                  uint8_t value);

/**
 * @brief Write multiple bytes starting at a register
 */
static int is31fl3235a_write_buffer(const struct device *dev,
                                     uint8_t reg,
                                     const uint8_t *buf,
                                     size_t len);

/**
 * @brief Read a single byte from a register
 */
static int is31fl3235a_read_reg(const struct device *dev,
                                 uint8_t reg,
                                 uint8_t *value);

/**
 * @brief Trigger update register
 */
static inline int is31fl3235a_trigger_update(const struct device *dev)
{
    return is31fl3235a_write_reg(dev, IS31FL3235A_REG_UPDATE,
                                  IS31FL3235A_UPDATE_TRIGGER);
}
```

### Error Handling

All I2C functions should:
1. Check I2C bus ready before operation
2. Return negative errno on failure
3. Log errors using Zephyr logging subsystem
4. Clean up resources (release mutex, etc.) on error paths

## Initialization Sequence

The `is31fl3235a_init()` function performs:

```c
static int is31fl3235a_init(const struct device *dev)
{
    const struct is31fl3235a_cfg *cfg = dev->config;
    struct is31fl3235a_data *data = dev->data;
    int ret;

    /* 1. Initialize mutex */
    k_mutex_init(&data->lock);

    /* 2. Check I2C bus ready */
    if (!device_is_ready(cfg->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    /* 3. Configure SDB pin if present */
    if (cfg->sdb_gpio.port != NULL) {
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
        gpio_pin_set_dt(&cfg->sdb_gpio, 1);
        data->hw_shutdown = false;

        /* Wait for chip to wake up (check datasheet for timing) */
        k_msleep(1);
    }

    /* 4. Reset chip to known state */
    ret = is31fl3235a_write_reg(dev, IS31FL3235A_REG_RESET, 0x00);
    if (ret < 0) {
        LOG_ERR("Failed to reset chip: %d", ret);
        return ret;
    }

    /* Wait for reset to complete */
    k_msleep(1);

    /* 5. Wake from software shutdown */
    ret = is31fl3235a_write_reg(dev, IS31FL3235A_REG_SHUTDOWN,
                                 IS31FL3235A_SHUTDOWN_NORMAL);
    if (ret < 0) {
        LOG_ERR("Failed to wake from shutdown: %d", ret);
        return ret;
    }
    data->sw_shutdown = false;

    /* 6. Configure PWM frequency */
    uint8_t freq = cfg->pwm_freq_22khz ? IS31FL3235A_FREQ_22KHZ
                                        : IS31FL3235A_FREQ_3KHZ;
    ret = is31fl3235a_write_reg(dev, IS31FL3235A_REG_FREQ, freq);
    if (ret < 0) {
        LOG_ERR("Failed to set PWM frequency: %d", ret);
        return ret;
    }

    /* 7. Initialize all channels: enabled, 1x current, 0 brightness */
    for (int i = 0; i < IS31FL3235A_NUM_CHANNELS; i++) {
        /* Set PWM to 0 */
        ret = is31fl3235a_write_reg(dev, IS31FL3235A_PWM_REG(i), 0);
        if (ret < 0) {
            return ret;
        }
        data->pwm_cache[i] = 0;

        /* Set control: enabled, 1x current */
        uint8_t ctrl = IS31FL3235A_CTRL_ENABLE | IS31FL3235A_SCALE_1X;
        ret = is31fl3235a_write_reg(dev, IS31FL3235A_CTRL_REG(i), ctrl);
        if (ret < 0) {
            return ret;
        }
        data->ctrl_cache[i] = ctrl;
    }

    /* 8. Trigger update to apply all settings */
    ret = is31fl3235a_trigger_update(dev);
    if (ret < 0) {
        return ret;
    }

    /* 9. Mark as initialized */
    data->initialized = true;

    LOG_INF("IS31FL3235A initialized (PWM: %s)",
            cfg->pwm_freq_22khz ? "22kHz" : "3kHz");

    return 0;
}
```

## Device Instantiation

Using Zephyr's device tree macro system:

```c
#define IS31FL3235A_DEFINE(inst)                                              \
    static struct is31fl3235a_data is31fl3235a_data_##inst;                   \
                                                                               \
    static const struct is31fl3235a_cfg is31fl3235a_cfg_##inst = {           \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                    \
        .sdb_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, sdb_gpios, {0}),          \
        .pwm_freq_22khz = (DT_INST_PROP(inst, pwm_frequency) == 22000),      \
    };                                                                         \
                                                                               \
    DEVICE_DT_INST_DEFINE(inst,                                               \
                          is31fl3235a_init,                                   \
                          NULL,                                               \
                          &is31fl3235a_data_##inst,                           \
                          &is31fl3235a_cfg_##inst,                            \
                          POST_KERNEL,                                        \
                          CONFIG_LED_INIT_PRIORITY,                           \
                          &is31fl3235a_led_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3235A_DEFINE)
```

This macro:
- Creates one driver instance per device tree node
- Initializes config structure from device tree
- Allocates runtime data structure
- Registers device with LED API
- Runs at `POST_KERNEL` init level

## Thread Safety

All public API functions must:
1. Acquire `data->lock` mutex before device access
2. Release mutex before returning (including error paths)
3. Use `k_mutex_lock(&data->lock, K_FOREVER)` for blocking

Internal helper functions assume mutex is already held.

## Power Management (Optional Future Enhancement)

```c
#ifdef CONFIG_PM_DEVICE
static int is31fl3235a_pm_action(const struct device *dev,
                                  enum pm_device_action action)
{
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        return is31fl3235a_sw_shutdown(dev, true);

    case PM_DEVICE_ACTION_RESUME:
        return is31fl3235a_sw_shutdown(dev, false);

    default:
        return -ENOTSUP;
    }
}

PM_DEVICE_DT_INST_DEFINE(0, is31fl3235a_pm_action);
#endif
```

## Logging

Use Zephyr's logging subsystem:

```c
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(is31fl3235a, CONFIG_LED_LOG_LEVEL);

/* Usage examples */
LOG_ERR("I2C write failed: %d", ret);
LOG_WRN("Invalid channel %u (max %u)", channel, IS31FL3235A_NUM_CHANNELS - 1);
LOG_INF("Device initialized successfully");
LOG_DBG("Set channel %u brightness to %u", channel, value);
```

## Error Codes

Standard negative errno values:
- `-EINVAL`: Invalid parameter (channel out of range, etc.)
- `-ENODEV`: Device not ready (I2C bus, GPIO)
- `-EIO`: I2C communication error
- `-ENOTSUP`: Feature not supported (e.g., SDB not configured)
- `-EBUSY`: Resource busy (should not happen with proper mutex use)

## Performance Considerations

1. **Cache coherency**: Cache values to avoid unnecessary I2C reads
2. **Batch updates**: Use `write_channels()` instead of multiple `set_brightness()` calls
3. **Mutex granularity**: Hold mutex only during I2C transactions, not during validation
4. **I2C speed**: Configure I2C bus to 400kHz (Fast Mode) if supported

## Memory Footprint

Estimated per-instance memory usage:
- **ROM (flash):**
  - Code: ~2-3 KB
  - Config structure: ~20 bytes
- **RAM:**
  - Data structure: ~80 bytes (28×2 cache arrays + state)
  - Stack during calls: ~200 bytes

Total per instance: ~3KB ROM, ~100 bytes RAM
