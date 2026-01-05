# IS31FL3235A Register Definitions

## Complete Register Map

Based on IS31FL3235A datasheet analysis.

| Address | Name | R/W | Default | Description |
|---------|------|-----|---------|-------------|
| 0x00 | Shutdown Register | R/W | 0x00 | Software shutdown control |
| 0x01-0x04 | Reserved | - | - | Do not use |
| 0x05 | PWM Register (OUT1) | W | 0x00 | Channel 1 PWM duty cycle |
| 0x06 | PWM Register (OUT2) | W | 0x00 | Channel 2 PWM duty cycle |
| ... | ... | ... | ... | ... |
| 0x20 | PWM Register (OUT28) | W | 0x00 | Channel 28 PWM duty cycle |
| 0x21-0x24 | Reserved | - | - | Do not use |
| 0x25 | Update Register | W | - | Write 0x00 to load buffered data |
| 0x26-0x29 | Reserved | - | - | Do not use |
| 0x2A | LED Control (OUT1) | R/W | 0x00 | Channel 1 enable + current scale |
| 0x2B | LED Control (OUT2) | R/W | 0x00 | Channel 2 enable + current scale |
| ... | ... | ... | ... | ... |
| 0x45 | LED Control (OUT28) | R/W | 0x00 | Channel 28 enable + current scale |
| 0x46-0x4A | Reserved | - | - | Do not use |
| 0x4B | Output Frequency | R/W | 0x00 | PWM frequency selection |
| 0x4C-0x4E | Reserved | - | - | Do not use |
| 0x4F | Reset Register | W | - | Reset all registers to default |

## Detailed Register Descriptions

### Shutdown Register (0x00)

Controls software shutdown mode.

**Bit Fields:**

| Bit | Name | R/W | Default | Description |
|-----|------|-----|---------|-------------|
| D7:D1 | Reserved | - | 0 | Reserved, write as 0 |
| D0 | SSD | R/W | 0 | Software shutdown: 0=shutdown, 1=normal |

**Values:**
- `0x00`: Software shutdown (all outputs off, low power)
- `0x01`: Normal operation

**C Definition:**
```c
#define IS31FL3235A_REG_SHUTDOWN          0x00
#define IS31FL3235A_SHUTDOWN_SSD_BIT      BIT(0)
#define IS31FL3235A_SHUTDOWN_MODE         0x00  /* Shutdown */
#define IS31FL3235A_SHUTDOWN_NORMAL       0x01  /* Normal operation */
```

### PWM Registers (0x05-0x20)

28 registers controlling PWM duty cycle for each output channel.

**Bit Fields:**

| Bit | Name | R/W | Default | Description |
|-----|------|-----|---------|-------------|
| D7:D0 | PWM | W | 0x00 | PWM duty cycle (0-255) |

**Mapping:**
- 0x05 = Channel 1 (OUT1)
- 0x06 = Channel 2 (OUT2)
- ...
- 0x20 = Channel 28 (OUT28)

**PWM Values:**
- `0x00` (0): LED off (0% duty cycle)
- `0xFF` (255): Maximum brightness (100% duty cycle)
- Linear scaling between 0-255

**C Definition:**
```c
#define IS31FL3235A_REG_PWM_BASE          0x05
#define IS31FL3235A_REG_PWM_OUT1          0x05
#define IS31FL3235A_REG_PWM_OUT28         0x20
#define IS31FL3235A_PWM_REG(ch)           (IS31FL3235A_REG_PWM_BASE + (ch))
#define IS31FL3235A_PWM_MIN               0x00
#define IS31FL3235A_PWM_MAX               0xFF
```

### Update Register (0x25)

Loads buffered PWM and LED Control register data to active registers.

**Operation:**
Write `0x00` to this register to transfer all pending changes to active outputs.

**Note:**
- PWM registers (0x05-0x20) are buffered
- LED Control registers (0x2A-0x45) are buffered
- Writing to Update Register applies all changes simultaneously
- Allows synchronized multi-channel updates

**C Definition:**
```c
#define IS31FL3235A_REG_UPDATE            0x25
#define IS31FL3235A_UPDATE_TRIGGER        0x00
```

### LED Control Registers (0x2A-0x45)

28 registers controlling enable state and current scaling for each output.

**Bit Fields:**

| Bit | Name | R/W | Default | Description |
|-----|------|-----|---------|-------------|
| D7:D3 | Reserved | - | 0 | Reserved, write as 0 |
| D2:D1 | SL | R/W | 00b | Current scale select |
| D0 | OUT | R/W | 0 | Output enable: 0=off, 1=on |

**SL (Current Scale) Bit Values:**

| D2:D1 | Binary | Description | Current |
|-------|--------|-------------|---------|
| 0 | 00b | Full scale | IMAX |
| 1 | 01b | Half scale | IMAX/2 |
| 2 | 10b | Third scale | IMAX/3 |
| 3 | 11b | Quarter scale | IMAX/4 |

**OUT (Enable) Bit Values:**
- `0`: Output disabled (off)
- `1`: Output enabled (on, controlled by PWM)

