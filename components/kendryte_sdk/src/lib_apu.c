#include <stdio.h>
#include <math.h>
#include "lib_apu.h"

// The vendor apu.c defines these with external linkage but omits them from apu.h -- forward-declared
// here rather than editing the kendryte-standalone-sdk submodule. Both are the *correct*,
// read-modify-write-safe siblings of functions this file used to call by mistake (see
// lib_apu_init_apu and lib_apu_set_source_mode below).
extern void apu_channel_enable(uint8_t channel_bit);
extern void apu_set_src_mode(uint8_t src_mode);

uint64_t logic_count;
uint64_t dir_logic_count;
uint64_t voc_logic_count;

#if APU_FFT_ENABLE
uint32_t APU_DIR_FFT_BUFFER[APU_DIR_CHANNEL_MAX]
				[APU_DIR_CHANNEL_SIZE]
	__attribute__((aligned(128)));
uint32_t APU_VOC_FFT_BUFFER[APU_VOC_CHANNEL_SIZE]
	__attribute__((aligned(128)));
#else
int16_t APU_DIR_BUFFER[APU_DIR_CHANNEL_MAX][APU_DIR_CHANNEL_SIZE]
	__attribute__((aligned(128)));
int16_t APU_VOC_BUFFER[APU_VOC_CHANNEL_SIZE]
	__attribute__((aligned(128)));
#endif


// CALLBACKS REGION START

int int_apu(void *ctx)
{
	apu_int_stat_t rdy_reg = apu->bf_int_stat_reg;

	if (rdy_reg.dir_search_data_rdy) {
		apu_dir_clear_int_state();

#if APU_FFT_ENABLE
		static int ch;

		ch = (ch + 1) % 16;
		for (uint32_t i = 0; i < 512; i++) { //
			uint32_t data = apu->sobuf_dma_rdata;

			APU_DIR_FFT_BUFFER[ch][i] = data;
		}
		if (ch == 0) { //
			dir_logic_count++;
		}
#else
		for (uint32_t ch = 0; ch < APU_DIR_CHANNEL_MAX; ch++) {
			for (uint32_t i = 0; i < 256; i++) { //
				uint32_t data = apu->sobuf_dma_rdata;

				APU_DIR_BUFFER[ch][i * 2 + 0] =
					data & 0xffff;
				APU_DIR_BUFFER[ch][i * 2 + 1] =
					(data >> 16) & 0xffff;
			}
		}
		dir_logic_count++;
#endif

	} else if (rdy_reg.voc_buf_data_rdy) {
		apu_voc_clear_int_state();

#if APU_FFT_ENABLE
		for (uint32_t i = 0; i < 512; i++) { //
			uint32_t data = apu->vobuf_dma_rdata;

			APU_VOC_FFT_BUFFER[i] = data;
		}
#else
		for (uint32_t i = 0; i < 256; i++) { //
			uint32_t data = apu->vobuf_dma_rdata;

			APU_VOC_BUFFER[i * 2 + 0] = data & 0xffff;
			APU_VOC_BUFFER[i * 2 + 1] = (data >> 16) & 0xffff;
		}
#endif

		voc_logic_count++;
	} else { //
		printf("[waring]: unknown %s interrupt cause.\n", __func__);
	}
	return 0;
}

#if APU_DMA_ENABLE
int int_apu_dir_dma(void *ctx)
{
	uint64_t chx_intstatus =
		dmac->channel[APU_DIR_DMA_CHANNEL].intstatus;
	if (chx_intstatus & 0x02) {
		dmac_wait_idle(APU_DIR_DMA_CHANNEL); // ME instead dmac_chanel_interrupt_clear(APU_DIR_DMA_CHANNEL);
		

#if APU_FFT_ENABLE
		static int ch;

		ch = (ch + 1) % 16;
		dmac->channel[APU_DIR_DMA_CHANNEL].dar =
			(uint64_t)APU_DIR_FFT_BUFFER[ch];
#else
		dmac->channel[APU_DIR_DMA_CHANNEL].dar =
			(uint64_t)APU_DIR_BUFFER;
#endif

		dmac->chen = 0x0101 << APU_DIR_DMA_CHANNEL;

#if APU_FFT_ENABLE
		if (ch == 0) { //
			dir_logic_count++;
		}
#else
		dir_logic_count++;
#endif

	} else {
		printf("[warning] unknown dma interrupt. %lx %lx\n",
		       dmac->intstatus, dmac->com_intstatus);
		printf("dir intstatus: %lx\n", chx_intstatus);

		dmac_wait_idle(APU_DIR_DMA_CHANNEL); // Me instead dmac_chanel_interrupt_clear();
		
	}
	return 0;
}


