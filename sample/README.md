# IS31FL3235A LED Driver Sample Application

## Overview

This sample application demonstrates the basic functionality of the IS31FL3235A LED driver for Zephyr RTOS.

## Features Demonstrated

1. **Basic Brightness Control** - Setting individual channel brightness (0-255)
2. **Multi-Channel Updates** - Synchronized RGB LED control
3. **Current Scaling** - Testing all 4 current scaling factors (1×, ½×, ⅓×, ¼×)
4. **Breathing Effect** - Smooth fade in/out animation
5. **Channel Enable/Disable** - Turning channels on/off without changing brightness
6. **Software Shutdown** - Low power mode demonstration

## Hardware Requirements

- Board with I2C bus support
- IS31FL3235A chip or evaluation board
- At least 3 LEDs connected to channels 0-2 (for RGB testing)
- Optional: GPIO pin connected to SDB (shutdown) pin

## Wiring

### I2C Connection

| IS31FL3235A Pin | Board Connection |
|-----------------|------------------|
| SDA | I2C SDA |
| SCL | I2C SCL |
| VCC | 2.7V - 5.5V |
| GND | GND |
| AD | GND/VCC/SCL/SDA (sets I2C address) |

### I2C Address Selection

The I2C address is determined by the AD pin:

| AD Pin | I2C Address | `reg` Value in DT |
|--------|-------------|-------------------|
| GND | 0x78 >> 1 | `<0x3C>` |
| VCC | 0x7A >> 1 | `<0x3D>` |
| SCL | 0x7C >> 1 | `<0x3E>` |
| SDA | 0x7E >> 1 | `<0x3F>` |

### Optional SDB Pin

Connect a GPIO pin to the SDB pin for hardware shutdown support. Update `sdb-gpios` in the device tree overlay.

### LED Connections

Connect LEDs to OUT1-OUT3 (at minimum) for testing. Use an external resistor (REXT) to set the maximum current.

## Building and Running

### 1. Set up your environment

```bash
# Navigate to Zephyr workspace
cd ~/zephyrproject/zephyr

# Set up environment
source zephyr-env.sh
```

### 2. Customize the device tree overlay

Edit `app.overlay` to match your hardware:

```c
&i2c0 {  /* Change to your I2C bus */
    is31fl3235a@3c {
        reg = <0x3c>;  /* Change based on AD pin */
        sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;  /* Change to your GPIO */
        pwm-frequency = <22000>;  /* 3000 or 22000 */
    };
};
```

### 3. Build the application

```bash
# For your specific board
west build -b <your_board> path/to/sample

# Example for nrf52840dk_nrf52840
west build -b nrf52840dk_nrf52840 path/to/sample
```

### 4. Flash and run

```bash
west flash

# Monitor output
west attach
```

## Expected Output

```
*** Booting Zephyr OS build vX.X.X ***
[00:00:00.000,000] <inf> main: IS31FL3235A LED Driver Sample
[00:00:00.010,000] <inf> is31fl3235a: Initializing IS31FL3235A
[00:00:00.050,000] <inf> is31fl3235a: IS31FL3235A initialized successfully (PWM: 22kHz, I2C: 0x3c)
[00:00:00.051,000] <inf> main: LED device is31fl3235a@3c ready
[00:00:00.052,000] <inf> main: Test 1: Setting individual channel brightness
[00:00:01.552,000] <inf> main: Test 2: Synchronized multi-channel update (RGB)
[00:00:04.052,000] <inf> main: Test 3: Testing current scaling
[00:00:04.053,000] <inf> main: Setting current scale to 1x
[00:00:05.053,000] <inf> main: Setting current scale to 1/2x
[00:00:06.053,000] <inf> main: Setting current scale to 1/3x
[00:00:07.053,000] <inf> main: Setting current scale to 1/4x
[00:00:08.553,000] <inf> main: Test 4: Breathing effect on channel 0
[00:00:24.053,000] <inf> main: Test 5: Testing channel enable/disable
[00:00:24.054,000] <inf> main: Disabling channel 0 (LED should turn off)
[00:00:25.054,000] <inf> main: Enabling channel 0 (LED should turn on)
[00:00:27.054,000] <inf> main: Test 6: Testing software shutdown
[00:00:28.054,000] <inf> main: Entering software shutdown
[00:00:30.054,000] <inf> main: Waking from software shutdown
[00:00:31.054,000] <inf> main: Sample complete
```

