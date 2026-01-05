.. _led_is31fl3235a:

IS31FL3235A LED Driver Sample
#############################

Overview
********

This sample demonstrates how to use the IS31FL3235A LED driver to control
up to 28 independent LED channels. The IS31FL3235A is a constant current
LED driver from Lumissil Microsystems (ISSI) with I2C interface.

The sample performs the following demonstrations:

1. Basic brightness control on individual channels
2. Synchronized RGB LED updates
3. Current scaling demonstration (1×, ½×, ⅓×, ¼×)
4. Breathing effect animation
5. Channel enable/disable functionality
6. Power management with software shutdown

Building and Running
********************

This sample can be built and run on any board with I2C support. The IS31FL3235A
chip must be connected to the I2C bus.

Hardware Setup
==============

Connect the IS31FL3235A to your board:

* **I2C Bus**: Connect SDA and SCL to the board's I2C pins
* **Power**: Connect VCC (2.7V-5.5V) and GND
* **Address**: Connect AD pin to GND, VCC, SCL, or SDA to set I2C address
* **LEDs**: Connect LEDs to OUT1-OUT28 outputs
* **REXT**: Connect external resistor to set maximum current
* **SDB** (optional): Connect to GPIO for hardware shutdown

I2C Address Selection
---------------------

The I2C address is determined by the AD pin connection:

.. list-table::
   :header-rows: 1

   * - AD Pin
     - I2C Address
     - Device Tree ``reg``
   * - GND
     - 0x78 >> 1
     - ``<0x3C>``
   * - VCC
     - 0x7A >> 1
     - ``<0x3D>``
   * - SCL
     - 0x7C >> 1
     - ``<0x3E>``
   * - SDA
     - 0x7E >> 1
     - ``<0x3F>``

Device Tree Overlay
===================

The sample requires a device tree overlay to configure the IS31FL3235A.
Create a board-specific overlay file or edit ``app.overlay``:

.. code-block:: devicetree

   #include <dt-bindings/led/led.h>

   &i2c0 {
       status = "okay";
       clock-frequency = <I2C_BITRATE_FAST>;  /* 400kHz recommended */

       led_controller: is31fl3235a@3c {
           compatible = "issi,is31fl3235a";
           reg = <0x3c>;  /* I2C address (AD pin = GND) */
           sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;  /* Optional */
           pwm-frequency = <22000>;  /* 3000 or 22000 Hz */

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
       };
   };

Building
========

Build the sample for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led_is31fl3235a
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:

Sample Output
=============

The sample will output the following to the console:

.. code-block:: console

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
   [00:00:27.054,000] <inf> main: Test 6: Testing software shutdown
   [00:00:31.054,000] <inf> main: Sample complete

During execution, you should observe:

* LEDs turning on and off individually
* RGB color changes (red, green, blue, yellow, magenta, cyan, white)
* Brightness changes as current scaling is adjusted
* Smooth breathing effect on one LED
* LED turning off when disabled, back on when enabled
* All LEDs turning off during shutdown, back on when waking

Troubleshooting
***************

Device Not Ready
================

If you see ``LED device not ready``:

* Verify I2C bus is enabled in device tree
* Check I2C address matches AD pin configuration
* Ensure I2C pull-up resistors are present (2.2kΩ - 4.7kΩ)
* Verify power supply is correct (2.7V - 5.5V)
* If using SDB pin, ensure it's high during initialization

No LED Output
=============

If the device initializes but LEDs don't turn on:

* Check LEDs are connected with correct polarity
* Verify REXT resistor is connected (sets maximum current)
* Ensure chip is not in shutdown mode
* Check channel enable bits are set
* Verify PWM values are greater than 0

I2C Communication Errors
========================

If you see I2C errors:

* Verify I2C bus speed is ≤ 400kHz
* Check SDA and SCL connections
* Ensure pull-up resistors are present
* Check for bus contention (multiple devices at same address)
* Try reducing I2C speed to 100kHz for testing

API Usage
*********

The sample demonstrates various API functions:

Basic Brightness Control
=========================

.. code-block:: c

   #include <zephyr/drivers/led.h>

   const struct device *led = DEVICE_DT_GET(DT_NODELABEL(led_controller));

   /* Set channel 0 to 50% brightness */
   led_set_brightness(led, 0, 128);

   /* Turn on channel 5 fully */
   led_set_brightness(led, 5, 255);

   /* Turn off channel 10 */
   led_set_brightness(led, 10, 0);

Multi-Channel Updates
=====================

.. code-block:: c

   /* Set RGB LED to purple */
   uint8_t rgb[3] = {255, 0, 255};
   led_write_channels(led, 0, 3, rgb);

Current Scaling
===============

.. code-block:: c

   #include <zephyr/drivers/led/is31fl3235a.h>

   /* Reduce channel 0 current to 50% */
   is31fl3235a_set_current_scale(led, 0, IS31FL3235A_SCALE_1_2X);

   /* Balance RGB LED currents */
   is31fl3235a_set_current_scale(led, 0, IS31FL3235A_SCALE_1_4X);  /* Red */
   is31fl3235a_set_current_scale(led, 1, IS31FL3235A_SCALE_1_2X);  /* Green */
   is31fl3235a_set_current_scale(led, 2, IS31FL3235A_SCALE_1X);    /* Blue */

Power Management
================

.. code-block:: c

   /* Enter software shutdown (low power) */
   is31fl3235a_sw_shutdown(led, true);

   /* Wake from shutdown */
   is31fl3235a_sw_shutdown(led, false);

   /* Hardware shutdown via SDB pin (if configured) */
   is31fl3235a_hw_shutdown(led, true);

References
**********

* `IS31FL3235A Datasheet <https://www.lumissil.com/assets/pdf/core/IS31FL3235A_DS.pdf>`_
* :ref:`LED driver API <led_api>`
