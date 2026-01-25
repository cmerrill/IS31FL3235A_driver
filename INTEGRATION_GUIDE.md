# IS31FL3235A Driver Integration Guide

## Overview

This guide explains how to integrate the IS31FL3235A driver into a Zephyr RTOS installation.

## Directory Structure

This repository contains the following files:

```
IS31FL3235A_driver/
├── driver/
│   ├── is31fl3235a.c           # Main driver implementation
│   ├── is31fl3235a_regs.h      # Register definitions (private)
│   ├── Kconfig.is31fl3235a     # Driver Kconfig
│   ├── CMakeLists.txt          # Build integration (reference)
│   └── Kconfig                 # Kconfig integration (reference)
├── dts_bindings/
│   └── issi,is31fl3235a.yaml   # Device tree binding
├── include/
│   └── is31fl3235a.h           # Public API header
├── sample/
│   ├── main.c                  # Sample application
│   ├── app.overlay             # Device tree overlay example
│   ├── prj.conf                # Sample configuration
│   └── README.md               # Sample documentation
└── [Planning documents...]
```

## Integration Steps

### Method 1: Direct Integration into Zephyr Tree

For contributing the driver to upstream Zephyr or using it in a local Zephyr installation.

#### 1. Copy Driver Files

```bash
# From the root of this repository
cd IS31FL3235A_driver

# Set ZEPHYR_BASE to your Zephyr installation
export ZEPHYR_BASE=~/zephyrproject/zephyr

# Copy driver implementation
cp driver/is31fl3235a.c $ZEPHYR_BASE/drivers/led/
cp driver/is31fl3235a_regs.h $ZEPHYR_BASE/drivers/led/
cp driver/Kconfig.is31fl3235a $ZEPHYR_BASE/drivers/led/

# Copy public API header
cp include/is31fl3235a.h $ZEPHYR_BASE/include/zephyr/drivers/led/

# Copy device tree binding
cp dts_bindings/issi,is31fl3235a.yaml $ZEPHYR_BASE/dts/bindings/led/
```

#### 2. Update Build Files

**Edit `$ZEPHYR_BASE/drivers/led/CMakeLists.txt`**

Add this line:

```cmake
zephyr_library_sources_ifdef(CONFIG_LED_IS31FL3235A is31fl3235a.c)
```

**Edit `$ZEPHYR_BASE/drivers/led/Kconfig`**

Add this line (typically near other LED driver sources):

```kconfig
source "drivers/led/Kconfig.is31fl3235a"
```

#### 3. Verify Integration

```bash
cd $ZEPHYR_BASE

# Check that the driver Kconfig is visible
west build -b <your_board> -t menuconfig samples/basic/blinky
# Navigate to: Device Drivers -> LED Drivers
# You should see "IS31FL3235A LED driver"

# Build a simple application to test compilation
west build -b <your_board> samples/basic/blinky
```

### Method 2: Out-of-Tree Module

For keeping the driver separate from the Zephyr tree (recommended for development/testing).

#### 1. Create Module Directory Structure

```bash
mkdir -p ~/zephyr-modules/is31fl3235a
cd ~/zephyr-modules/is31fl3235a

# Copy files
cp -r path/to/IS31FL3235A_driver/driver/* drivers/led/
cp -r path/to/IS31FL3235A_driver/dts_bindings/* dts/bindings/led/
cp -r path/to/IS31FL3235A_driver/include/* include/zephyr/drivers/led/
```

#### 2. Create Module Configuration

**Create `~/zephyr-modules/is31fl3235a/zephyr/module.yml`:**

```yaml
name: is31fl3235a
build:
  cmake: .
  kconfig: Kconfig
  settings:
    dts_root: .
```

**Create `~/zephyr-modules/is31fl3235a/CMakeLists.txt`:**

```cmake
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_LED_IS31FL3235A drivers/led/is31fl3235a.c)
zephyr_library_include_directories(include)
```

**Create `~/zephyr-modules/is31fl3235a/Kconfig`:**

```kconfig
source "drivers/led/Kconfig.is31fl3235a"
```

#### 3. Use the Module

```bash
# Set ZEPHYR_EXTRA_MODULES when building
export ZEPHYR_EXTRA_MODULES=~/zephyr-modules/is31fl3235a

# Or add to west.yml in your workspace
```

### Method 3: Application-Local Driver

For prototyping or application-specific driver.

#### 1. Create Driver Directory in Your Application

```bash
cd ~/my_zephyr_app
mkdir -p drivers/led
mkdir -p dts/bindings/led
```

#### 2. Copy Files

```bash
cp path/to/IS31FL3235A_driver/driver/is31fl3235a.c drivers/led/
cp path/to/IS31FL3235A_driver/driver/is31fl3235a_regs.h drivers/led/
cp path/to/IS31FL3235A_driver/driver/Kconfig.is31fl3235a drivers/led/
cp path/to/IS31FL3235A_driver/include/is31fl3235a.h drivers/led/
cp path/to/IS31FL3235A_driver/dts_bindings/issi,is31fl3235a.yaml dts/bindings/led/
```

#### 3. Update Application CMakeLists.txt

