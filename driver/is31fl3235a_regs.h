/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LED_IS31FL3235A_REGS_H_
#define ZEPHYR_DRIVERS_LED_IS31FL3235A_REGS_H_

/**
 * @file
 * @brief IS31FL3235A LED driver register definitions
 *
 * Register definitions for the ISSI/Lumissil IS31FL3235A
 * 28-channel LED driver with I2C interface.
 */

/* Configuration and Control Registers */
#define IS31FL3235A_REG_SHUTDOWN        0x00  /* Software shutdown control */
#define IS31FL3235A_REG_PWM_BASE        0x05  /* PWM registers base (OUT1) */
#define IS31FL3235A_REG_PWM_OUT1        0x05  /* PWM register for OUT1 */
#define IS31FL3235A_REG_PWM_OUT28       0x20  /* PWM register for OUT28 */
#define IS31FL3235A_REG_UPDATE          0x25  /* Update/load register */
#define IS31FL3235A_REG_CTRL_BASE       0x2A  /* LED control registers base */
#define IS31FL3235A_REG_CTRL_OUT1       0x2A  /* LED control for OUT1 */
#define IS31FL3235A_REG_CTRL_OUT28      0x45  /* LED control for OUT28 */
#define IS31FL3235A_REG_FREQ            0x4B  /* PWM frequency select */
#define IS31FL3235A_REG_RESET           0x4F  /* Reset register */

/* Helper macros for register access */
#define IS31FL3235A_PWM_REG(ch)         (IS31FL3235A_REG_PWM_BASE + (ch))
#define IS31FL3235A_CTRL_REG(ch)        (IS31FL3235A_REG_CTRL_BASE + (ch))

/* Constants */
#define IS31FL3235A_NUM_CHANNELS        28
#define IS31FL3235A_MAX_BRIGHTNESS      255

/* Shutdown Register (0x00) bits */
#define IS31FL3235A_SHUTDOWN_SSD_BIT    BIT(0)  /* Software shutdown */
#define IS31FL3235A_SHUTDOWN_MODE       0x00    /* Shutdown mode */
#define IS31FL3235A_SHUTDOWN_NORMAL     0x01    /* Normal operation */

/* Update Register (0x25) */
#define IS31FL3235A_UPDATE_TRIGGER      0x00    /* Write to trigger update */

/* LED Control Register (0x2A-0x45) bit fields */
#define IS31FL3235A_CTRL_OUT_BIT        0       /* D0: Output enable */
#define IS31FL3235A_CTRL_SL_SHIFT       1       /* D2:D1: Current scale */
#define IS31FL3235A_CTRL_SL_MASK        0x06    /* Mask for D2:D1 */

/* OUT bit (D0) values */
#define IS31FL3235A_CTRL_OUT_DISABLE    0x00
#define IS31FL3235A_CTRL_OUT_ENABLE     BIT(0)

/* SL (current scale) values (D2:D1) */
#define IS31FL3235A_CTRL_SL_1X          (0 << IS31FL3235A_CTRL_SL_SHIFT)  /* 00b: IMAX */
#define IS31FL3235A_CTRL_SL_HALF        (1 << IS31FL3235A_CTRL_SL_SHIFT)  /* 01b: IMAX/2 */
#define IS31FL3235A_CTRL_SL_THIRD       (2 << IS31FL3235A_CTRL_SL_SHIFT)  /* 10b: IMAX/3 */
#define IS31FL3235A_CTRL_SL_QUARTER     (3 << IS31FL3235A_CTRL_SL_SHIFT)  /* 11b: IMAX/4 */

/* Convenience macros: enable + scale combinations */
#define IS31FL3235A_CTRL_ENABLE_1X      (IS31FL3235A_CTRL_OUT_ENABLE | \
					 IS31FL3235A_CTRL_SL_1X)
#define IS31FL3235A_CTRL_ENABLE_HALF    (IS31FL3235A_CTRL_OUT_ENABLE | \
					 IS31FL3235A_CTRL_SL_HALF)
#define IS31FL3235A_CTRL_ENABLE_THIRD   (IS31FL3235A_CTRL_OUT_ENABLE | \
					 IS31FL3235A_CTRL_SL_THIRD)
#define IS31FL3235A_CTRL_ENABLE_QUARTER (IS31FL3235A_CTRL_OUT_ENABLE | \
					 IS31FL3235A_CTRL_SL_QUARTER)

/* Output Frequency Register (0x4B) */
#define IS31FL3235A_FREQ_OFS_BIT        BIT(0)  /* Output frequency select */
#define IS31FL3235A_FREQ_3KHZ           0x00    /* 3kHz PWM */
#define IS31FL3235A_FREQ_22KHZ          0x01    /* 22kHz PWM */

/* Reset Register (0x4F) */
#define IS31FL3235A_RESET_TRIGGER       0x00    /* Write to reset */

/* I2C addresses (7-bit, based on AD pin strapping) */
#define IS31FL3235A_I2C_ADDR_GND        0x3C    /* AD pin = GND */
#define IS31FL3235A_I2C_ADDR_VCC        0x3D    /* AD pin = VCC */
#define IS31FL3235A_I2C_ADDR_SCL        0x3E    /* AD pin = SCL */
#define IS31FL3235A_I2C_ADDR_SDA        0x3F    /* AD pin = SDA */

/* Timing constants (in milliseconds) */
#define IS31FL3235A_RESET_DELAY_MS      1       /* Delay after reset */
#define IS31FL3235A_STARTUP_DELAY_MS    1       /* Delay after SDB high */

/* PWM value limits */
#define IS31FL3235A_PWM_MIN             0x00
#define IS31FL3235A_PWM_MAX             0xFF

#endif /* ZEPHYR_DRIVERS_LED_IS31FL3235A_REGS_H_ */
