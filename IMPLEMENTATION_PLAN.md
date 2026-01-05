# IS31FL3235A Zephyr RTOS Driver - Implementation Plan

## Overview

This document outlines the implementation plan for a Zephyr RTOS driver for the IS31FL3235A 28-channel LED driver IC from Lumissil Microsystems (ISSI).

## IC Specifications Summary

- **Manufacturer:** Lumissil Microsystems / ISSI
- **Channels:** 28 constant current LED channels
- **Output Current:** Up to 38mA per channel (set by external resistor)
- **Current Scaling:** Per-channel scaling: 1×, 1/2×, 1/3×, 1/4×
- **PWM Resolution:** 8-bit (256 steps)
- **PWM Frequency:** 3kHz (default) or 22kHz (selectable)
- **Interface:** I2C
- **Supply Voltage:** 2.7V to 5.5V
- **Shutdown:** Hardware (SDB pin) and software shutdown
- **Package:** QFN-36 (4mm × 4mm)

## Key Differences from IS31FL3216A

| Feature | IS31FL3216A | IS31FL3235A |
|---------|-------------|-------------|
| Channels | 16 | 28 |
| PWM Frequency | 26kHz | 3kHz/22kHz selectable |
| GPIO Support | Yes (8 channels) | No |
| Audio Modulation | Yes | No |
| Current Scaling | No | Yes (1×, 1/2×, 1/3×, 1/4×) |
| Frame Storage | Yes (8 frames) | No |

## Register Map

Based on datasheet research:

| Address | Name | Description |
|---------|------|-------------|
| 0x00 | Shutdown Register | Software shutdown control (SSD bit) |
| 0x05-0x20 | PWM Registers | 28 channels PWM duty cycle (CH1-CH28) |
| 0x25 | Update Register | Write 0x00 to apply buffered changes |
| 0x2A-0x45 | LED Control Registers | 28 channels enable + current scaling |
| 0x4B | Output Frequency Setting | PWM frequency: 0=3kHz, 1=22kHz |
| 0x4F | Reset Register | Reset all registers to default |

### LED Control Register Format (0x2A-0x45)

Each LED Control Register contains:
- **Enable bit:** Turn channel on/off
- **Current scaling bits:** Set to 1×, 1/2×, 1/3×, or 1/4× of maximum current

### Update Mechanism

The IS31FL3235A uses a buffered update mechanism:
1. Write to PWM registers (0x05-0x20) - values stored in temporary registers
2. Write to LED Control registers (0x2A-0x45) - values stored in temporary registers
3. Write 0x00 to Update Register (0x25) - all changes applied simultaneously

This allows synchronized multi-channel updates.

## Requirements from User

1. **Standard API:** Implement Zephyr LED API for all 28 channels
2. **Current Scaling:** Runtime API for per-channel current scaling (1×, 1/2×, 1/3×, 1/4×)
3. **PWM Frequency:** Device tree configuration (3kHz or 22kHz)
4. **Hardware Current:** Maximum current set by hardware resistor; scaling via API
5. **Gamma Correction:** Not implemented (user recommendation only)
6. **SDB Pin:** Optional in device tree; enable on init + runtime control API
7. **Shutdown Control:** APIs for both hardware (SDB) and software shutdown
8. **I2C Address:** Defined in device tree; support multiple instances
9. **Synchronized Updates:** Support writing multiple channels before update
10. **Sample/Tests:** Not required initially

## Driver Architecture

### File Structure

```
drivers/led/
├── is31fl3235a.c          # Main driver implementation
└── is31fl3235a.h          # Private header (register definitions)

dts/bindings/led/
└── issi,is31fl3235a.yaml  # Device tree binding

include/zephyr/drivers/led/
└── is31fl3235a.h          # Public API header (optional, if extended API needed)
```

### Data Structures

#### Configuration Structure (const, in ROM)
```c
struct is31fl3235a_cfg {
    struct i2c_dt_spec i2c;           /* I2C bus and address */
    struct gpio_dt_spec sdb_gpio;     /* Optional SDB pin */
    bool pwm_freq_22khz;              /* PWM frequency: false=3kHz, true=22kHz */
};
```

