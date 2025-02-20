#include <stdio.h>
#include "lib_apu.h"
#include "sysctl.h"
#include "apu.h"
#include "i2s.h"
#include "plic.h"
#include "dmac.h"
#include "fpioa.h"

// Aligned buffer for APU data (4 channels)
static int16_t APU_DIR_BUFFER[4][512] __attribute__((aligned(128)));

static int apu_dir_int_handler(void* ctx)
{
    apu_dir_clear_int_state();
    return 0;
}

// Initialize FPIOA pins for I2S
void lib_apu_init_fpioa(int i2s_d0_pin, int i2s_d1_pin,
                              int i2s_d2_pin, int i2s_d3_pin,
                              int i2s_ws_pin, int i2s_sclk_pin) {
    fpioa_init();
    // Configure I2S pins using FPIOA
    if (i2s_d0_pin >= 0)
        fpioa_set_function(i2s_d0_pin, FUNC_I2S0_IN_D0);
    if (i2s_d1_pin >= 0)
        fpioa_set_function(i2s_d1_pin, FUNC_I2S0_IN_D1);
    if (i2s_d2_pin >= 0)
        fpioa_set_function(i2s_d2_pin, FUNC_I2S0_IN_D2);
    if (i2s_d3_pin >= 0)
        fpioa_set_function(i2s_d3_pin, FUNC_I2S0_IN_D3);
    if (i2s_ws_pin >= 0)
        fpioa_set_function(i2s_ws_pin, FUNC_I2S0_WS);
    if (i2s_sclk_pin >= 0)
        fpioa_set_function(i2s_sclk_pin, FUNC_I2S0_SCLK);
}

// Initialize I2S device
void lib_apu_init_i2s(uint32_t sample_rate) {
    // Initialize I2S0 for 4 channels (this also enables the I2S clock)
    i2s_init(I2S_DEVICE_0, I2S_RECEIVER, 0xF);
    
    // Configure I2S channels
    for (int i = 0; i < 4; i++) {
        i2s_rx_channel_config(I2S_DEVICE_0,
                            I2S_CHANNEL_0 + i,      // Channel number
                            RESOLUTION_16_BIT,       // 16-bit resolution
                            SCLK_CYCLES_32,         // Word select size (32 cycles)
                            TRIGGER_LEVEL_4,        // DMA trigger level
                            STANDARD_MODE);         // Standard I2S mode
    }

    i2s_set_sample_rate(I2S_DEVICE_0, sample_rate);
}

// Initialize APU device
void lib_apu_init_apu(uint16_t gain, uint8_t channels) {
    // Configure APU gain and channels
    apu_set_audio_gain(gain);
    apu_set_channel_enabled(channels);
}

// Initialize PLIC (Platform-Level Interrupt Controller)
void lib_apu_init_plic(uint32_t priority) {
    //plic_init();
    plic_set_priority(IRQN_I2S0_INTERRUPT, priority);
    plic_irq_register(IRQN_I2S0_INTERRUPT, apu_dir_int_handler, NULL);
    plic_irq_enable(IRQN_I2S0_INTERRUPT);
}

// Initialize PLL2 for APU
void lib_apu_init_clock(uint32_t freq) {
    sysctl_pll_set_freq(SYSCTL_PLL2, freq);
}

void lib_apu_init(uint16_t gain, uint8_t channels,
                  int i2s_d0_pin, int i2s_d1_pin,
                  int i2s_d2_pin, int i2s_d3_pin,
                  int i2s_ws_pin, int i2s_sclk_pin) {
    // 1. Set system clock
    lib_apu_init_clock(45158400UL);
    
    // 2. Initialize FPIOA pins
    lib_apu_init_fpioa(i2s_d0_pin, i2s_d1_pin, i2s_d2_pin, i2s_d3_pin,
                       i2s_ws_pin, i2s_sclk_pin);

    // 5. Initialize PLIC with default priority 4
    lib_apu_init_plic(4);
    
    // 3. Initialize I2S with default 44.1kHz
    lib_apu_init_i2s(44100);
    
    // 4. Initialize APU
    lib_apu_init_apu(gain, channels);
    
}

void lib_apu_start_direction_detection(void) {
    // Clear any previous interrupt state
    apu_dir_clear_int_state();
    // Start direction detection
    apu_dir_enable();
}

uint8_t lib_apu_dir_is_ready(void) {
    volatile apu_reg_t* apu_reg = (volatile apu_reg_t*)0x50250200;
    return apu_reg->bf_int_stat_reg.dir_search_data_rdy;
}

