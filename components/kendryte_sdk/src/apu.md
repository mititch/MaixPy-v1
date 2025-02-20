# Microphone Array Processor (APU)




## Overview

APU (Array Processing Unit) is a microphone array voice data acceleration processing unit that can calculate voice delay accumulation values in different directions in real-time. These values are used to determine the direction of voice and enhance voice data from a specific direction.

## Features

APU has the following features:

- Supports up to eight microphones
- Can calculate voice delay accumulation values in 16 directions in real-time
- Supports enhanced voice output in selected directions

## API Reference

Corresponding header file: `apu.h`

The following interfaces are provided to users:

- apu_dir_set_prev_fir
- apu_dir_set_post_fir
- apu_voc_set_prev_fir
- apu_voc_set_post_fir
- apu_set_delay
- apu_voc_set_direction
- apu_set_channel_enabled
- apu_dir_enable
- apu_voc_enable

### apu_dir_set_prev_fir

#### Description

Sets the FIR filter coefficients before calculating delay accumulation values for different directions.

#### Function Definition

```c
void apu_dir_set_prev_fir(uint16_t *fir_coef)
```

#### Parameters

| Parameter Name |   Description                         | Input/Output |
| -------------- | ------------------------------------- | ------------ |
| fir_coef       | Pointer to 17-bit filter coefficients | Input        |

#### Return Value

None.

### apu_dir_set_post_fir

#### Description

Sets the FIR filter coefficients after calculating delay accumulation values for different directions.

#### Function Prototype

```c
void apu_dir_set_post_fir(uint16_t *fir_coef)
```

#### Parameters

| Parameter Name |   Description                         | Input/Output |
| -------------- | ------------------------------------- | ------------ |
| fir_coef       | Pointer to 17-bit filter coefficients | Input        |

#### Return Value

None.

### apu_voc_set_prev_fir

#### Description

Sets the FIR filter coefficients before calculating delay accumulation value for the selected direction.

#### Function Prototype

```c
void apu_voc_set_prev_fir(uint16_t *fir_coef)
```

#### Parameters

| Parameter Name |   Description                         | Input/Output |
| -------------- | ------------------------------------- | ------------ |
| fir_coef       | Pointer to 17-bit filter coefficients | Input        |

#### Return Value

None.

### apu_voc_set_post_fir

#### Description

Sets the FIR filter coefficients after calculating delay accumulation value for the selected direction.

#### Function Prototype

```c
void apu_voc_set_post_fir(uint16_t *fir_coef)
```

#### Parameters

| Parameter Name |   Description                         | Input/Output |
| -------------- | ------------------------------------- | ------------ |
| fir_coef       | Pointer to 17-bit filter coefficients | Input        |

#### Return Value

None.

### apu_set_delay

#### Description

Initializes the sound wave delay parameters between microphones on a circular microphone array board.

#### Function Prototype

```c
void apu_set_delay(float R, uint8_t mic_num_a_circle, uint8_t center, float I2s_fs)
```

#### Parameters

| Parameter Name     | Description                                  | Input/Output |
| ------------------ | -------------------------------------------- | ------------ |
| R                  | Radius of the microphone circle              | Input        |
| mic_num_a_circle   | Number of microphones on the circle perimeter| Input        |
| center             | Whether there is a microphone at the center  | Input        |
| I2s_fs             | Microphone sampling rate                     | Input        |

#### Return Value

None.

### apu_voc_set_direction

#### Description

Selects the direction for voice enhancement.

#### Function Prototype

```c
void apu_voc_set_direction(enum en_bf_dir direction)
```

#### Parameters

| Parameter Name | Description                                      | Input/Output |
| -------------- | ------------------------------------------------ | ------------ |
| direction      | Selected direction for voice enhancement (0-15)  | Input        |

#### Return Value

None.

### apu_set_channel_enabled

#### Description

Selects the microphone channels to use. I2S has 4 routes, each with left and right channels, totaling 8 channels. Setting the corresponding bit to 1 enables that channel.