#### Runtime Data Structure (RAM)
```c
struct is31fl3235a_data {
    struct k_mutex lock;              /* Protect concurrent access */
    bool initialized;                 /* Init complete flag */
    bool sw_shutdown;                 /* Software shutdown state */
    uint8_t pwm_cache[28];           /* Cache of PWM values */
    uint8_t ctrl_cache[28];          /* Cache of LED control values */
};
```

### Register Definitions

```c
/* Register addresses */
#define IS31FL3235A_REG_SHUTDOWN     0x00
#define IS31FL3235A_REG_PWM_BASE     0x05  /* CH1 at 0x05, CH28 at 0x20 */
#define IS31FL3235A_REG_UPDATE       0x25
#define IS31FL3235A_REG_CTRL_BASE    0x2A  /* CH1 at 0x2A, CH28 at 0x45 */
#define IS31FL3235A_REG_FREQ         0x4B
#define IS31FL3235A_REG_RESET        0x4F

/* Shutdown register bits */
#define IS31FL3235A_SHUTDOWN_SSD_BIT  BIT(0)  /* 0=shutdown, 1=normal */

/* LED Control register bits */
#define IS31FL3235A_CTRL_ENABLE_BIT   BIT(?)  /* Enable channel (TBD from datasheet) */
#define IS31FL3235A_CTRL_SCALE_MASK   (0x??)  /* Current scaling bits (TBD) */
#define IS31FL3235A_CTRL_SCALE_1X     (0x??)
#define IS31FL3235A_CTRL_SCALE_HALF   (0x??)
#define IS31FL3235A_CTRL_SCALE_THIRD  (0x??)
#define IS31FL3235A_CTRL_SCALE_QUARTER (0x??)

/* Frequency register */
#define IS31FL3235A_FREQ_3KHZ         0x00
#define IS31FL3235A_FREQ_22KHZ        0x01

/* Number of channels */
#define IS31FL3235A_NUM_CHANNELS      28
```

**Note:** Exact bit positions for LED Control register need to be verified from datasheet.

## API Design

### Standard Zephyr LED API

Implement the standard LED driver API from `include/zephyr/drivers/led.h`:

```c
static const struct led_driver_api is31fl3235a_led_api = {
    .set_brightness = is31fl3235a_led_set_brightness,
    .write_channels = is31fl3235a_led_write_channels,
    /* on/off not directly supported, use brightness 0/max */
};
```

#### led_set_brightness()
Set brightness for a single LED channel (0-255).

#### led_write_channels()
Write brightness values to multiple channels simultaneously, then trigger update.

### Extended API (IS31FL3235A-specific)

Define extended functions for features not in standard LED API:

```c
/**
 * @brief Set current scaling for a channel
 * @param dev Pointer to device structure
 * @param channel Channel number (0-27)
 * @param scale Current scaling factor (0=1x, 1=1/2x, 2=1/3x, 3=1/4x)
 * @return 0 on success, negative errno otherwise
 */
int is31fl3235a_set_current_scale(const struct device *dev,
                                   uint8_t channel,
                                   uint8_t scale);

/**
 * @brief Enable or disable a channel
 * @param dev Pointer to device structure
 * @param channel Channel number (0-27)
 * @param enable true to enable, false to disable
 * @return 0 on success, negative errno otherwise
 */
int is31fl3235a_channel_enable(const struct device *dev,
                                uint8_t channel,
                                bool enable);

/**
 * @brief Software shutdown control
 * @param dev Pointer to device structure
 * @param shutdown true to shutdown, false to wake
 * @return 0 on success, negative errno otherwise
 */
int is31fl3235a_sw_shutdown(const struct device *dev, bool shutdown);

/**
 * @brief Hardware shutdown control via SDB pin
 * @param dev Pointer to device structure
 * @param shutdown true to shutdown (SDB low), false to wake (SDB high)
 * @return 0 on success, negative errno otherwise
 * @return -ENOTSUP if SDB pin not configured
 */
int is31fl3235a_hw_shutdown(const struct device *dev, bool shutdown);

/**
 * @brief Trigger update of buffered PWM and control register values
 * @param dev Pointer to device structure
 * @return 0 on success, negative errno otherwise
 */
int is31fl3235a_update(const struct device *dev);
```

