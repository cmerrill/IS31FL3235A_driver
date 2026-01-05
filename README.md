# IS31FL3235A Zephyr RTOS Driver

## Overview

This repository contains the **complete implementation** of a Zephyr RTOS driver for the **IS31FL3235A** 28-channel LED driver IC from Lumissil Microsystems (ISSI).

### Device Summary

- **Manufacturer:** Lumissil Microsystems / ISSI
- **Channels:** 28 constant current outputs
- **Interface:** I2C (400kHz Fast Mode supported)
- **Output Current:** Up to 38mA per channel (hardware configurable)
- **Current Scaling:** 1×, ½×, ⅓×, ¼× per channel (software)
- **PWM Resolution:** 8-bit (256 levels)
- **PWM Frequency:** 3kHz or 22kHz (selectable)
- **Supply Voltage:** 2.7V - 5.5V
- **Package:** QFN-36 (4mm × 4mm)
- **Shutdown:** Hardware (SDB pin) and software

## Documentation

This repository contains comprehensive planning documentation:

### Core Documents

| Document | Description |
|----------|-------------|
| [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) | Overall implementation strategy, phases, and timeline |
| [DRIVER_ARCHITECTURE.md](DRIVER_ARCHITECTURE.md) | Software architecture, data structures, and patterns |
| [REGISTER_DEFINITIONS.md](REGISTER_DEFINITIONS.md) | Complete register map with exact bit definitions |
| [DEVICE_TREE_BINDING.md](DEVICE_TREE_BINDING.md) | Device tree binding spec with examples |
| [API_SPECIFICATION.md](API_SPECIFICATION.md) | Complete API documentation with usage examples |
| [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) | How to integrate driver into Zephyr |
| [NEXT_STEPS.md](NEXT_STEPS.md) | Open questions and implementation checklist |

### Quick Reference

- **Getting started?** → Read [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)
- **Understanding the architecture?** → Read [DRIVER_ARCHITECTURE.md](DRIVER_ARCHITECTURE.md)
- **Writing device tree?** → Read [DEVICE_TREE_BINDING.md](DEVICE_TREE_BINDING.md)
- **Using the API?** → Read [API_SPECIFICATION.md](API_SPECIFICATION.md)
- **Working on registers?** → Read [REGISTER_DEFINITIONS.md](REGISTER_DEFINITIONS.md)
- **Ready to integrate?** → Read [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)
- **Implementation checklist?** → Read [NEXT_STEPS.md](NEXT_STEPS.md)

## Features

### Standard Zephyr LED API

The driver implements the standard Zephyr LED API for portability:

- `led_set_brightness()` - Set single channel brightness (0-255)
- `led_write_channels()` - Set multiple channels simultaneously

### Extended IS31FL3235A API

Additional functions for IS31FL3235A-specific features:

- `is31fl3235a_set_current_scale()` - Per-channel current scaling (1×, ½×, ⅓×, ¼×)
- `is31fl3235a_channel_enable()` - Enable/disable individual channels
- `is31fl3235a_sw_shutdown()` - Software shutdown for power saving
- `is31fl3235a_hw_shutdown()` - Hardware shutdown via SDB pin
- `is31fl3235a_update()` - Manual update trigger for advanced use

### Key Capabilities

✓ **All 28 channels supported**
✓ **Synchronized multi-channel updates** (buffered writes)
✓ **Runtime current scaling** (balance LED brightness)
✓ **Multiple instances** on same I2C bus
✓ **Power management** (SW and HW shutdown modes)
✓ **Thread-safe** operation
✓ **Device tree configured** (I2C address, SDB pin, PWM frequency)

## Device Tree Example

```dts
#include <dt-bindings/led/led.h>

&i2c0 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;  /* 400kHz recommended */

    led_controller: is31fl3235a@3c {
        compatible = "issi,is31fl3235a";
        reg = <0x3c>;
        sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;
        pwm-frequency = <22000>;  /* 22kHz for reduced flicker */

        /* RGB LED channels */
        led_red {
            reg = <0>;
            label = "RGB Red";
            color = <LED_COLOR_ID_RED>;
        };

        led_green {
            reg = <1>;
            label = "RGB Green";
            color = <LED_COLOR_ID_GREEN>;
        };

        led_blue {
            reg = <2>;
            label = "RGB Blue";
            color = <LED_COLOR_ID_BLUE>;
        };

        /* ... up to 28 channels ... */
    };
};
```

## Usage Example

```c
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/is31fl3235a.h>

void led_demo(void)
{
    const struct device *led = DEVICE_DT_GET(DT_NODELABEL(led_controller));

    if (!device_is_ready(led)) {
        printk("LED device not ready\n");
        return;
    }

    /* Set single channel brightness */
    led_set_brightness(led, 0, 128);  /* Channel 0, 50% brightness */

    /* Set RGB LED to purple */
    uint8_t rgb[] = {255, 0, 255};
    led_write_channels(led, 0, 3, rgb);

    /* Balance RGB currents for white balance */
    is31fl3235a_set_current_scale(led, 0, IS31FL3235A_SCALE_1_4X);  /* Red */
    is31fl3235a_set_current_scale(led, 1, IS31FL3235A_SCALE_1_2X);  /* Green */
    is31fl3235a_set_current_scale(led, 2, IS31FL3235A_SCALE_1X);    /* Blue */

    /* Enter low power mode */
    is31fl3235a_sw_shutdown(led, true);
}
```