## Troubleshooting

### Device Not Ready

**Error:** `LED device not ready`

**Solutions:**
- Check I2C bus is enabled in device tree
- Verify I2C address matches AD pin configuration
- Ensure power supply is correct (2.7V - 5.5V)
- If using SDB pin, verify it's connected and configured correctly
- Check I2C pull-up resistors are present (2.2kΩ - 4.7kΩ)

### No LED Output

**Observations:** Device initializes but LEDs don't light up

**Solutions:**
- Verify LEDs are connected correctly (anode to OUTx, cathode to GND)
- Check REXT resistor is connected (sets maximum current)
- Ensure chip is not in shutdown mode
- Use `CONFIG_LED_LOG_LEVEL_DBG=y` to see detailed debug output
- Verify channel is enabled (not disabled)

### I2C Communication Errors

**Error:** I2C read/write failures

**Solutions:**
- Check I2C bus speed (should be ≤ 400kHz)
- Verify SDA and SCL connections
- Ensure pull-up resistors are present and correct value
- Check for bus contention (multiple devices on same address)
- Try reducing I2C speed to 100kHz for testing

### Incorrect I2C Address

**Error:** No I2C ACK from device

**Solutions:**
- Use `i2cdetect` to scan for devices
- Verify AD pin configuration
- Remember: device tree uses 7-bit addresses (shift datasheet addresses right by 1)

## API Usage Examples

### Basic Brightness Control

```c
const struct device *led = DEVICE_DT_GET(DT_NODELABEL(led_controller));

/* Set channel 0 to 50% brightness */
led_set_brightness(led, 0, 128);

/* Turn channel 5 fully on */
led_set_brightness(led, 5, 255);

/* Turn channel 10 off */
led_set_brightness(led, 10, 0);
```

### RGB LED Control

```c
/* Set RGB LED to purple (R=255, G=0, B=255) */
uint8_t rgb[3] = {255, 0, 255};
led_write_channels(led, 0, 3, rgb);
```

### Current Scaling

```c
/* Balance RGB LED brightness */
is31fl3235a_set_current_scale(led, 0, IS31FL3235A_SCALE_1_4X);  /* Red: 25% */
is31fl3235a_set_current_scale(led, 1, IS31FL3235A_SCALE_1_2X);  /* Green: 50% */
is31fl3235a_set_current_scale(led, 2, IS31FL3235A_SCALE_1X);    /* Blue: 100% */
```

### Power Management

```c
/* Enter low power mode */
is31fl3235a_sw_shutdown(led, true);

/* Wake from low power */
is31fl3235a_sw_shutdown(led, false);

/* Hardware shutdown (if SDB configured) */
is31fl3235a_hw_shutdown(led, true);
```

## Performance Notes

- **I2C Speed:** Use 400kHz (Fast Mode) for best performance
- **Update Time:** ~1ms per register write at 400kHz
- **Multi-Channel:** Use `led_write_channels()` for better performance
- **PWM Frequency:** 22kHz reduces visible flicker but slightly higher EMI

## References

- [IS31FL3235A Datasheet](https://www.lumissil.com/assets/pdf/core/IS31FL3235A_DS.pdf)
- [Zephyr LED Driver Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/led.html)
- [Driver Implementation Plan](../IMPLEMENTATION_PLAN.md)
- [API Specification](../API_SPECIFICATION.md)