int int_apu_voc_dma(void *ctx)
{
	uint64_t chx_intstatus =
		dmac->channel[APU_VOC_DMA_CHANNEL].intstatus;

	if (chx_intstatus & 0x02) {
		dmac_wait_idle(APU_VOC_DMA_CHANNEL); // Me instead dmac_chanel_interrupt_clear(APU_VOC_DMA_CHANNEL);
		

#if APU_FFT_ENABLE
		dmac->channel[APU_VOC_DMA_CHANNEL].dar =
			(uint64_t)APU_VOC_FFT_BUFFER;
#else
		dmac->channel[APU_VOC_DMA_CHANNEL].dar =
			(uint64_t)APU_VOC_BUFFER;
#endif

		dmac->chen = 0x0101 << APU_VOC_DMA_CHANNEL;


		voc_logic_count++;

	} else {
		printf("[warning] unknown dma interrupt. %lx %lx\n",
		       dmac->intstatus, dmac->com_intstatus);
		printf("voc intstatus: %lx\n", chx_intstatus);

		dmac_wait_idle(APU_VOC_DMA_CHANNEL); // Me instead dmac_chanel_interrupt_clear(APU_VOC_DMA_CHANNEL);
	}
	return 0;
}
#endif

// CALLBACKS REGION END



// Initialize PLL2 for APU
// used in ALL
void lib_apu_init_clock(uint32_t freq) {
    sysctl_pll_set_freq(SYSCTL_PLL2, freq);
}


// Initialize FPIOA pins for I2S
// used in ALL
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

// Initialize PLIC (Platform-Level Interrupt Controller)
// used in ALL
void lib_apu_init_plic(uint32_t priority)
{
	//plic_init(); already called in maixpy_main.c
#if APU_DMA_ENABLE
	// dma
	plic_set_priority(IRQN_DMA0_INTERRUPT + APU_DIR_DMA_CHANNEL, priority);
	plic_irq_register(IRQN_DMA0_INTERRUPT + APU_DIR_DMA_CHANNEL,
			  int_apu_dir_dma, NULL);
	plic_irq_enable(IRQN_DMA0_INTERRUPT + APU_DIR_DMA_CHANNEL);
	// dma
	plic_set_priority(IRQN_DMA0_INTERRUPT + APU_VOC_DMA_CHANNEL, priority);
	plic_irq_register(IRQN_DMA0_INTERRUPT + APU_VOC_DMA_CHANNEL,
			  int_apu_voc_dma, NULL);
	plic_irq_enable(IRQN_DMA0_INTERRUPT + APU_VOC_DMA_CHANNEL);
#else
	plic_set_priority(IRQN_I2S0_INTERRUPT, priority);
	plic_irq_enable(IRQN_I2S0_INTERRUPT);
	plic_irq_register(IRQN_I2S0_INTERRUPT, int_apu, NULL);
#endif
}

// Initialize I2S device
// used in ALL
void lib_apu_init_i2s(uint32_t sample_rate) {
	/* I2s init */
    i2s_init(I2S_DEVICE_0, I2S_RECEIVER, 0x3);

    i2s_rx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_0,
            RESOLUTION_16_BIT, SCLK_CYCLES_32,
            TRIGGER_LEVEL_4, STANDARD_MODE);
    i2s_rx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_1,
            RESOLUTION_16_BIT, SCLK_CYCLES_32,
            TRIGGER_LEVEL_4, STANDARD_MODE);
    i2s_rx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_2,
            RESOLUTION_16_BIT, SCLK_CYCLES_32,
            TRIGGER_LEVEL_4, STANDARD_MODE);
    i2s_rx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_3,
            RESOLUTION_16_BIT, SCLK_CYCLES_32,
            TRIGGER_LEVEL_4, STANDARD_MODE);

    i2s_set_sample_rate(I2S_DEVICE_0, sample_rate);
}