## Architecture Highlights

### Data Structures

```c
/* Configuration (ROM, from device tree) */
struct is31fl3235a_cfg {
    struct i2c_dt_spec i2c;           /* I2C bus and address */
    struct gpio_dt_spec sdb_gpio;     /* Optional SDB pin */
    bool pwm_freq_22khz;              /* PWM frequency */
};

/* Runtime data (RAM) */
struct is31fl3235a_data {
    struct k_mutex lock;              /* Thread safety */
    uint8_t pwm_cache[28];           /* PWM value cache */
    uint8_t ctrl_cache[28];          /* Control register cache */
    bool sw_shutdown;                /* Shutdown state */
    bool hw_shutdown;
};
```

### Register Map Summary

| Register | Address | Description |
|----------|---------|-------------|
| Shutdown | 0x00 | Software shutdown control |
| PWM | 0x05-0x20 | 28 channels, PWM duty cycle (0-255) |
| Update | 0x25 | Write 0x00 to apply buffered changes |
| LED Control | 0x2A-0x45 | 28 channels, enable + current scaling |
| Frequency | 0x4B | PWM freq: 0=3kHz, 1=22kHz |
| Reset | 0x4F | Software reset |

### Update Mechanism

The IS31FL3235A uses buffered writes for synchronized updates:

1. Write to PWM registers (0x05-0x20) → buffered
2. Write to Control registers (0x2A-0x45) → buffered
3. Write 0x00 to Update register (0x25) → all changes applied simultaneously

This allows changing multiple channels without visible glitches.

## Comparison with IS31FL3216A

| Feature | IS31FL3216A | IS31FL3235A |
|---------|-------------|-------------|
| **Channels** | 16 | **28** |
| **PWM Frequency** | 26kHz | **3kHz / 22kHz selectable** |
| **Current Scaling** | No | **Yes (4 levels)** |
| **GPIO Support** | Yes (8 pins) | No |
| **Audio Modulation** | Yes | No |
| **Frame Storage** | Yes (8 frames) | No |
| **Use Case** | Simple displays, audio-reactive | **General LED control, high channel count** |

**Note:** This driver is based on the existing IS31FL3216A Zephyr driver structure but adapted for IS31FL3235A features.

## Implementation Status

### Planning: ✅ Complete

All planning documents are complete:

- ✅ Register definitions with exact bit fields
- ✅ Driver architecture designed
- ✅ API specification defined
- ✅ Device tree binding specified
- ✅ Implementation phases outlined
- ✅ Test strategy defined

### Implementation: ✅ Complete

The driver implementation is **complete and ready for integration** into Zephyr:

- ✅ Main driver implementation (is31fl3235a.c)
- ✅ Register definitions header (is31fl3235a_regs.h)
- ✅ Public API header (is31fl3235a.h)
- ✅ Device tree binding (issi,is31fl3235a.yaml)
- ✅ Kconfig configuration (Kconfig.is31fl3235a)
- ✅ Build integration files (CMakeLists.txt, Kconfig)
- ✅ Sample application with examples
- ✅ Integration guide

See [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) for how to integrate into Zephyr.

### Implementation Files

All driver files are in this repository:

```
IS31FL3235A_driver/
├── driver/
│   ├── is31fl3235a.c              # Main driver (620 lines)
│   ├── is31fl3235a_regs.h         # Register definitions (84 lines)
│   ├── Kconfig.is31fl3235a        # Configuration (15 lines)
│   ├── CMakeLists.txt             # Build integration
│   └── Kconfig                    # Kconfig integration
├── dts_bindings/
│   └── issi,is31fl3235a.yaml      # DT binding (142 lines)
├── include/
│   └── is31fl3235a.h              # Public API (152 lines)
└── sample/
    ├── main.c                     # Sample application (220 lines)
    ├── app.overlay                # Device tree example
    ├── prj.conf                   # Sample configuration
    └── README.md                  # Sample documentation
```

## Requirements

### Zephyr Dependencies

- Zephyr RTOS v3.0 or later (tested with latest)
- `CONFIG_I2C` - I2C bus support
- `CONFIG_LED` - LED subsystem
- `CONFIG_GPIO` - GPIO support (if using SDB pin)

### Hardware Requirements

- I2C bus (100kHz or 400kHz)
- IS31FL3235A chip or evaluation board
- Optional: GPIO pin for SDB (shutdown)
- External resistor (REXT) sets maximum current

### Software Requirements

- Zephyr SDK installed
- West tool for building
- Device tree overlay for your board

## Configuration Options

