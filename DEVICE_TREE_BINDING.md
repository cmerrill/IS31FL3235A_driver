# IS31FL3235A Device Tree Binding Documentation

## Overview

This document describes the device tree binding for the IS31FL3235A 28-channel LED driver IC.

## Binding File Location

```
dts/bindings/led/issi,is31fl3235a.yaml
```

## Compatible String

```
compatible = "issi,is31fl3235a";
```

## Complete Binding Specification

```yaml
# Copyright (c) 2026
# SPDX-License-Identifier: Apache-2.0

description: |
  ISSI/Lumissil IS31FL3235A 28-channel LED driver with I2C interface.

  The IS31FL3235A is a constant current LED driver with 28 independent
  channels. Each channel supports:
  - 8-bit PWM brightness control (0-255)
  - Per-channel current scaling (1x, 1/2x, 1/3x, 1/4x)
  - Up to 38mA output current (set by external resistor)
  - Enable/disable control

  The device supports both hardware shutdown (via SDB pin) and software
  shutdown (via I2C register). PWM frequency is selectable between 3kHz
  and 22kHz.

  Example usage:

    &i2c0 {
        led_controller: is31fl3235a@3c {
            compatible = "issi,is31fl3235a";
            reg = <0x3c>;
            sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;
            pwm-frequency = <22000>;

            led_0 {
                reg = <0>;
                label = "Red LED";
                color = <LED_COLOR_ID_RED>;
            };

            led_1 {
                reg = <1>;
                label = "Green LED";
                color = <LED_COLOR_ID_GREEN>;
            };

            /* ... up to 28 channels ... */
        };
    };

compatible: "issi,is31fl3235a"

include: [i2c-device.yaml, base.yaml]

properties:
  sdb-gpios:
    type: phandle-array
    description: |
      Optional shutdown (SDB) GPIO pin specification.

      When this pin is driven low, the chip enters hardware shutdown mode
      (lowest power consumption). When driven high, the chip operates normally.

      If specified, the driver will configure this pin as an output and
      set it high during initialization to enable the chip.

      If not specified, only software shutdown (via I2C) is available.

      The GPIO should be configured as active-high.

  pwm-frequency:
    type: int
    default: 3000
    enum:
      - 3000
      - 22000
    description: |
      PWM frequency in Hz for all LED channels.

      Valid values:
        - 3000: 3kHz PWM frequency (default, lower EMI)
        - 22000: 22kHz PWM frequency (reduced flicker, faster response)

      The frequency applies to all 28 channels and is configured at
      initialization time. It cannot be changed at runtime.

      Choose 3kHz for lower electromagnetic interference and general purpose
      applications. Choose 22kHz for applications requiring reduced visible
      flicker or faster LED response times (e.g., high-speed scanning).

child-binding:
  description: |
    LED channel configuration.

    Each child node represents one of the 28 LED output channels.
    Channels are numbered 0-27, corresponding to hardware channels
    CH1-CH28 in the datasheet.

    Child nodes are optional. If not specified, all channels are still
    accessible via the LED driver API using channel numbers 0-27.

    The main purpose of child nodes is to assign human-readable labels
    and color information for use with Zephyr's LED abstraction layer.

  properties:
    reg:
      type: int
      required: true
      description: |
        LED channel number (0-27).

        Channel mapping to hardware:
          reg = 0  -> Hardware CH1  (PWM register 0x05, Control register 0x2A)
          reg = 1  -> Hardware CH2  (PWM register 0x06, Control register 0x2B)
          ...
          reg = 27 -> Hardware CH28 (PWM register 0x20, Control register 0x45)

        Valid range: 0-27

    label:
      type: string
      description: |
        Human-readable label for this LED channel.

        This label can be used with Zephyr's LED shell commands and APIs
        to reference the LED by name instead of channel number.

        Examples: "Red LED", "Status Indicator", "Backlight Zone 1"

    color:
      type: int
      description: |
        LED color identifier.

        Use LED_COLOR_ID_* constants from dt-bindings/led/led.h:
          - LED_COLOR_ID_WHITE
          - LED_COLOR_ID_RED
          - LED_COLOR_ID_GREEN
          - LED_COLOR_ID_BLUE
          - LED_COLOR_ID_AMBER
          - LED_COLOR_ID_VIOLET
          - LED_COLOR_ID_YELLOW
          - LED_COLOR_ID_IR
          - LED_COLOR_ID_RGB (for multi-color)

        This property is informational and used by LED framework for
        color-based LED selection and control.

    function:
      type: int
      description: |
        LED function identifier.

        Use LED_FUNCTION_* constants from dt-bindings/led/led.h:
          - LED_FUNCTION_STATUS
          - LED_FUNCTION_INDICATOR
          - LED_FUNCTION_BACKLIGHT
          - LED_FUNCTION_POWER
          - etc.

        This property is informational and used by LED framework for
        function-based LED selection and control.
```

## Property Details

### Required Properties

#### compatible
- **Type:** string
- **Value:** `"issi,is31fl3235a"`
- **Description:** Identifies the driver to bind to this device