// Initialize BF and array params
// used in ALL
void init_bf(void)
{
	uint16_t fir_prev_t[] = {
		0x020b, 0x0401, 0xff60, 0xfae2, 0xf860, 0x0022,
		0x10e6, 0x22f1, 0x2a98, 0x22f1, 0x10e6, 0x0022,
		0xf860, 0xfae2, 0xff60, 0x0401, 0x020b,
	};
	uint16_t fir_post_t[] = {
		0xf649, 0xe59e, 0xd156, 0xc615, 0xd12c, 0xf732,
		0x2daf, 0x5e03, 0x7151, 0x5e03, 0x2daf, 0xf732,
		0xd12c, 0xc615, 0xd156, 0xe59e, 0xf649,
	};

	uint16_t fir_neg_one[] = {
		0x8000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	};

	uint16_t fir_common[] = {
		0x03c3, 0x03c3, 0x03c3, 0x03c3, 0x03c3, 0x03c3,
		0x03c3, 0x03c3, 0x03c3, 0x03c3, 0x03c3, 0x03c3,
		0x03c3, 0x03c3, 0x03c3, 0x03c3, 0x03c3,
	};
//3cm
	// uint8_t offsets[16][8] = {
	// 	{0, 1, 5, 7, 7, 5, 1, 4, },
	// 	{0, 0, 3, 6, 7, 6, 3, 4, },
	// 	{1, 0, 2, 5, 8, 7, 4, 4, },
	// 	{2, 0, 1, 4, 7, 8, 6, 4, },
	// 	{4, 1, 0, 2, 6, 8, 7, 4, },
	// 	{5, 2, 0, 1, 4, 7, 8, 4, },
	// 	{6, 3, 0, 0, 2, 6, 7, 4, },
	// 	{7, 5, 1, 0, 1, 5, 7, 4, },
	// 	{7, 6, 3, 0, 0, 3, 6, 4, },
	// 	{8, 7, 4, 1, 0, 2, 5, 4, },
	// 	{7, 8, 6, 2, 0, 1, 4, 4, },
	// 	{6, 8, 7, 4, 1, 0, 2, 4, },
	// 	{4, 7, 8, 5, 2, 0, 1, 4, },
	// 	{3, 6, 7, 6, 3, 0, 0, 4, },
	// 	{1, 5, 7, 7, 5, 1, 0, 4, },
	// 	{0, 3, 6, 7, 6, 2, 0, 4, },
	// };

	apu_dir_set_prev_fir(fir_neg_one);
	apu_dir_set_post_fir(fir_neg_one);
	apu_voc_set_prev_fir(fir_neg_one);
	apu_voc_set_post_fir(fir_neg_one);

	apu_set_delay(4, 7, 1);
	apu_set_smpl_shift(APU_SMPL_SHIFT);
	apu_voc_set_saturation_limit(APU_SATURATION_VPOS_DEBUG,
					  APU_SATURATION_VNEG_DEBUG);
	apu_set_audio_gain(APU_AUDIO_GAIN_TEST);
	apu_voc_set_direction(0); //- temporally not managed (always takes from 0 dirrection)
	apu_set_channel_enabled(0x3f);
	apu_set_down_size(0, 0);

#if APU_FFT_ENABLE
	apu_set_fft_shift_factor(1, 0xaa);
#else
	apu_set_fft_shift_factor(0, 0);
#endif

	apu_set_interrupt_mask(APU_DMA_ENABLE, APU_DMA_ENABLE);
#if APU_DIR_ENABLE
	apu_dir_enable();
#endif
#if APU_VOC_ENABLE
	apu_voc_enable(1);
#else
	apu_voc_enable(0);
#endif
}


#if APU_DMA_ENABLE
void init_dma(void)
{
	// dmac enable dmac and interrupt
	/*
	union dmac_cfg_u dmac_cfg;
	dmac_cfg.data = readq(&dmac->cfg);
	dmac_cfg.cfg.dmac_en = 1;
	dmac_cfg.cfg.int_en = 1;
	writeq(dmac_cfg.data, &dmac->cfg);
	*/ // ME this logic is copy of dmac_enable(); called in void dmac_init(void); from maixpy_main.c

	sysctl_dma_select(SYSCTL_DMA_CHANNEL_0 + APU_DIR_DMA_CHANNEL,
			  SYSCTL_DMA_SELECT_I2S0_BF_DIR_REQ);
	sysctl_dma_select(SYSCTL_DMA_CHANNEL_0 + APU_VOC_DMA_CHANNEL,
			  SYSCTL_DMA_SELECT_I2S0_BF_VOICE_REQ);
}
#endif