**Mapping:**
- 0x2A = Channel 1 (OUT1)
- 0x2B = Channel 2 (OUT2)
- ...
- 0x45 = Channel 28 (OUT28)

**Example Values:**
- `0x01` (00000_00_1): Enabled, 1× current
- `0x03` (00000_01_1): Enabled, 1/2× current
- `0x05` (00000_10_1): Enabled, 1/3× current
- `0x07` (00000_11_1): Enabled, 1/4× current
- `0x00` (00000_00_0): Disabled

**C Definition:**
```c
#define IS31FL3235A_REG_CTRL_BASE         0x2A
#define IS31FL3235A_REG_CTRL_OUT1         0x2A
#define IS31FL3235A_REG_CTRL_OUT28        0x45
#define IS31FL3235A_CTRL_REG(ch)          (IS31FL3235A_REG_CTRL_BASE + (ch))

/* Bit positions */
#define IS31FL3235A_CTRL_OUT_BIT          0      /* D0 */
#define IS31FL3235A_CTRL_SL_SHIFT         1      /* D2:D1 */
#define IS31FL3235A_CTRL_SL_MASK          0x06   /* Mask for D2:D1 */

/* OUT bit values */
#define IS31FL3235A_CTRL_OUT_DISABLE      0x00
#define IS31FL3235A_CTRL_OUT_ENABLE       BIT(0)

/* SL (current scale) values */
#define IS31FL3235A_CTRL_SL_1X            (0 << IS31FL3235A_CTRL_SL_SHIFT)  /* 00b */
#define IS31FL3235A_CTRL_SL_HALF          (1 << IS31FL3235A_CTRL_SL_SHIFT)  /* 01b */
#define IS31FL3235A_CTRL_SL_THIRD         (2 << IS31FL3235A_CTRL_SL_SHIFT)  /* 10b */
#define IS31FL3235A_CTRL_SL_QUARTER       (3 << IS31FL3235A_CTRL_SL_SHIFT)  /* 11b */

/* Convenience: enable + scale combinations */
#define IS31FL3235A_CTRL_ENABLE_1X        (IS31FL3235A_CTRL_OUT_ENABLE | IS31FL3235A_CTRL_SL_1X)
#define IS31FL3235A_CTRL_ENABLE_HALF      (IS31FL3235A_CTRL_OUT_ENABLE | IS31FL3235A_CTRL_SL_HALF)
#define IS31FL3235A_CTRL_ENABLE_THIRD     (IS31FL3235A_CTRL_OUT_ENABLE | IS31FL3235A_CTRL_SL_THIRD)
#define IS31FL3235A_CTRL_ENABLE_QUARTER   (IS31FL3235A_CTRL_OUT_ENABLE | IS31FL3235A_CTRL_SL_QUARTER)
```

### Output Frequency Register (0x4B)

Selects PWM frequency for all channels.

**Bit Fields:**

| Bit | Name | R/W | Default | Description |
|-----|------|-----|---------|-------------|
| D7:D1 | Reserved | - | 0 | Reserved, write as 0 |
| D0 | OFS | R/W | 0 | Output frequency select |

**OFS Bit Values:**
- `0`: 3kHz PWM frequency (default)
- `1`: 22kHz PWM frequency

**C Definition:**
```c
#define IS31FL3235A_REG_FREQ              0x4B
#define IS31FL3235A_FREQ_OFS_BIT          BIT(0)
#define IS31FL3235A_FREQ_3KHZ             0x00
#define IS31FL3235A_FREQ_22KHZ            0x01
```

### Reset Register (0x4F)

Resets all registers to default values.

**Operation:**
Write `0x00` to this register to perform a software reset.

**Effect:**
- All PWM registers → 0x00
- All LED Control registers → 0x00
- Shutdown register → 0x00 (shutdown mode)
- Frequency register → 0x00 (3kHz)

**Note:**
After reset, device is in shutdown mode. Must write to Shutdown Register to wake.

**C Definition:**
```c
#define IS31FL3235A_REG_RESET             0x4F
#define IS31FL3235A_RESET_TRIGGER         0x00
```

## Register Access Patterns

### Initialization Sequence

```c
/* 1. Reset to defaults */
i2c_write(IS31FL3235A_REG_RESET, 0x00);
delay_ms(1);

/* 2. Wake from shutdown */
i2c_write(IS31FL3235A_REG_SHUTDOWN, IS31FL3235A_SHUTDOWN_NORMAL);

/* 3. Set PWM frequency */
i2c_write(IS31FL3235A_REG_FREQ, IS31FL3235A_FREQ_22KHZ);

/* 4. Configure channels (example: channel 0) */
i2c_write(IS31FL3235A_PWM_REG(0), 0x80);           /* 50% brightness */
i2c_write(IS31FL3235A_CTRL_REG(0), IS31FL3235A_CTRL_ENABLE_1X);

/* 5. Apply changes */
i2c_write(IS31FL3235A_REG_UPDATE, IS31FL3235A_UPDATE_TRIGGER);
```

### Setting Single LED Brightness

