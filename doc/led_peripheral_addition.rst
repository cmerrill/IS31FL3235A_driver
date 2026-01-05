.. _led_api_is31fl3235a:

IS31FL3235A
***********

The IS31FL3235A is a 28-channel constant current LED driver from Lumissil
Microsystems (a division of ISSI) with I2C interface. It provides independent
8-bit PWM brightness control for each channel along with per-channel current
scaling.

Hardware Features
=================

* 28 independent constant current output channels
* 8-bit PWM brightness control per channel (256 levels)
* Per-channel current scaling (1×, ½×, ⅓×, ¼×)
* Selectable PWM frequency (3kHz or 22kHz)
* Up to 38mA output current per channel (set by external resistor)
* Hardware shutdown via SDB pin (optional)
* Software shutdown for power management
* Buffered register updates for synchronized multi-channel changes
* I2C interface (up to 400kHz Fast Mode)
* Supply voltage: 2.7V - 5.5V
* Operating temperature: -40°C to +85°C

The driver implements the standard Zephyr LED API for basic operations and
provides an extended API for accessing IS31FL3235A-specific features.

Configuration
=============

The IS31FL3235A is configured via device tree. The I2C address is determined
by the hardware AD pin connection:

.. code-block:: devicetree

   &i2c0 {
       status = "okay";
       clock-frequency = <I2C_BITRATE_FAST>;

       led_controller: is31fl3235a@3c {
           compatible = "issi,is31fl3235a";
           reg = <0x3c>;                              /* I2C address */
           sdb-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;  /* Optional shutdown pin */
           pwm-frequency = <22000>;                   /* 3000 or 22000 Hz */

           /* Optional: Define LED channels */
           led_0 {
               reg = <0>;
               label = "Status LED";
               color = <LED_COLOR_ID_GREEN>;
           };
       };
   };

Device Tree Properties
----------------------

.. list-table::
   :header-rows: 1

   * - Property
     - Type
     - Required
     - Description
   * - ``compatible``
     - string
     - Yes
     - Must be ``"issi,is31fl3235a"``
   * - ``reg``
     - int
     - Yes
     - I2C address (0x3C-0x3F based on AD pin)
   * - ``sdb-gpios``
     - phandle-array
     - No
     - GPIO for hardware shutdown (active-high)
   * - ``pwm-frequency``
     - int
     - No
     - PWM frequency: 3000 (default) or 22000 Hz

Basic Usage
===========

The driver supports the standard Zephyr LED API:

.. code-block:: c

   #include <zephyr/drivers/led.h>

   const struct device *led_dev = DEVICE_DT_GET(DT_NODELABEL(led_controller));

   /* Set channel 0 to 50% brightness */
   led_set_brightness(led_dev, 0, 128);

   /* Update multiple channels simultaneously */
   uint8_t rgb_values[3] = {255, 0, 255};  /* Purple */
   led_write_channels(led_dev, 0, 3, rgb_values);

Extended Features
=================

The IS31FL3235A provides additional features beyond the standard LED API:

Current Scaling
---------------

Each channel's output current can be scaled independently to balance brightness
across different LED types or reduce power consumption:

.. code-block:: c

   #include <zephyr/drivers/led/is31fl3235a.h>

   /* Reduce channel current to 50% of maximum */
   is31fl3235a_set_current_scale(led_dev, 0, IS31FL3235A_SCALE_1_2X);

Available scaling factors:

* ``IS31FL3235A_SCALE_1X`` - 100% of maximum current (default)
* ``IS31FL3235A_SCALE_1_2X`` - 50% of maximum current
* ``IS31FL3235A_SCALE_1_3X`` - 33% of maximum current
* ``IS31FL3235A_SCALE_1_4X`` - 25% of maximum current

This is particularly useful for balancing RGB LEDs:

.. code-block:: c

   /* Balance RGB LED to achieve white color */
   is31fl3235a_set_current_scale(led_dev, 0, IS31FL3235A_SCALE_1_4X);  /* Red */
   is31fl3235a_set_current_scale(led_dev, 1, IS31FL3235A_SCALE_1_2X);  /* Green */
   is31fl3235a_set_current_scale(led_dev, 2, IS31FL3235A_SCALE_1X);    /* Blue */

   /* Now all channels at max brightness produce balanced white */
   uint8_t white[3] = {255, 255, 255};
   led_write_channels(led_dev, 0, 3, white);

Channel Enable/Disable
----------------------

Channels can be enabled or disabled independently. Disabled channels produce
no output regardless of PWM value:

.. code-block:: c

   /* Disable channel 5 */
   is31fl3235a_channel_enable(led_dev, 5, false);

   /* Re-enable channel 5 */
   is31fl3235a_channel_enable(led_dev, 5, true);

Power Management
----------------

The IS31FL3235A supports both software and hardware shutdown modes for power
saving:

Software Shutdown
^^^^^^^^^^^^^^^^^

Software shutdown turns off all outputs while maintaining register values and
I2C communication capability:

.. code-block:: c

   /* Enter low power mode */
   is31fl3235a_sw_shutdown(led_dev, true);

   /* Wake from low power mode */
   is31fl3235a_sw_shutdown(led_dev, false);

Hardware Shutdown
^^^^^^^^^^^^^^^^^

Hardware shutdown via the SDB pin provides the lowest power consumption:

.. code-block:: c

   /* Enter hardware shutdown (SDB low) */
   is31fl3235a_hw_shutdown(led_dev, true);

   /* Wake from hardware shutdown (SDB high) */
   is31fl3235a_hw_shutdown(led_dev, false);

Note: Hardware shutdown requires the SDB pin to be configured in device tree
via the ``sdb-gpios`` property.

Synchronized Updates
--------------------

The IS31FL3235A uses buffered registers. Changes to PWM values and channel
settings are staged internally and applied simultaneously when an update is
triggered. This ensures glitch-free multi-channel updates.

The standard LED API functions automatically trigger updates. For advanced
use cases requiring manual control:

.. code-block:: c

   /* Manual update trigger (advanced usage) */
   is31fl3235a_update(led_dev);

Performance Considerations
==========================

* **I2C Speed**: Use 400kHz (Fast Mode) for best performance
* **Multi-Channel Updates**: Use ``led_write_channels()`` instead of multiple
  ``led_set_brightness()`` calls for better efficiency
* **Update Latency**: ~1ms per I2C transaction at 400kHz
* **PWM Frequency**:

  * 3kHz: Lower EMI, general purpose (default)
  * 22kHz: Reduced flicker, faster response, suitable for high-speed scanning

Sample Application
==================

See :zephyr:code-sample:`led_is31fl3235a` for a comprehensive sample
application demonstrating all driver features.

API Reference
=============

Standard LED API
----------------

The driver implements the standard LED driver API:

.. doxygengroup:: led_interface
   :project: Zephyr

Extended API
------------

IS31FL3235A-specific extended functions:

.. doxygengroup:: is31fl3235a_interface
   :project: Zephyr