void init_dma_ch(int ch, volatile uint32_t *src_reg, void *buffer,
		 size_t size_of_byte)
{
	printf("%s %d\n", __func__, ch);

	dmac->channel[ch].sar = (uint64_t)src_reg;
	dmac->channel[ch].dar = (uint64_t)buffer;
	dmac->channel[ch].block_ts = (size_of_byte / 4) - 1;
	dmac->channel[ch].ctl =
		(((uint64_t)1 << 47) | ((uint64_t)15 << 48)
		 | ((uint64_t)1 << 38) | ((uint64_t)15 << 39)
		 | ((uint64_t)3 << 18) | ((uint64_t)3 << 14)
		 | ((uint64_t)2 << 11) | ((uint64_t)2 << 8) | ((uint64_t)0 << 6)
		 | ((uint64_t)1 << 4) | ((uint64_t)1 << 2) | ((uint64_t)1));
	/*
	 * dmac->channel[ch].ctl = ((  wburst_len_en  ) |
	 *                        (    wburst_len   ) |
	 *                        (  rburst_len_en  ) |
	 *                        (    rburst_len   ) |
	 *                        (one transaction:d) |
	 *                        (one transaction:s) |
	 *                        (    dst width    ) |
	 *                        (    src width   ) |
	 *                        (    dinc,0 inc  )|
	 *                        (  sinc:1,no inc ));
	 */

	dmac->channel[ch].cfg = (((uint64_t)1 << 49) | ((uint64_t)ch << 44)
				 | ((uint64_t)ch << 39) | ((uint64_t)2 << 32));
	/*
	 * dmac->channel[ch].cfg = ((     prior       ) |
	 *                         (      dst_per    ) |
	 *                         (     src_per     )  |
	 *           (    peri to mem  ));
	 *  01: Reload
	 */

	dmac->channel[ch].intstatus_en = 0x2; // 0xFFFFFFFF;
	dmac->channel[ch].intclear = 0xFFFFFFFF;

	dmac->chen = 0x0101 << ch;
}



void lib_apu_init_all(int i2s_d0_pin, int i2s_d1_pin,
                  int i2s_d2_pin, int i2s_d3_pin,
                  int i2s_ws_pin, int i2s_sclk_pin) {
    
    printf("Init ALL start...\n");

    // 1. Set system clock
    lib_apu_init_clock(45158400UL);
    printf("Clock init done.\n");
    msleep(100);

    // 2. Disable IRQ
    sysctl_disable_irq();
    printf("IRQ disabled.\n");
    msleep(100);

    // 3. Initialize FPIOA pins
    lib_apu_init_fpioa(i2s_d0_pin, i2s_d1_pin, i2s_d2_pin, i2s_d3_pin, i2s_ws_pin, i2s_sclk_pin);
    printf("FPIOA init done.\n");
    msleep(100);

    // 4. Initialize interrupts and callbacks
    lib_apu_init_plic(4);
    printf("PLIC init done.\n");
    msleep(100);

    // 5. Initialize I2S
    lib_apu_init_i2s(44100);
    printf("I2S init done.\n");
    msleep(100);

    // 6. Initialize APU
    init_bf();
    printf("APU BF init done.\n");
    msleep(100);

    // 7. Initialize DMA
    if (APU_DMA_ENABLE) {
        #if APU_DMA_ENABLE
		init_dma();
        #endif
#if APU_FFT_ENABLE
		init_dma_ch(APU_DIR_DMA_CHANNEL,
			    &apu->sobuf_dma_rdata,
			    APU_DIR_FFT_BUFFER[0], 512 * 4);
		init_dma_ch(APU_VOC_DMA_CHANNEL,
			    &apu->vobuf_dma_rdata, APU_VOC_FFT_BUFFER,
			    512 * 4);
#else
		init_dma_ch(APU_DIR_DMA_CHANNEL,
			    &apu->sobuf_dma_rdata, APU_DIR_BUFFER,
			    512 * 16 * 2);
		init_dma_ch(APU_VOC_DMA_CHANNEL,
			    &apu->vobuf_dma_rdata, APU_VOC_BUFFER,
			    512 * 2);
#endif
        printf("DMA init done.\n");
        msleep(100);
	}

    // 8. Enable IRQ
    sysctl_enable_irq();
    printf("Enable IRQ.\n");
    msleep(100);

    //apu_print_setting();
    //msleep(500);

    printf("Init ALL done!\n");
    msleep(100);
}