```c
/* Set channel 5 to 75% brightness */
i2c_write(IS31FL3235A_PWM_REG(5), 192);
i2c_write(IS31FL3235A_REG_UPDATE, IS31FL3235A_UPDATE_TRIGGER);
```

### Synchronized Multi-Channel Update

```c
/* Update 3 channels simultaneously */
i2c_write(IS31FL3235A_PWM_REG(0), 255);  /* Ch0: 100% */
i2c_write(IS31FL3235A_PWM_REG(1), 128);  /* Ch1: 50% */
i2c_write(IS31FL3235A_PWM_REG(2), 64);   /* Ch2: 25% */
/* All buffered until... */
i2c_write(IS31FL3235A_REG_UPDATE, IS31FL3235A_UPDATE_TRIGGER);
/* Now all 3 channels update simultaneously */
```

### Changing Current Scale

```c
/* Set channel 10 to half current scale */
uint8_t ctrl = i2c_read(IS31FL3235A_CTRL_REG(10));
ctrl &= ~IS31FL3235A_CTRL_SL_MASK;              /* Clear scale bits */
ctrl |= IS31FL3235A_CTRL_SL_HALF;               /* Set to 1/2 */
i2c_write(IS31FL3235A_CTRL_REG(10), ctrl);
i2c_write(IS31FL3235A_REG_UPDATE, IS31FL3235A_UPDATE_TRIGGER);
```

### Software Shutdown

```c
/* Enter shutdown */
i2c_write(IS31FL3235A_REG_SHUTDOWN, IS31FL3235A_SHUTDOWN_MODE);

/* Wake from shutdown */
i2c_write(IS31FL3235A_REG_SHUTDOWN, IS31FL3235A_SHUTDOWN_NORMAL);
```

## Constants and Limits

```c
/* Device limits */
#define IS31FL3235A_NUM_CHANNELS          28
#define IS31FL3235A_MAX_BRIGHTNESS        255

/* I2C addresses (7-bit) */
#define IS31FL3235A_I2C_ADDR_GND          0x3C  /* AD pin = GND */
#define IS31FL3235A_I2C_ADDR_VCC          0x3D  /* AD pin = VCC */
#define IS31FL3235A_I2C_ADDR_SCL          0x3E  /* AD pin = SCL */
#define IS31FL3235A_I2C_ADDR_SDA          0x3F  /* AD pin = SDA */

/* Timing (from datasheet - verify exact values) */
#define IS31FL3235A_RESET_DELAY_MS        1     /* Delay after reset */
#define IS31FL3235A_STARTUP_DELAY_MS      1     /* Delay after SDB high */
```

## Channel Mapping Reference

For easy reference when working with channels:

| Channel # (Code) | Hardware Name | PWM Register | Control Register |
|------------------|---------------|--------------|------------------|
| 0 | OUT1 | 0x05 | 0x2A |
| 1 | OUT2 | 0x06 | 0x2B |
| 2 | OUT3 | 0x07 | 0x2C |
| 3 | OUT4 | 0x08 | 0x2D |
| 4 | OUT5 | 0x09 | 0x2E |
| 5 | OUT6 | 0x0A | 0x2F |
| 6 | OUT7 | 0x0B | 0x30 |
| 7 | OUT8 | 0x0C | 0x31 |
| 8 | OUT9 | 0x0D | 0x32 |
| 9 | OUT10 | 0x0E | 0x33 |
| 10 | OUT11 | 0x0F | 0x34 |
| 11 | OUT12 | 0x10 | 0x35 |
| 12 | OUT13 | 0x11 | 0x36 |
| 13 | OUT14 | 0x12 | 0x37 |
| 14 | OUT15 | 0x13 | 0x38 |
| 15 | OUT16 | 0x14 | 0x39 |
| 16 | OUT17 | 0x15 | 0x3A |
| 17 | OUT18 | 0x16 | 0x3B |
| 18 | OUT19 | 0x17 | 0x3C |
| 19 | OUT20 | 0x18 | 0x3D |
| 20 | OUT21 | 0x19 | 0x3E |
| 21 | OUT22 | 0x1A | 0x3F |
| 22 | OUT23 | 0x1B | 0x40 |
| 23 | OUT24 | 0x1C | 0x41 |
| 24 | OUT25 | 0x1D | 0x42 |
| 25 | OUT26 | 0x1E | 0x43 |
| 26 | OUT27 | 0x1F | 0x44 |
| 27 | OUT28 | 0x20 | 0x45 |

## Notes

1. **Buffered Updates:** PWM and LED Control registers use double-buffering. Changes take effect only after writing to Update Register (0x25).

2. **Channel Indexing:** Code uses 0-27, datasheet uses OUT1-OUT28. Always use macros for clarity.

3. **Reserved Bits:** Always write 0 to reserved bits to ensure forward compatibility.

4. **Reset Behavior:** After reset or power-on, device is in shutdown mode. Must explicitly wake.

5. **I2C Speed:** Device supports standard (100kHz) and fast (400kHz) I2C modes.

6. **Current Setting:** Maximum current (IMAX) is set by external resistor (REXT). Current scaling provides software control.
