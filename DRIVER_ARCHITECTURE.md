# IS31FL3235A Driver Architecture

## Overview

This document details the software architecture for the IS31FL3235A Zephyr RTOS driver.

## Directory Structure

```
zephyr/
├── drivers/led/
│   ├── is31fl3235a.c            # Main driver implementation
│   └── is31fl3235a_regs.h       # Register definitions (private)
├── dts/bindings/led/
│   └── issi,is31fl3235a.yaml    # Device tree binding
└── include/zephyr/drivers/led/
    └── is31fl3235a.h            # Public extended API header
```

## Data Structures

### Configuration Structure (ROM)

Stored in flash, populated from device tree at compile time.

```c
struct is31fl3235a_cfg {
    struct i2c_dt_spec i2c;       /* I2C bus and address */
    struct gpio_dt_spec sdb_gpio; /* Optional SDB (shutdown) GPIO */
    bool pwm_freq_22khz;          /* PWM frequency: false=3kHz, true=22kHz */
};
```

**Device Tree Mapping:**
- `i2c`: From I2C bus and `reg` property
- `sdb_gpio`: From `sdb-gpios` property (if present)
- `pwm_freq_22khz`: Derived from `pwm-frequency` property

### Runtime Data Structure (RAM)

```c
struct is31fl3235a_data {
    struct k_mutex lock;                         /* Thread safety */
    bool initialized;                            /* Init complete flag */
    bool sw_shutdown;                            /* Software shutdown state */
    bool hw_shutdown;                            /* Hardware shutdown state */
    uint8_t pwm_cache[IS31FL3235A_NUM_CHANNELS]; /* PWM value cache */
    uint8_t ctrl_cache[IS31FL3235A_NUM_CHANNELS];/* Control register cache */
};
```

**Caching Benefits:**
- Enables read-modify-write of control registers (device does not support reading)
- Supports synchronized multi-channel updates
- Tracks register state in software

## Register Access

See [REGISTER_DEFINITIONS.md](REGISTER_DEFINITIONS.md) for complete register map.

### Key Registers

| Register | Address | Purpose |
|----------|---------|---------|
| Shutdown | 0x00 | Software shutdown control |
| PWM | 0x05-0x20 | Per-channel brightness (28 channels) |
| Update | 0x25 | Apply buffered changes |
| Control | 0x2A-0x45 | Per-channel enable + current scale |
| Global Control | 0x4A | Global LED enable/disable |
| Frequency | 0x4B | PWM frequency selection |
| Reset | 0x4F | Software reset |

## API Design

### Standard LED API

```c
static const struct led_driver_api is31fl3235a_led_api = {
    .set_brightness = is31fl3235a_led_set_brightness,
    .write_channels = is31fl3235a_led_write_channels,
    .on = is31fl3235a_led_on,
    .off = is31fl3235a_led_off,
};
```

### Extended API

See [API_SPECIFICATION.md](API_SPECIFICATION.md) for detailed documentation.

**Current Scaling:**
- `is31fl3235a_set_current_scale()` - Per-channel current adjustment

**Channel Control:**
- `is31fl3235a_channel_enable()` - Single channel enable/disable
- `is31fl3235a_channels_enable()` - Multi-channel enable/disable
- `is31fl3235a_channels_enable_no_update()` - Multi-channel without auto-update

**Power Management:**
- `is31fl3235a_global_enable()` - Global LED output control
- `is31fl3235a_sw_shutdown()` - Software shutdown
- `is31fl3235a_hw_shutdown()` - Hardware shutdown via SDB pin

**Manual Update Control:**
- `is31fl3235a_update()` - Trigger buffered register update
- `is31fl3235a_set_brightness_no_update()` - Set brightness without update
- `is31fl3235a_write_channels_no_update()` - Write channels without update

## I2C Communication

### Helper Functions

```c
static int is31fl3235a_write_reg(const struct device *dev,
                                  uint8_t reg, uint8_t value);

static int is31fl3235a_write_buffer(const struct device *dev,
                                     uint8_t reg, const uint8_t *buf, size_t len);

static inline int is31fl3235a_trigger_update(const struct device *dev);
```

### Error Handling

All I2C functions:
1. Check I2C bus ready before operation
2. Return negative errno on failure
3. Log errors using Zephyr logging subsystem
4. Release mutex on error paths

## Initialization Sequence

1. Initialize mutex
2. Check I2C bus ready
3. Configure SDB pin (if present) and set high
4. Reset chip to known state
5. Wake from software shutdown
6. Configure PWM frequency
7. Initialize all channels: enabled, 1x current, 0 brightness
8. Trigger update to apply settings

## Device Instantiation

```c
#define IS31FL3235A_DEFINE(inst)                                    \
    static struct is31fl3235a_data is31fl3235a_data_##inst;         \
    static const struct is31fl3235a_cfg is31fl3235a_cfg_##inst = {  \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                          \
        .sdb_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, sdb_gpios, {0}), \
        .pwm_freq_22khz = (DT_INST_PROP(inst, pwm_frequency) == 22000), \
    };                                                               \
    DEVICE_DT_INST_DEFINE(inst, is31fl3235a_init, NULL,             \
                          &is31fl3235a_data_##inst,                  \
                          &is31fl3235a_cfg_##inst,                   \
                          POST_KERNEL, CONFIG_LED_INIT_PRIORITY,     \
                          &is31fl3235a_led_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3235A_DEFINE)
```

## Thread Safety

All public API functions:
1. Acquire `data->lock` mutex before device access
2. Release mutex before returning (including error paths)
3. Use `k_mutex_lock(&data->lock, K_FOREVER)` for blocking

Internal helper functions assume mutex is already held.

## Logging

```c
LOG_MODULE_REGISTER(is31fl3235a, CONFIG_LED_LOG_LEVEL);

LOG_ERR("I2C write failed: %d", ret);
LOG_WRN("Invalid channel %u", channel);
LOG_INF("Device initialized");
LOG_DBG("Set channel %u to %u", channel, value);
```

## Error Codes

| Code | Meaning |
|------|---------|
| `-EINVAL` | Invalid parameter (channel out of range, etc.) |
| `-ENODEV` | Device not ready (I2C bus, GPIO) |
| `-EIO` | I2C communication error |
| `-ENOTSUP` | Feature not supported (e.g., SDB not configured) |

## Memory Footprint

Estimated per-instance:
- **ROM:** ~3KB (code + config)
- **RAM:** ~80 bytes (data structure with caches)