### Power Management Integration

Optionally implement Zephyr PM subsystem callbacks:
- `PM_DEVICE_ACTION_SUSPEND`: Enter software shutdown
- `PM_DEVICE_ACTION_RESUME`: Wake from shutdown

## Device Tree Binding Specification

### File: dts/bindings/led/issi,is31fl3235a.yaml

```yaml
description: |
  ISSI/Lumissil IS31FL3235A 28-channel LED driver with I2C interface.

  Each channel can drive up to 38mA (set by external resistor) with
  8-bit PWM brightness control and per-channel current scaling.

compatible: "issi,is31fl3235a"

include: i2c-device.yaml

properties:
  sdb-gpios:
    type: phandle-array
    description: |
      Optional SDB (shutdown) pin. When low, chip enters hardware shutdown.
      If specified, driver will set high during initialization to enable chip.

  pwm-frequency:
    type: int
    default: 3000
    enum:
      - 3000
      - 22000
    description: |
      PWM frequency in Hz. Default is 3kHz.
      Valid values: 3000 (3kHz) or 22000 (22kHz).

child-binding:
  description: |
    LED channels. Use standard LED binding properties.
    Channels are numbered 0-27 corresponding to hardware CH1-CH28.

  properties:
    reg:
      type: int
      required: true
      description: |
        Channel number (0-27 for CH1-CH28)

    label:
      type: string
      description: |
        Human-readable label for this LED channel

    color:
      type: int
      description: |
        LED color (use LED_COLOR_ID_* from dt-bindings/led/led.h)
```

### Example Device Tree Usage

```dts
&i2c0 {
    led_controller: is31fl3235a@3c {
        compatible = "issi,is31fl3235a";
        reg = <0x3c>;
        sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;
        pwm-frequency = <22000>;  /* 22kHz */

        led_red_0 {
            reg = <0>;  /* CH1 */
            label = "Red LED 0";
            color = <LED_COLOR_ID_RED>;
        };

        led_green_0 {
            reg = <1>;  /* CH2 */
            label = "Green LED 0";
            color = <LED_COLOR_ID_GREEN>;
        };

        led_blue_0 {
            reg = <2>;  /* CH3 */
            label = "Blue LED 0";
            color = <LED_COLOR_ID_BLUE>;
        };

        /* ... define other channels as needed ... */
    };
};
```

### Multiple Instance Example

```dts
&i2c0 {
    led_controller_0: is31fl3235a@3c {
        compatible = "issi,is31fl3235a";
        reg = <0x3c>;
        sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;
        pwm-frequency = <3000>;
    };

    led_controller_1: is31fl3235a@3d {
        compatible = "issi,is31fl3235a";
        reg = <0x3d>;
        /* No SDB pin - software shutdown only */
        pwm-frequency = <22000>;
    };
};
```

## Implementation Steps

### Phase 1: Core Driver Structure

1. **Create register definitions header** (`is31fl3235a.h`)
   - Define all register addresses
   - Define bit masks and values
   - Define constants (number of channels, etc.)

2. **Create device tree binding** (`issi,is31fl3235a.yaml`)
   - Define properties (I2C, SDB pin, PWM frequency)
   - Define child binding for LED channels
   - Add documentation

3. **Create driver skeleton** (`is31fl3235a.c`)
   - Define config and data structures
   - Implement I2C helper functions
   - Implement device initialization

### Phase 2: Standard LED API Implementation

4. **Implement init function**
   - Verify I2C bus ready
   - Configure SDB pin if present (set high)
   - Reset chip using reset register
   - Configure PWM frequency from device tree
   - Initialize all channels to off
   - Trigger update