void lib_apu_init_led(int sk9822_dat_pin, int sk9822_clk_pin) {
    
    printf("Init LED start...\n");

    fpioa_set_function(sk9822_dat_pin, FUNC_GPIOHS0 + SK9822_DAT_GPIONUM);
    fpioa_set_function(sk9822_clk_pin, FUNC_GPIOHS0 + SK9822_CLK_GPIONUM);
    printf("FPIOA init done.\n");
    msleep(100);

    sipeed_init_mic_array_led();
    printf("Init ALL done!\n");
    msleep(100);
}




// LED LOGIC START

void lib_apu_set_led(uint32_t degree, uint32_t color, uint32_t delay) {
    
    uint8_t led_num = (uint8_t)round(degree * 12 / 360);

    uint32_t led_color;

    if (color == 0) {
        led_color = 0xffeec900; //LIGHT BLUE
    } else if (color == 1) {
        led_color = 0xffff0000; //BLUE
    } else if (color == 2) {
        led_color = 0xff00ff00; //GREEN
    } else if (color == 3) {
        led_color = 0xff0000ff; //RED
    } else  {
        led_color = 0xffff0000;
    }

    led_color |= 0xe0000000;

    uint8_t index;
    int led_colors[12] = {0};
    for (index = 0; index < 12; index++)
    {
        led_colors[index] = index == led_num ? led_color : 0xe0000000;
    }
    
    sk9822_start_frame();
    for (index = 0; index < 12; index++)
    {
        sk9822_send_data(led_colors[index]);
    }
    sk9822_stop_frame();
    if (delay > 0) {
        msleep(delay);
    }
    
}

// LED LOGIC END






// LOOP LOGIC START

int dir_logic(apu_dir_result_t *result)
{
	int32_t dir_max = 0;
	uint16_t contex = 0;

	logic_count++;
	if (logic_count > 10) {
		logic_count = 0;
	}

	for (size_t ch = 0; ch < APU_DIR_CHANNEL_MAX; ch++) { //
		// int64_t + reset per channel: the previous int32_t accumulator (a) was never reset
		// between channels, letting each channel's comparison value carry a small (~1/512)
		// contamination from the previous channel's leftover, and (b) could overflow int32_t
		// on loud/near-clipping input (512 samples near full-scale int16 sums to ~5.5e11,
		// vs. int32_t max ~2.1e9).
		int64_t dir_sum = 0;

        for (size_t i = 0; i < APU_DIR_CHANNEL_SIZE; i++) { //
                dir_sum += (int64_t)APU_DIR_BUFFER[ch][i] * (int64_t)APU_DIR_BUFFER[ch][i];
        }
        dir_sum = dir_sum / APU_DIR_CHANNEL_SIZE;
        result->sector_power[ch] = (int32_t)dir_sum; // safe: max ~1.07e9 post-division, well within int32_t
        if(dir_sum > dir_max){
            dir_max = (int32_t)dir_sum;
            contex = ch;
        }

    }

    for (size_t i = 0; i < APU_DIR_CHANNEL_SIZE; i++) { //
        result->samples[i] = APU_DIR_BUFFER[contex][i];
    }

    if (logic_count == 100) {
        printf("--- %d   %d\n", contex, dir_max);
    }
    
    result->direction = (en_bf_dir_t)contex; // Assign a direction
    result->power = dir_max;                 // Assign a power value
	apu_dir_enable();
	return 0;
}

int voc_logic(apu_dir_result_t *result)
{
	
	result->voc_dir = (en_bf_dir_t)(apu->bf_ch_cfg_reg.bf_target_dir);

	for (size_t i = 0; i < APU_DIR_CHANNEL_SIZE; i++) { //
        result->voc_samples[i] = APU_VOC_BUFFER[i];
    }

	return 0;
}