```cmake
# Add to your application's CMakeLists.txt
target_sources(app PRIVATE
    drivers/led/is31fl3235a.c
)

target_include_directories(app PRIVATE
    drivers/led
)

# Set DTS root to find the binding
set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
```

#### 4. Update Application Kconfig

```kconfig
# Create or edit Kconfig in your application root
source "drivers/led/Kconfig.is31fl3235a"
```

## Testing the Integration

### 1. Create Test Application

```bash
cd ~/zephyrproject
mkdir -p test_is31fl3235a
cd test_is31fl3235a

# Copy sample files
cp path/to/IS31FL3235A_driver/sample/* .
```

### 2. Create Device Tree Overlay

Edit `app.overlay` to match your hardware:

```c
#include <dt-bindings/led/led.h>

&i2c0 {  /* Change to your I2C bus */
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;

    led_controller: is31fl3235a@3c {
        compatible = "issi,is31fl3235a";
        reg = <0x3c>;
        sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;
        pwm-frequency = <22000>;

        led_0 {
            reg = <0>;
            label = "Test LED";
        };
    };
};
```

### 3. Build and Flash

```bash
west build -b <your_board> .
west flash
west attach
```

### 4. Verify Driver Loads

Look for these log messages:

```
[00:00:00.000,000] <inf> is31fl3235a: Initializing IS31FL3235A
[00:00:00.050,000] <inf> is31fl3235a: IS31FL3235A initialized successfully (PWM: 22kHz, I2C: 0x3c)
```

## Common Issues

### 1. Driver Not Found During Build

**Symptom:** `No rule to make target 'is31fl3235a.c'`

**Solutions:**
- Verify `is31fl3235a.c` is in `drivers/led/` directory
- Check `CMakeLists.txt` has the correct path
- Ensure `CONFIG_LED_IS31FL3235A=y` in `prj.conf`

### 2. Kconfig Option Not Visible

**Symptom:** `CONFIG_LED_IS31FL3235A` not found in menuconfig

**Solutions:**
- Verify `Kconfig.is31fl3235a` is in `drivers/led/`
- Check `drivers/led/Kconfig` sources the file
- Ensure `CONFIG_LED=y` is enabled (dependency)

### 3. Device Tree Binding Not Found

**Symptom:** `compatible 'issi,is31fl3235a' not found`

**Solutions:**
- Verify `issi,is31fl3235a.yaml` is in `dts/bindings/led/`
- Check `DTS_ROOT` is set correctly (for out-of-tree)
- Ensure binding file has `.yaml` extension (not `.yml`)

### 4. Header File Not Found

**Symptom:** `fatal error: zephyr/drivers/led/is31fl3235a.h: No such file or directory`

**Solutions:**
- Verify `is31fl3235a.h` is in `include/zephyr/drivers/led/`
- Check include directories are set correctly
- Ensure header path matches `#include` statement

### 5. Undefined References

**Symptom:** `undefined reference to 'is31fl3235a_set_current_scale'`

**Solutions:**
- Verify `is31fl3235a.c` is being compiled (`CONFIG_LED_IS31FL3235A=y`)
- Check that both `.c` file and header are from same version
- Ensure `CMakeLists.txt` includes the driver source

## Verification Checklist

After integration, verify:

- [ ] Driver compiles without errors
- [ ] Device tree binding is recognized
- [ ] Kconfig option appears in menuconfig
- [ ] Public API header is accessible
- [ ] Sample application builds
- [ ] Device initializes on target hardware
- [ ] Standard LED API works
- [ ] Extended API functions work

## File Locations Reference

| File | Location in Zephyr Tree |
|------|-------------------------|
| `is31fl3235a.c` | `drivers/led/` |
| `is31fl3235a_regs.h` | `drivers/led/` |
| `Kconfig.is31fl3235a` | `drivers/led/` |
| `is31fl3235a.h` | `include/zephyr/drivers/led/` |
| `issi,is31fl3235a.yaml` | `dts/bindings/led/` |

## Next Steps

After successful integration:

1. Test with your hardware
2. Adjust device tree overlay for your board
3. Try the sample application
4. Read the API documentation in `API_SPECIFICATION.md`
5. If contributing to Zephyr: follow [Contribution Guidelines](https://docs.zephyrproject.org/latest/contribute/index.html)

## Additional Resources

- [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) - Overall plan
- [DRIVER_ARCHITECTURE.md](DRIVER_ARCHITECTURE.md) - Architecture details
- [API_SPECIFICATION.md](API_SPECIFICATION.md) - Complete API documentation
- [DEVICE_TREE_BINDING.md](DEVICE_TREE_BINDING.md) - Device tree examples
- [REGISTER_DEFINITIONS.md](REGISTER_DEFINITIONS.md) - Register details
- [sample/README.md](sample/README.md) - Sample application guide

## Support

For issues or questions:

1. Check the planning documents in this repository
2. Review Zephyr's [LED driver documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/led.html)
3. Consult the [IS31FL3235A datasheet](https://www.lumissil.com/assets/pdf/core/IS31FL3235A_DS.pdf)
4. Ask on [Zephyr Discord](https://discord.gg/zephyr) (#drivers channel)
5. Post on [Zephyr GitHub Discussions](https://github.com/zephyrproject-rtos/zephyr/discussions)