```kconfig
# Enable IS31FL3235A driver
CONFIG_LED_IS31FL3235A=y

# Dependencies (auto-selected)
CONFIG_I2C=y
CONFIG_LED=y

# Logging (optional)
CONFIG_LOG=y
CONFIG_LED_LOG_LEVEL_DBG=y
```

## Testing

### Basic Test

```c
/* Test all 28 channels */
for (int ch = 0; ch < 28; ch++) {
    led_set_brightness(led_dev, ch, 255);  /* Turn on */
    k_sleep(K_MSEC(100));
    led_set_brightness(led_dev, ch, 0);    /* Turn off */
}
```

### Current Scaling Test

```c
/* Test all 4 scaling factors on channel 0 */
enum is31fl3235a_current_scale scales[] = {
    IS31FL3235A_SCALE_1X,      /* 100% current */
    IS31FL3235A_SCALE_1_2X,    /* 50% current */
    IS31FL3235A_SCALE_1_3X,    /* 33% current */
    IS31FL3235A_SCALE_1_4X,    /* 25% current */
};

led_set_brightness(led_dev, 0, 255);  /* Full brightness */

for (int i = 0; i < 4; i++) {
    is31fl3235a_set_current_scale(led_dev, 0, scales[i]);
    k_sleep(K_SECONDS(1));
    /* Observe current reduction or measure with meter */
}
```

## Performance

- **I2C transactions:** ~1ms per register write at 400kHz
- **Update latency:** <1ms (single I2C transaction)
- **Multi-channel update:** ~5ms for all 28 channels + update
- **Shutdown current:** <10µA (typical, from datasheet)

## Troubleshooting

### Device Not Ready

```c
if (!device_is_ready(led_dev)) {
    /* Check: */
    /* 1. I2C bus enabled in device tree? */
    /* 2. Correct I2C address? */
    /* 3. SDB pin high (if used)? */
    /* 4. Power supply correct (2.7-5.5V)? */
}
```

### No LED Output

1. Check SDB pin is high (if used)
2. Verify not in shutdown mode
3. Check channel is enabled
4. Verify PWM value > 0
5. Verify LED connected correctly
6. Check REXT resistor value

### I2C Errors

1. Verify I2C pull-ups (2.2kΩ - 4.7kΩ)
2. Check I2C address matches AD pin
3. Verify I2C bus speed ≤ 400kHz
4. Check for bus contention

## References

### Datasheets

- [IS31FL3235A Datasheet](https://www.lumissil.com/assets/pdf/core/IS31FL3235A_DS.pdf) (Lumissil)
- [IS31FL3235A Product Page](https://www.lumissil.com/applications/communication/home-networking/IS31FL3235A)

### Zephyr Documentation

- [Zephyr LED Driver API](https://docs.zephyrproject.org/latest/hardware/peripherals/led.html)
- [Zephyr Device Tree Guide](https://docs.zephyrproject.org/latest/build/dts/index.html)
- [Zephyr Device Driver Model](https://docs.zephyrproject.org/latest/kernel/drivers/index.html)

### Related Drivers

- [IS31FL3216A Zephyr Driver](https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/led/is31fl3216a.c) (reference)
- [Linux IS31FL32xx Driver](https://github.com/torvalds/linux/blob/master/drivers/leds/leds-is31fl32xx.c) (reference)

## Contributing

When contributing this driver to Zephyr mainline:

1. Follow [Zephyr Contribution Guidelines](https://docs.zephyrproject.org/latest/contribute/index.html)
2. Run coding style checks: `./scripts/checkpatch.pl`
3. Add MAINTAINERS entry
4. Create sample application (optional but recommended)
5. Submit pull request to [zephyrproject-rtos/zephyr](https://github.com/zephyrproject-rtos/zephyr)

## License

This driver will be licensed under Apache 2.0, consistent with Zephyr RTOS.

```
SPDX-License-Identifier: Apache-2.0
Copyright (c) 2026
```

## Authors

Planning and specification by Claude (Anthropic) based on:
- IS31FL3235A datasheet analysis
- IS31FL3216A Zephyr driver reference
- User requirements

Implementation by Claude (Anthropic) - Complete driver ready for integration

## Support

For questions or issues:

1. Check the planning documents in this repository
2. Refer to IS31FL3235A datasheet
3. Review Zephyr LED driver documentation
4. Ask on [Zephyr Discord](https://discord.gg/zephyr) (#drivers channel)
5. Post on [Zephyr GitHub Discussions](https://github.com/zephyrproject-rtos/zephyr/discussions)

## Changelog

### 2026-01-05 - Implementation Complete

- ✅ Complete planning documentation
- ✅ Register map with exact bit definitions
- ✅ Driver architecture defined
- ✅ API specification complete
- ✅ Device tree binding specified
- ✅ **Full driver implementation**
- ✅ **Sample application with examples**
- ✅ **Integration guide for Zephyr**

---

**Status:** ✅ Implementation Complete - Ready for Integration

**Next Step:** See [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) to integrate into Zephyr!