#### reg
- **Type:** integer
- **Description:** I2C slave address
- **Valid values:** Typically `0x3C` to `0x3F` depending on AD pin strapping
- **Inherited from:** `i2c-device.yaml`

### Optional Properties

#### sdb-gpios
- **Type:** phandle-array (GPIO specification)
- **Description:** Optional hardware shutdown pin
- **Format:** `<&gpio_controller pin_number flags>`
- **Example:** `sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;`
- **Notes:**
  - If present, driver sets pin high during init (chip enabled)
  - If absent, only software shutdown is available
  - Must be active-high configuration

#### pwm-frequency
- **Type:** integer
- **Default:** `3000`
- **Valid values:** `3000` or `22000`
- **Description:** PWM frequency in Hz
- **Recommendations:**
  - Use 3kHz for general purpose (lower EMI)
  - Use 22kHz for reduced flicker or fast response

## Child Node Properties

### Required (for child nodes)

#### reg
- **Type:** integer
- **Range:** 0-27
- **Description:** Channel number (0=CH1, 27=CH28)

### Optional (for child nodes)

#### label
- **Type:** string
- **Example:** `"Status LED"`, `"RGB Red"`, `"Backlight 1"`

#### color
- **Type:** integer (enum)
- **Example:** `<LED_COLOR_ID_RED>`
- **Header:** `#include <dt-bindings/led/led.h>`

#### function
- **Type:** integer (enum)
- **Example:** `<LED_FUNCTION_STATUS>`
- **Header:** `#include <dt-bindings/led/led.h>`

## Complete Examples

### Example 1: Basic Configuration (No SDB, Default Frequency)

```dts
#include <dt-bindings/led/led.h>

&i2c0 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;  /* 400kHz recommended */

    led_controller: is31fl3235a@3c {
        compatible = "issi,is31fl3235a";
        reg = <0x3c>;
        /* No SDB pin - software shutdown only */
        /* pwm-frequency defaults to 3000 Hz */

        status_led: led_0 {
            reg = <0>;
            label = "Status LED";
            color = <LED_COLOR_ID_GREEN>;
        };
    };
};
```

### Example 2: Full Configuration with SDB and 22kHz PWM

```dts
#include <dt-bindings/led/led.h>
#include <dt-bindings/gpio/gpio.h>

&i2c1 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;

    led_driver: is31fl3235a@3d {
        compatible = "issi,is31fl3235a";
        reg = <0x3d>;
        sdb-gpios = <&gpio0 15 GPIO_ACTIVE_HIGH>;
        pwm-frequency = <22000>;  /* 22kHz for reduced flicker */

        /* Define all 28 channels if needed */
        led_red_0 {
            reg = <0>;
            label = "RGB Red 0";
            color = <LED_COLOR_ID_RED>;
        };

        led_green_0 {
            reg = <1>;
            label = "RGB Green 0";
            color = <LED_COLOR_ID_GREEN>;
        };

        led_blue_0 {
            reg = <2>;
            label = "RGB Blue 0";
            color = <LED_COLOR_ID_BLUE>;
        };

        /* Channels 3-26 omitted for brevity */

        led_status {
            reg = <27>;
            label = "Status Indicator";
            color = <LED_COLOR_ID_AMBER>;
            function = <LED_FUNCTION_STATUS>;
        };
    };
};
```

### Example 3: Multiple Instances on Same Bus

```dts
#include <dt-bindings/led/led.h>

&i2c0 {
    status = "okay";

    /* First controller at address 0x3C */
    led_controller_0: is31fl3235a@3c {
        compatible = "issi,is31fl3235a";
        reg = <0x3c>;
        sdb-gpios = <&gpio0 10 GPIO_ACTIVE_HIGH>;
        pwm-frequency = <3000>;

        led_matrix_0_0 {
            reg = <0>;
            label = "Matrix 0 LED 0";
        };
        /* ... more channels ... */
    };

    /* Second controller at address 0x3D */
    led_controller_1: is31fl3235a@3d {
        compatible = "issi,is31fl3235a";
        reg = <0x3d>;
        sdb-gpios = <&gpio0 11 GPIO_ACTIVE_HIGH>;
        pwm-frequency = <22000>;

        led_matrix_1_0 {
            reg = <0>;
            label = "Matrix 1 LED 0";
        };
        /* ... more channels ... */
    };
};
```

### Example 4: RGB LED Strip (3 LEDs × 3 colors = 9 channels)