#### Function Prototype

```c
void apu_set_channel_enabled(uint8_t channel_bit)
```

#### Parameters

| Parameter Name | Description                                        | Input/Output |
| -------------- | -------------------------------------------------- | ------------ |
| channel_bit    | Set corresponding bit to 1 to enable that channel  | Input        |

#### Return Value

None.

### apu_dir_enable

#### Description
Enables or disables the calculation of delay accumulation values in different directions.

#### Function Prototype

```c
void apu_dir_enable(void)
```

#### Parameters

None.

#### Return Value

None.

### apu_voc_enable

#### Description

Enables or disables voice enhancement in the selected direction.

#### Function Prototype

```c
void apu_voc_enable(void)
```

#### Parameters

None.

#### Return Value

None.

## Data Types

The following data types and data structures are defined:

- apu_reg_t: APU task structure.

### apu_reg_t

#### Description

APU task structure.

#### Definition

```c
typedef struct _apu_reg
{
    //0x200
    apu_ch_cfg_t         bf_ch_cfg_reg;
    //0x204
    apu_ctl_t            bf_ctl_reg;
    //0x208
    apu_dir_bidx_t       bf_dir_bidx[16][2];
    //0x288
    apu_fir_coef_t       bf_pre_fir0_coef[9];
    //0x2ac
    apu_fir_coef_t       bf_post_fir0_coef[9];
    //0x2d0
    apu_fir_coef_t       bf_pre_fir1_coef[9];
    //0x2f4
    apu_fir_coef_t       bf_post_fir1_coef[9];
    //0x318
    apu_dwsz_cfg_t       bf_dwsz_cfg_reg;
    //0x31c
    apu_fft_cfg_t        bf_fft_cfg_reg;
    // 0x320
    volatile uint32_t    sobuf_dma_rdata;
    // 0x324
    volatile uint32_t    vobuf_dma_rdata;
    /*0x328*/
    apu_int_stat_t       bf_int_stat_reg;
    /*0x32c*/
    apu_int_mask_t       bf_int_mask_reg;
    /*0x330*/
    uint32_t             saturation_counter;
    /*0x334*/
    uint32_t             saturation_limits;
} __attribute__((packed, aligned(4))) apu_reg_t;
```

#### Members

| Member Name           | Description                                          |
| --------------------- | ---------------------------------------------------- |
| bf_ch_cfg_reg         | Channel configuration                                |
| bf_ctl_reg            | Control register                                     |
| bf_dir_bidx[16][2]    | Direction index buffer                               |
| bf_pre_fir0_coef[9]   | Pre-FIR coefficients for direction calculation       |
| bf_post_fir0_coef[9]  | Post-FIR coefficients for direction calculation      |
| bf_pre_fir1_coef[9]   | Pre-FIR coefficients for selected direction          |
| bf_post_fir1_coef[9]  | Post-FIR coefficients for selected direction         |
| bf_dwsz_cfg_reg       | Downsampling coefficient                             |
| bf_fft_cfg_reg        | FFT control register                                 |
| sobuf_dma_rdata       | Source address for direction data                    |
| vobuf_dma_rdata       | Source address for selected direction data           |
| bf_int_stat_reg       | Interrupt status register                            |
| bf_int_mask_reg       | Interrupt mask register                              |
| saturation_counter    | Saturation counter                                   |
| saturation_limits     | Saturation limits                                    |

## Example

```c
// Set FIR filter coefficients
apu_dir_set_prev_fir(fir_prev_t);
apu_dir_set_post_fir(fir_post_t);
apu_voc_set_prev_fir(fir_prev_t);
apu_voc_set_post_fir(fir_post_t);

// Initialize with a circular array of 7 microphones, radius 3cm, with center mic, at 44.1kHz
apu_set_delay(3, 7, 1, 44100);
apu_voc_set_direction(0);  // Set direction to 0 degrees
apu_set_channel_enabled(0xff);  // Enable all channels

apu_dir_enable();
apu_voc_enable(1);
