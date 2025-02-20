#ifndef _LIB_APU_H
#define _LIB_APU_H

#include <stdint.h>
#include "apu.h"

// Initialize APU with specified gain and channel mask
// pins: -1 means don't configure that pin
void lib_apu_init(uint16_t gain, uint8_t channels, 
                  int i2s_d0_pin, int i2s_d1_pin, 
                  int i2s_d2_pin, int i2s_d3_pin,
                  int i2s_ws_pin, int i2s_sclk_pin);

// Configure APU direction
void lib_apu_configure_direction(float radius, uint8_t mic_num_a_circle, uint8_t center);

// Start direction detection
void lib_apu_start_direction_detection(void);

// Get current direction with confidence level
typedef struct {
    en_bf_dir_t direction;  // Detected direction
    uint16_t confidence;    // Confidence level (0-1024)
    uint16_t power;        // Signal power in the detected direction
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

// Set direction detection pre-FIR coefficients
void lib_apu_set_dir_pre_fir(const uint16_t* coefficients);

// Set direction detection post-FIR coefficients
void lib_apu_set_dir_post_fir(const uint16_t* coefficients);

// Set voice output pre-FIR coefficients
void lib_apu_set_voice_pre_fir(const uint16_t* coefficients);

// Set voice output post-FIR coefficients
void lib_apu_set_voice_post_fir(const uint16_t* coefficients);

// Get direction detection pre-FIR coefficients
void lib_apu_get_dir_pre_fir(uint16_t* coefficients);

// Get direction detection post-FIR coefficients
void lib_apu_get_dir_post_fir(uint16_t* coefficients);

// Get voice output pre-FIR coefficients
void lib_apu_get_voice_pre_fir(uint16_t* coefficients);

// Get voice output post-FIR coefficients
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

#endif // _LIB_APU_H