5. **Implement led_set_brightness()**
   - Validate channel number (0-27)
   - Write PWM value to register
   - Trigger update (write to update register)

6. **Implement led_write_channels()**
   - Validate channel numbers
   - Write multiple PWM values
   - Trigger single update at end

### Phase 3: Extended API

7. **Implement current scaling API**
   - Read-modify-write LED control register
   - Support 1×, 1/2×, 1/3×, 1/4× scaling
   - Trigger update

8. **Implement channel enable/disable**
   - Set/clear enable bit in LED control register
   - Trigger update

9. **Implement shutdown APIs**
   - Software shutdown: write to shutdown register
   - Hardware shutdown: control SDB GPIO pin
   - Cache shutdown state

### Phase 4: Advanced Features

10. **Implement synchronized updates**
    - Cache PWM and control values
    - Separate write and update functions
    - Allow multiple writes before update

11. **Add power management support** (optional)
    - PM suspend → software shutdown
    - PM resume → wake from shutdown

12. **Add logging and error handling**
    - Log init messages
    - Log I2C errors
    - Validate parameters

### Phase 5: Kconfig and Build Integration

13. **Create Kconfig entries**
    - Add `CONFIG_LED_IS31FL3235A`
    - Add dependencies (I2C, LED subsystem)

14. **Update CMakeLists.txt**
    - Add driver source to build

15. **Add MAINTAINERS entry** (optional)

## Testing Strategy (Future)

While not required initially, consider these test scenarios:

1. **Basic functionality**
   - Set individual channel brightness
   - Set all channels to different values
   - Test brightness range (0-255)

2. **Current scaling**
   - Test all scaling factors
   - Verify current measurements

3. **Shutdown**
   - Software shutdown/wake
   - Hardware shutdown/wake (if SDB present)
   - Verify power consumption in shutdown

4. **Synchronized updates**
   - Write multiple channels
   - Verify simultaneous update

5. **Multiple instances**
   - Test two controllers on same I2C bus
   - Different configurations per instance

6. **Error conditions**
   - Invalid channel numbers
   - I2C bus errors
   - Missing SDB pin when API called

## Open Questions / TBD

1. **LED Control Register bit positions** - Need exact bit layout from datasheet:
   - Which bit(s) control enable?
   - Which bits control current scaling (1×, 1/2×, 1/3×, 1/4×)?
   - Are there any reserved bits?

2. **Reset behavior** - Does the reset register need a specific value written, or any write?

3. **I2C timing** - Any specific delays required after reset or shutdown commands?

4. **Child node handling** - Should driver enumerate child LED nodes and track labels?

5. **Channel naming** - Should we use 0-27 (C-style) or 1-28 (datasheet style) in API?
   - **Recommendation:** Use 0-27 internally (C arrays), document as CH1-CH28

## Reference Documents

- IS31FL3235A Datasheet (Lumissil Microsystems)
- Zephyr LED Driver API: `include/zephyr/drivers/led.h`
- Zephyr Device Tree Guide
- IS31FL3216A driver: `drivers/led/is31fl3216a.c`
- I2C Device Driver Guide: Zephyr documentation

## Timeline Considerations

Implementation can be done incrementally:
- **Phase 1-2:** Basic working driver with standard LED API (~1-2 days)
- **Phase 3:** Extended features (~1 day)
- **Phase 4:** Polish and advanced features (~1 day)
- **Testing:** Ongoing with hardware

Total estimated effort: 3-5 days with hardware available for testing.

## Success Criteria

Driver is considered complete when:
- ✓ Compiles without errors/warnings
- ✓ Passes Zephyr coding style checks
- ✓ Device tree binding is valid
- ✓ Standard LED API works (set brightness, write channels)
- ✓ Extended API works (scaling, shutdown)
- ✓ Multiple instances supported
- ✓ SDB pin handling works (when present)
- ✓ All 28 channels controllable
- ✓ Synchronized updates work correctly
- ✓ Proper error handling and logging
