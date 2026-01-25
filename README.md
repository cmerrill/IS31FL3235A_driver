# IS31FL3235A Zephyr RTOS Driver

## Overview

Zephyr RTOS driver for the **IS31FL3235A** 28-channel LED driver IC from Lumissil Microsystems.

### Device Summary

| Feature | Value |
|---------|-------|
| Manufacturer | Lumissil Microsystems |
| Channels | 28 constant current outputs |
| Interface | I2C (400kHz Fast Mode) |
| Output Current | Up to 38mA per channel (hardware configurable) |
| Current Scaling | 1x, 1/2x, 1/3x, 1/4x per channel (software) |
| PWM Resolution | 8-bit (256 levels) |
| PWM Frequency | 3kHz or 22kHz (selectable) |
| Supply Voltage | 2.7V - 5.5V |
| Package | QFN-36 (4mm x 4mm) |

## Documentation

| Document | Description |
|----------|-------------|
| [API_SPECIFICATION.md](API_SPECIFICATION.md) | Complete API documentation with usage examples |
| [DEVICE_TREE_BINDING.md](DEVICE_TREE_BINDING.md) | Device tree binding specification |
| [DRIVER_ARCHITECTURE.md](DRIVER_ARCHITECTURE.md) | Software architecture and data structures |
| [REGISTER_DEFINITIONS.md](REGISTER_DEFINITIONS.md) | Complete register map with bit definitions |
| [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) | How to integrate driver into Zephyr |

## Features

### Standard Zephyr LED API

- `led_set_brightness()` - Set single channel brightness (0-255)
- `led_write_channels()` - Set multiple channels simultaneously

### Extended API

Additional functions for IS31FL3235A-specific features:

| Function | Description |
|----------|-------------|
| `is31fl3235a_set_current_scale()` | Per-channel current scaling (1x, 1/2x, 1/3x, 1/4x) |
| `is31fl3235a_channel_enable()` | Enable/disable individual channel |
| `is31fl3235a_channels_enable()` | Enable/disable multiple channels |
| `is31fl3235a_channels_enable_no_update()` | Enable/disable multiple channels (no auto-update) |
| `is31fl3235a_global_enable()` | Global LED output enable/disable |
| `is31fl3235a_sw_shutdown()` | Software shutdown for power saving |
| `is31fl3235a_hw_shutdown()` | Hardware shutdown via SDB pin |
| `is31fl3235a_update()` | Manual update trigger |
| `is31fl3235a_set_brightness_no_update()` | Set brightness (no auto-update) |
| `is31fl3235a_write_channels_no_update()` | Write multiple channels (no auto-update) |

See [API_SPECIFICATION.md](API_SPECIFICATION.md) for detailed documentation.

## Device Tree Example

```dts
&i2c0 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;

    led_controller: is31fl3235a@3c {
        compatible = "issi,is31fl3235a";
        reg = <0x3c>;
        sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;
        pwm-frequency = <22000>;
    };
};
```

See [DEVICE_TREE_BINDING.md](DEVICE_TREE_BINDING.md) for complete specification.

## Usage Example

```c
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/is31fl3235a.h>

void led_demo(void)
{
    const struct device *led = DEVICE_DT_GET(DT_NODELABEL(led_controller));

    if (!device_is_ready(led)) {
        return;
    }

    /* Set single channel brightness */
    led_set_brightness(led, 0, 128);

    /* Set RGB LED to purple */
    uint8_t rgb[] = {255, 0, 255};
    led_write_channels(led, 0, 3, rgb);

    /* Adjust current scaling for white balance */
    is31fl3235a_set_current_scale(led, 0, IS31FL3235A_SCALE_1_4X);
    is31fl3235a_set_current_scale(led, 1, IS31FL3235A_SCALE_1_2X);
    is31fl3235a_set_current_scale(led, 2, IS31FL3235A_SCALE_1X);

    /* Enter low power mode */
    is31fl3235a_sw_shutdown(led, true);
}
```

## Repository Structure

```
IS31FL3235A_driver/
├── driver/
│   ├── is31fl3235a.c           # Main driver implementation
│   ├── is31fl3235a_regs.h      # Register definitions
│   ├── Kconfig.is31fl3235a     # Configuration options
│   ├── CMakeLists.txt          # Build integration
│   └── Kconfig                 # Kconfig integration
├── dts_bindings/
│   └── issi,is31fl3235a.yaml   # Device tree binding
├── include/
│   └── is31fl3235a.h           # Public API header
└── sample/
    ├── main.c                  # Sample application
    ├── app.overlay             # Device tree example
    └── prj.conf                # Sample configuration
```

## Configuration

```kconfig
CONFIG_LED_IS31FL3235A=y
CONFIG_I2C=y
CONFIG_LED=y
```

## Requirements

- Zephyr RTOS v3.0 or later
- I2C bus (100kHz or 400kHz)
- Optional: GPIO pin for SDB (hardware shutdown)

## Troubleshooting

### Device Not Ready

1. Verify I2C bus enabled in device tree
2. Check I2C address matches AD pin configuration
3. Ensure SDB pin is high (if used)
4. Verify power supply (2.7-5.5V)

### No LED Output

1. Check SDB pin is high (if used)
2. Verify not in shutdown mode
3. Check channel is enabled
4. Verify PWM value > 0

### I2C Errors

1. Verify I2C pull-ups (2.2k - 4.7k ohm)
2. Check I2C address matches AD pin
3. Verify I2C bus speed <= 400kHz

## References

- [IS31FL3235A Datasheet](https://www.lumissil.com/assets/pdf/core/IS31FL3235A_DS.pdf)
- [Zephyr LED Driver API](https://docs.zephyrproject.org/latest/hardware/peripherals/led.html)
- [Zephyr Device Tree Guide](https://docs.zephyrproject.org/latest/build/dts/index.html)

## License

```
SPDX-License-Identifier: Apache-2.0
```