apu_dir_result_t lib_apu_get_direction(void) {
    apu_dir_result_t result = {0};
    volatile apu_reg_t* apu_reg = (volatile apu_reg_t*)0x50250200;
    
    // Read direction from channel config register
    result.direction = (en_bf_dir_t)(apu_reg->bf_ch_cfg_reg.bf_target_dir);
    
    // Extract confidence (10 bits) from interrupt status
    result.confidence = (apu_reg->bf_int_stat_reg.dir_search_data_rdy) ? 1023 : 0;
    
    // Extract power from saturation counter
    result.power = apu_reg->saturation_counter;
    
    return result;
}

void lib_apu_dir_clear_ready(void) {
    apu_dir_clear_int_state();
}

void lib_apu_enable_voice_output(en_bf_dir_t direction) {
    apu_voc_set_direction(direction);
    apu_voc_enable(1);
}

void lib_apu_disable_voice_output(void) {
    apu_voc_enable(0);
}

void lib_apu_set_source_mode(uint8_t mode)
{
    volatile apu_reg_t* apu_reg = (volatile apu_reg_t*)0x50250200;
    
    // Set the write enable bit and the mode
    apu_reg->bf_ch_cfg_reg.we_data_src_mode = 1;
    apu_reg->bf_ch_cfg_reg.data_src_mode = mode & 0x1;
}

void lib_apu_reset(void) {
    apu_dir_reset();
    apu_voc_reset();
}

void lib_apu_configure_direction(float radius, uint8_t mic_num_a_circle, uint8_t center)
{
    // Reset direction detection first
    apu_dir_reset();
    
    // Configure delays for each direction based on microphone array geometry
    apu_set_delay(radius, mic_num_a_circle, center);
}

// FIR filter functions using existing APU driver functions
void lib_apu_set_dir_pre_fir(const uint16_t* coefficients)
{
    apu_dir_set_prev_fir((uint16_t*)coefficients);
}

void lib_apu_set_dir_post_fir(const uint16_t* coefficients)
{
    apu_dir_set_post_fir((uint16_t*)coefficients);
}

void lib_apu_set_voice_pre_fir(const uint16_t* coefficients)
{
    apu_voc_set_prev_fir((uint16_t*)coefficients);
}

void lib_apu_set_voice_post_fir(const uint16_t* coefficients)
{
    apu_voc_set_post_fir((uint16_t*)coefficients);
}

void lib_apu_get_dir_pre_fir(uint16_t* coefficients)
{
    /*
    volatile apu_reg_t* apu_reg = (volatile apu_reg_t*)0x50250200;
    for (int i = 0; i < 9; i++) {
        coefficients[i] = apu_reg->bf_pre_fir0_coef[i].coef;
    }
    */
}

void lib_apu_get_dir_post_fir(uint16_t* coefficients)
{
    /*
    volatile apu_reg_t* apu_reg = (volatile apu_reg_t*)0x50250200;
    for (int i = 0; i < 9; i++) {
        coefficients[i] = apu_reg->bf_post_fir0_coef[i].coef;
    }
    */
}

void lib_apu_get_voice_pre_fir(uint16_t* coefficients)
{
    /*
    volatile apu_reg_t* apu_reg = (volatile apu_reg_t*)0x50250200;
    for (int i = 0; i < 9; i++) {
        coefficients[i] = apu_reg->bf_pre_fir1_coef[i].coef;
    }
    */
}

void lib_apu_get_voice_post_fir(uint16_t* coefficients)
{
    // Implementation commented out until register access issue is resolved
    /*
    volatile apu_reg_t* apu_reg = (volatile apu_reg_t*)0x50250200;
    for (int i = 0; i < 9; i++) {
        coefficients[i] = apu_reg->bf_voice_post_fir_coef[i].coef;
    }
    */
}

void lib_apu_dir_set_down_size(uint8_t dir_dwn_size)
{
    apu_dir_set_down_size(dir_dwn_size);
}

void lib_apu_voc_set_down_size(uint8_t voc_dwn_size)
{
    apu_voc_set_down_size(voc_dwn_size);
}

void lib_apu_dir_set_interrupt_mask(uint8_t dir_int_mask)
{
    apu_dir_set_interrupt_mask(dir_int_mask);
}

void lib_apu_voc_set_interrupt_mask(uint8_t voc_int_mask)
{
    apu_voc_set_interrupt_mask(voc_int_mask);
}

void lib_apu_dir_clear_int_state(void)
{
    apu_dir_clear_int_state();
}

void lib_apu_voc_clear_int_state(void)
{
    apu_voc_clear_int_state();
}

void lib_apu_voc_reset_saturation_counter(void)
{
    apu_voc_reset_saturation_counter();
}

uint32_t lib_apu_voc_get_saturation_counter(void)
{
    return apu_voc_get_saturation_counter();
}

void lib_apu_voc_set_saturation_limit(uint16_t upper, uint16_t bottom)
{
    apu_voc_set_saturation_limit(upper, bottom);
}

uint32_t lib_apu_voc_get_saturation_limit(void)
{
    return apu_voc_get_saturation_limit();
}