apu_dir_result_t lib_apu_get_direction(void)
{
	uint8_t waiting = 1;
    apu_dir_result_t result;

    while (waiting) {
		if (dir_logic_count > 0) {
			dir_logic(&result);
            waiting = 0;
			while (--dir_logic_count != 0) {
				printf("[warning]: %s, restart before prev callback has end\n",
				       "dir_logic");
			}
		}
		if (voc_logic_count > 0) {
			voc_logic(&result);
			while (--voc_logic_count != 0) {
				printf("[warning]: %s, restart before prev callback has end\n",
				       "voc_logic");
			}
		}
	}
	return result;
}

// LOOP LOGIC END
















// Initialize APU device
void lib_apu_init_apu(uint16_t gain, uint8_t channels) {
    // Configure APU gain and channels.
    // Note: apu_channel_enable() (not the similarly-named apu_set_channel_enabled()) -- the latter
    // writes an uninitialized local apu_ch_cfg_t straight to hardware, leaving data_src_mode and its
    // write-enable bit as stack garbage. apu_channel_enable() reads the register first and clears the
    // other write-enable bits explicitly, so this call can't have side effects on unrelated fields.
    apu_set_audio_gain(gain);
    apu_channel_enable(channels);
}

void lib_apu_start_direction_detection(void) {
    // Clear any previous interrupt state
    // apu_dir_clear_int_state();
    // Start direction detection
    apu_dir_enable();
}

uint8_t lib_apu_dir_is_ready(void) {
    //volatile apu_reg_t* apu_reg = (volatile apu_reg_t*)0x50250200;
    return apu->bf_int_stat_reg.dir_search_data_rdy;
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
    // Was: two separate bitfield writes through a raw pointer, with no initial read and no
    // clearing of the other we_* bits. we_data_src_mode is a one-shot, self-clearing write-enable,
    // so the second write (setting data_src_mode) landed with we_data_src_mode already cleared by
    // the first write -- hardware discarded the intended value. apu_set_src_mode() does the correct
    // single atomic read-modify-write (read whole register, clear the other we_* bits, set both
    // we_data_src_mode and data_src_mode together, write back once).
    apu_set_src_mode(mode & 0x1);
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

// The previous "register access issue" these four getters were stubbed out for was simply wrong
// field/member names: apu_fir_coef_t has fir_tap0/fir_tap1, not a single .coef member, and the last
// getter referenced a register (bf_voice_post_fir_coef) that doesn't exist -- the real one is
// bf_post_fir1_coef. Fixed below, unpacking each 9-register bank back into APU_FIR_TAP_COUNT (17)
// taps using the exact inverse of how the *_set_*_fir setters pack them (fir_coef[i*2]/[i*2+1] for
// i in 0..8, with i==8's second tap skipped -- it's always forced to 0 on write, and writing/reading
// index 17 would be one past the 17-element buffer).

void lib_apu_get_dir_pre_fir(uint16_t* coefficients)
{
    for (int i = 0; i < 9; i++) {
        coefficients[i * 2] = apu->bf_pre_fir0_coef[i].fir_tap0;
        if (i < 8) {
            coefficients[i * 2 + 1] = apu->bf_pre_fir0_coef[i].fir_tap1;
        }
    }
}

void lib_apu_get_dir_post_fir(uint16_t* coefficients)
{
    for (int i = 0; i < 9; i++) {
        coefficients[i * 2] = apu->bf_post_fir0_coef[i].fir_tap0;
        if (i < 8) {
            coefficients[i * 2 + 1] = apu->bf_post_fir0_coef[i].fir_tap1;
        }
    }
}

void lib_apu_get_voice_pre_fir(uint16_t* coefficients)
{
    for (int i = 0; i < 9; i++) {
        coefficients[i * 2] = apu->bf_pre_fir1_coef[i].fir_tap0;
        if (i < 8) {
            coefficients[i * 2 + 1] = apu->bf_pre_fir1_coef[i].fir_tap1;
        }
    }
}

void lib_apu_get_voice_post_fir(uint16_t* coefficients)
{
    for (int i = 0; i < 9; i++) {
        coefficients[i * 2] = apu->bf_post_fir1_coef[i].fir_tap0;
        if (i < 8) {
            coefficients[i * 2 + 1] = apu->bf_post_fir1_coef[i].fir_tap1;
        }
    }
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

void lib_apu_print_setting(void) 
{
    // Call the APU driver's print_setting function
    apu_print_setting();
}