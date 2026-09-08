#pragma once
#include <stdint.h>
#include <plic.h>
#include <i2s.h>
#include <sysctl.h>
#include <dmac.h>
#include <fpioa.h>
#include "sipeed_sk9822.h"
#include "gpiohs.h"
#include "sleep.h"
#include "apu.h"


#ifndef APU_DIR_ENABLE
#define APU_DIR_ENABLE 1
#endif

#ifndef APU_VOC_ENABLE
#define APU_VOC_ENABLE 1
#endif

#ifndef APU_DMA_ENABLE
#define APU_DMA_ENABLE 1
#endif

#ifndef APU_FFT_ENABLE
#define APU_FFT_ENABLE 0
#endif

#ifndef APU_DATA_DEBUG
#define APU_DATA_DEBUG 0
#endif

#ifndef APU_GAIN_DEBUG
#define APU_GAIN_DEBUG 0
#endif

#ifndef APU_SETDIR_DEBUG
#define APU_SETDIR_DEBUG 0
#endif

#ifndef APU_SMPL_SHIFT
#define APU_SMPL_SHIFT 0x00
#endif

#ifndef APU_SATURATION_DEBUG
#define APU_SATURATION_DEBUG 0
#endif

#ifndef APU_SATURATION_VPOS_DEBUG
#define APU_SATURATION_VPOS_DEBUG 0x07ff
#endif

#ifndef APU_SATURATION_VNEG_DEBUG
#define APU_SATURATION_VNEG_DEBUG 0xf800
#endif

#ifndef APU_INPUT_CONST_DEBUG
#define APU_INPUT_CONST_DEBUG 0x0
#endif

#ifndef APU_SMPL_SHIFT_DEBUG
#define APU_SMPL_SHIFT_DEBUG 0
#endif

#ifndef I2S_RESOLUTION_TEST
#define I2S_RESOLUTION_TEST RESOLUTION_12_BIT
#endif

#ifndef I2S_SCLK_CYCLES_TEST
#define I2S_SCLK_CYCLES_TEST SCLK_CYCLES_16
#endif

#ifndef SYSCTL_THRESHOLD_I2S0_TEST
#define SYSCTL_THRESHOLD_I2S0_TEST 0xf
#endif

#ifndef APU_AUDIO_GAIN_TEST
#define APU_AUDIO_GAIN_TEST (1 << 10)
#endif

#ifndef APU_PRESETN_DEBUG
#define APU_PRESETN_DEBUG 1
#endif

#ifndef APU_DEBUG_NO_EXIT
#define APU_DEBUG_NO_EXIT 1
#endif


#define APU_DIR_DMA_CHANNEL DMAC_CHANNEL3
#define APU_VOC_DMA_CHANNEL DMAC_CHANNEL4

#define APU_DIR_CHANNEL_MAX 16
#define APU_DIR_CHANNEL_SIZE 512
#define APU_VOC_CHANNEL_SIZE 512

// Vendor apu_dir_set_prev_fir/apu_dir_set_post_fir/apu_voc_set_prev_fir/apu_voc_set_post_fir pack
// coefficients 2-per-register across 9 registers (bf_pre_fir0_coef[9] etc.), with the very last
// tap (register 8's second tap) always forced to 0 by the driver. That leaves 17 real, caller-
// controlled taps (indices 0..16) -- NOT 16. Every buffer that holds a FIR coefficient array,
// on both the C and MicroPython side, must be sized APU_FIR_TAP_COUNT, not a bare 16.
#define APU_FIR_TAP_COUNT 17

#if APU_FFT_ENABLE
extern uint32_t APU_DIR_FFT_BUFFER[APU_DIR_CHANNEL_MAX]
				       [APU_DIR_CHANNEL_SIZE]
	__attribute__((aligned(128)));
extern uint32_t APU_VOC_FFT_BUFFER[APU_VOC_CHANNEL_SIZE]
	__attribute__((aligned(128)));
#else
extern int16_t APU_DIR_BUFFER[APU_DIR_CHANNEL_MAX]
				  [APU_DIR_CHANNEL_SIZE]
	__attribute__((aligned(128)));
extern int16_t APU_VOC_BUFFER[APU_VOC_CHANNEL_SIZE]
	__attribute__((aligned(128)));
#endif


extern uint64_t dir_logic_count;
extern uint64_t voc_logic_count;



// Initialize APU defaults
void lib_apu_init_all(int i2s_d0_pin, int i2s_d1_pin, 
                  int i2s_d2_pin, int i2s_d3_pin,
                  int i2s_ws_pin, int i2s_sclk_pin);

void lib_apu_init_led(int sk9822_dat_pin, int sk9822_clk_pin);

void lib_apu_set_led(uint32_t degree, uint32_t color, uint32_t delay);

// Configure APU direction
void lib_apu_configure_direction(float radius, uint8_t mic_num_a_circle, uint8_t center);

// Start direction detection
void lib_apu_start_direction_detection(void);

// Get current direction with confidence level
typedef struct {
    en_bf_dir_t direction;  // Detected direction (argmax sector)
    int32_t power;          // Signal power in the detected direction (== sector_power[direction])
    int32_t sector_power[APU_DIR_CHANNEL_MAX]; // Power for all 16 sectors from the same dir_logic() pass as
                                                // direction/power above, so a caller doing sub-sector
                                                // interpolation always compares values from one consistent frame.
    int16_t samples[APU_DIR_CHANNEL_SIZE];
    int16_t voc_samples[APU_DIR_CHANNEL_SIZE];
    en_bf_dir_t voc_dir;
} apu_dir_result_t;