```dts
#include <dt-bindings/led/led.h>

&i2c0 {
    led_strip: is31fl3235a@3c {
        compatible = "issi,is31fl3235a";
        reg = <0x3c>;
        pwm-frequency = <22000>;

        /* RGB LED 0 */
        rgb0_red {
            reg = <0>;
            label = "RGB0 Red";
            color = <LED_COLOR_ID_RED>;
        };
        rgb0_green {
            reg = <1>;
            label = "RGB0 Green";
            color = <LED_COLOR_ID_GREEN>;
        };
        rgb0_blue {
            reg = <2>;
            label = "RGB0 Blue";
            color = <LED_COLOR_ID_BLUE>;
        };

        /* RGB LED 1 */
        rgb1_red {
            reg = <3>;
            label = "RGB1 Red";
            color = <LED_COLOR_ID_RED>;
        };
        rgb1_green {
            reg = <4>;
            label = "RGB1 Green";
            color = <LED_COLOR_ID_GREEN>;
        };
        rgb1_blue {
            reg = <5>;
            label = "RGB1 Blue";
            color = <LED_COLOR_ID_BLUE>;
        };

        /* RGB LED 2 */
        rgb2_red {
            reg = <6>;
            label = "RGB2 Red";
            color = <LED_COLOR_ID_RED>;
        };
        rgb2_green {
            reg = <7>;
            label = "RGB2 Green";
            color = <LED_COLOR_ID_GREEN>;
        };
        rgb2_blue {
            reg = <8>;
            label = "RGB2 Blue";
            color = <LED_COLOR_ID_BLUE>;
        };

        /* Remaining 19 channels available for more LEDs */
    };
};
```

## Board Overlay Example

For testing with an existing board, create an overlay file:

**File: `boards/my_board.overlay`**

```dts
#include <dt-bindings/led/led.h>

&i2c0 {
    status = "okay";

    is31fl3235a@3c {
        compatible = "issi,is31fl3235a";
        reg = <0x3c>;
        sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;
        pwm-frequency = <3000>;

        test_led {
            reg = <0>;
            label = "Test LED";
            color = <LED_COLOR_ID_WHITE>;
        };
    };
};
```

## I2C Address Selection

The IS31FL3235A I2C address is determined by the AD pin:

| AD Pin | I2C Address | Device Tree `reg` |
|--------|-------------|-------------------|
| GND    | 0x78 >> 1   | `<0x3C>`         |
| VCC    | 0x7A >> 1   | `<0x3D>`         |
| SCL    | 0x7C >> 1   | `<0x3E>`         |
| SDA    | 0x7E >> 1   | `<0x3F>`         |

**Note:** Device tree uses 7-bit I2C addresses (right-shifted by 1 from datasheet values).

## Usage in Application Code

Once defined in device tree, access the device in your application:

```c
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/is31fl3235a.h>

/* Get device by node label */
const struct device *led_dev = DEVICE_DT_GET(DT_NODELABEL(led_controller));

if (!device_is_ready(led_dev)) {
    printk("LED device not ready\n");
    return;
}

/* Set brightness using standard LED API */
led_set_brightness(led_dev, 0, 128);  /* Channel 0, 50% brightness */

/* Use extended API for current scaling */
is31fl3235a_set_current_scale(led_dev, 0, IS31FL3235A_SCALE_1_2X);
```

## Validation Checklist

When creating device tree entries:

- [ ] Compatible string is exactly `"issi,is31fl3235a"`
- [ ] I2C address matches hardware AD pin configuration
- [ ] I2C bus reference is correct (`&i2c0`, `&i2c1`, etc.)
- [ ] I2C bus has `status = "okay";`
- [ ] SDB GPIO controller and pin number are correct (if used)
- [ ] SDB GPIO is configured as `GPIO_ACTIVE_HIGH`
- [ ] PWM frequency is either 3000 or 22000
- [ ] Child node `reg` values are in range 0-27
- [ ] No duplicate `reg` values within same controller
- [ ] Required headers are included (`dt-bindings/led/led.h`, etc.)

## Common Pitfalls

1. **Wrong I2C address width:** Device tree uses 7-bit addresses, not 8-bit
   - ✗ Wrong: `reg = <0x78>;`
   - ✓ Correct: `reg = <0x3c>;`

2. **SDB pin polarity:** Must be active-high
   - ✗ Wrong: `sdb-gpios = <&gpio0 12 GPIO_ACTIVE_LOW>;`
   - ✓ Correct: `sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;`

3. **Invalid PWM frequency:** Must be exactly 3000 or 22000
   - ✗ Wrong: `pwm-frequency = <3>;` (wrong unit)
   - ✓ Correct: `pwm-frequency = <3000>;`

4. **Channel out of range:** Only 0-27 valid
   - ✗ Wrong: `reg = <28>;` (no channel 28)
   - ✓ Correct: `reg = <27>;` (channel 28 = reg 27)

5. **Missing I2C bus status:** Bus must be enabled
   ```dts
   &i2c0 {
       status = "okay";  /* Don't forget this! */
       /* ... devices ... */
   };
   ```

## Testing Device Tree Configuration

Use these Zephyr shell commands to verify device tree configuration:

```shell
# List all LED devices
led list

# Get device info
device list

# Check if device is ready
device get is31fl3235a@3c
```

## References

- Zephyr Device Tree Bindings Guide: https://docs.zephyrproject.org/latest/build/dts/bindings.html
- Zephyr I2C Device Binding: `dts/bindings/i2c/i2c-device.yaml`
- LED Bindings Header: `include/zephyr/dt-bindings/led/led.h`
- IS31FL3235A Datasheet: Lumissil Microsystems
