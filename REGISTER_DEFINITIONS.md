# IS31FL3235A Register Definitions

## Register Map

| Address | Name | R/W | Default | Description |
|---------|------|-----|---------|-------------|
| 0x00 | Shutdown | R/W | 0x00 | Software shutdown control |
| 0x01-0x04 | Reserved | - | - | Do not use |
| 0x05-0x20 | PWM (OUT1-OUT28) | W | 0x00 | Per-channel PWM duty cycle |
| 0x21-0x24 | Reserved | - | - | Do not use |
| 0x25 | Update | W | - | Write to apply buffered changes |
| 0x26-0x29 | Reserved | - | - | Do not use |
| 0x2A-0x45 | LED Control (OUT1-OUT28) | R/W | 0x00 | Per-channel enable + current scale |
| 0x46-0x49 | Reserved | - | - | Do not use |
| 0x4A | Global Control | R/W | 0x00 | Global LED enable |
| 0x4B | Frequency | R/W | 0x00 | PWM frequency selection |
| 0x4C-0x4E | Reserved | - | - | Do not use |
| 0x4F | Reset | W | - | Software reset |

## Register Details

### Shutdown Register (0x00)

Controls software shutdown mode.

| Bit | Name | Default | Description |
|-----|------|---------|-------------|
| D7:D1 | Reserved | 0 | Write as 0 |
| D0 | SSD | 0 | 0=shutdown, 1=normal operation |

**Values:**
- `0x00`: Shutdown mode (all outputs off, low power, registers retained)
- `0x01`: Normal operation

### PWM Registers (0x05-0x20)

28 registers controlling PWM duty cycle. Values are buffered until Update register is written.

| Bit | Name | Default | Description |
|-----|------|---------|-------------|
| D7:D0 | PWM | 0x00 | Duty cycle (0=off, 255=max) |

**Channel Mapping:**
- 0x05 = Channel 0 (OUT1)
- 0x06 = Channel 1 (OUT2)
- ...
- 0x20 = Channel 27 (OUT28)

### Update Register (0x25)

Writing `0x00` transfers buffered PWM and LED Control values to active registers. This enables synchronized multi-channel updates.

### LED Control Registers (0x2A-0x45)

28 registers controlling enable state and current scaling. Values are buffered until Update register is written.

| Bit | Name | Default | Description |
|-----|------|---------|-------------|
| D7:D3 | Reserved | 0 | Write as 0 |
| D2:D1 | SL | 00 | Current scale select |
| D0 | OUT | 0 | Output enable (0=off, 1=on) |

**Current Scale (SL) Values:**

| D2:D1 | Scale | Current |
|-------|-------|---------|
| 00 | 1x | IMAX |
| 01 | 1/2x | IMAX/2 |
| 10 | 1/3x | IMAX/3 |
| 11 | 1/4x | IMAX/4 |

**Example Values:**
- `0x01`: Enabled, 1x current
- `0x03`: Enabled, 1/2x current
- `0x05`: Enabled, 1/3x current
- `0x07`: Enabled, 1/4x current
- `0x00`: Disabled

### Global Control Register (0x4A)

Global enable/disable for all LED outputs.

| Bit | Name | Default | Description |
|-----|------|---------|-------------|
| D7:D1 | Reserved | 0 | Write as 0 |
| D0 | G_EN | 0 | 0=normal operation, 1=shutdown all LEDs |

**Values:**
- `0x00`: Normal operation (LEDs controlled by individual settings)
- `0x01`: All LED outputs disabled (individual settings preserved)

### Frequency Register (0x4B)

Selects PWM frequency for all channels.

| Bit | Name | Default | Description |
|-----|------|---------|-------------|
| D7:D1 | Reserved | 0 | Write as 0 |
| D0 | OFS | 0 | 0=3kHz, 1=22kHz |

### Reset Register (0x4F)

Writing `0x00` resets all registers to default values. After reset, device is in shutdown mode.

## Channel Mapping Reference

| Channel (Code) | Output | PWM Reg | Control Reg |
|----------------|--------|---------|-------------|
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

## I2C Addresses

| AD Pin | Address (7-bit) |
|--------|-----------------|
| GND | 0x3C |
| VCC | 0x3D |
| SCL | 0x3E |
| SDA | 0x3F |

## Notes

1. **Buffered Updates:** PWM and LED Control registers are double-buffered. Changes take effect only after writing to the Update register.

2. **Channel Indexing:** Driver code uses 0-27, datasheet uses OUT1-OUT28.

3. **Reserved Bits:** Always write 0 to reserved bits.

4. **Reset State:** After reset or power-on, device is in shutdown mode.

5. **I2C Speed:** Supports standard (100kHz) and fast (400kHz) modes.