// Check if direction detection is ready
uint8_t lib_apu_dir_is_ready(void);

// Get current direction (only valid when lib_apu_dir_is_ready returns 1)
apu_dir_result_t lib_apu_get_direction(void);

// Clear direction detection ready flag
void lib_apu_dir_clear_ready(void);

// Enable voice output for a specific direction
void lib_apu_enable_voice_output(en_bf_dir_t direction);

// Disable voice output
void lib_apu_disable_voice_output(void);

// Set audio source mode (0: internal buffer, 1: FFT buffer)
void lib_apu_set_source_mode(uint8_t mode);

// Reset APU
void lib_apu_reset(void);

// Set direction detection pre-FIR coefficients. `coefficients` must point to APU_FIR_TAP_COUNT (17) taps.
void lib_apu_set_dir_pre_fir(const uint16_t* coefficients);

// Set direction detection post-FIR coefficients. `coefficients` must point to APU_FIR_TAP_COUNT (17) taps.
void lib_apu_set_dir_post_fir(const uint16_t* coefficients);

// Set voice output pre-FIR coefficients. `coefficients` must point to APU_FIR_TAP_COUNT (17) taps.
void lib_apu_set_voice_pre_fir(const uint16_t* coefficients);

// Set voice output post-FIR coefficients. `coefficients` must point to APU_FIR_TAP_COUNT (17) taps.
void lib_apu_set_voice_post_fir(const uint16_t* coefficients);

// Get direction detection pre-FIR coefficients. `coefficients` must point to APU_FIR_TAP_COUNT (17) taps.
void lib_apu_get_dir_pre_fir(uint16_t* coefficients);

// Get direction detection post-FIR coefficients. `coefficients` must point to APU_FIR_TAP_COUNT (17) taps.
void lib_apu_get_dir_post_fir(uint16_t* coefficients);

// Get voice output pre-FIR coefficients. `coefficients` must point to APU_FIR_TAP_COUNT (17) taps.
void lib_apu_get_voice_pre_fir(uint16_t* coefficients);

// Get voice output post-FIR coefficients. `coefficients` must point to APU_FIR_TAP_COUNT (17) taps.
void lib_apu_get_voice_post_fir(uint16_t* coefficients);

// Set down-sizing ratio for direction searching
void lib_apu_dir_set_down_size(uint8_t dir_dwn_size);

// Set down-sizing ratio for voice stream generation
void lib_apu_voc_set_down_size(uint8_t voc_dwn_size);

// Set direction searching interrupt mask
void lib_apu_dir_set_interrupt_mask(uint8_t dir_int_mask);

// Set voice stream generation interrupt mask
void lib_apu_voc_set_interrupt_mask(uint8_t voc_int_mask);

// Clear direction interrupt state
void lib_apu_dir_clear_int_state(void);

// Clear voice interrupt state
void lib_apu_voc_clear_int_state(void);

// Reset saturation counter
void lib_apu_voc_reset_saturation_counter(void);

// Get saturation counter (high 16 bits: counter, low 16 bits: total)
uint32_t lib_apu_voc_get_saturation_counter(void);

// Set saturation limit (upper and bottom thresholds)
void lib_apu_voc_set_saturation_limit(uint16_t upper, uint16_t bottom);

// Get saturation limit (high 16 bits: upper, low 16 bits: bottom)
uint32_t lib_apu_voc_get_saturation_limit(void);

/**
 * @brief       Initialize FPIOA pins for APU/I2S
 *
 * @param[in]   i2s_d0_pin              I2S data pin 0
 * @param[in]   i2s_d1_pin              I2S data pin 1
 * @param[in]   i2s_d2_pin              I2S data pin 2
 * @param[in]   i2s_d3_pin              I2S data pin 3
 * @param[in]   i2s_ws_pin              I2S word select pin
 * @param[in]   i2s_sclk_pin            I2S serial clock pin
 */
void lib_apu_init_fpioa(int i2s_d0_pin, int i2s_d1_pin,
                        int i2s_d2_pin, int i2s_d3_pin,
                        int i2s_ws_pin, int i2s_sclk_pin);

/**
 * @brief       Initialize I2S for APU
 *              Configures I2S device with specified sample rate
 *
 * @param[in]   sample_rate             I2S sample rate in Hz (e.g. 44100)
 */
void lib_apu_init_i2s(uint32_t sample_rate);

/**
 * @brief       Initialize APU device settings
 *
 * @param[in]   gain                    Audio gain value
 * @param[in]   channels                Channel mask (which channels to enable)
 */
void lib_apu_init_apu(uint16_t gain, uint8_t channels);

/**
 * @brief       Initialize PLL2 for APU
 *              Sets PLL2 frequency
 *
 * @param[in]   freq                    PLL2 frequency
 */
void lib_apu_init_clock(uint32_t freq);

/**
 * @brief       Initialize PLIC for APU interrupts
 *              Sets up interrupt controller with specified priority and handler
 *
 * @param[in]   priority               Interrupt priority (1-7, higher value = higher priority)
 */
void lib_apu_init_plic(uint32_t priority);

// Print all APU settings
void lib_apu_print_setting(void);
