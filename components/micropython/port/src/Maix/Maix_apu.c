/*
* Copyright 2019 Sipeed Co.,Ltd.

* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <stdio.h>
#include <string.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "modMaix.h"
#include "lib_apu.h"
//#include "apu_demo.h"

const mp_obj_type_t Maix_apu_type;

// Forward declarations
STATIC mp_obj_t Maix_apu_init_all(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
STATIC mp_obj_t Maix_apu_init_led(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
STATIC mp_obj_t Maix_apu_set_led(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
STATIC mp_obj_t Maix_apu_init_clock(mp_obj_t freq_obj);
STATIC mp_obj_t Maix_apu_init_fpioa(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
STATIC mp_obj_t Maix_apu_init_i2s(mp_obj_t sample_rate_obj);
STATIC mp_obj_t Maix_apu_init_apu(mp_obj_t gain_obj, mp_obj_t channels_obj);
STATIC mp_obj_t Maix_apu_init_plic(mp_obj_t priority_obj);
STATIC mp_obj_t Maix_apu_configure_direction(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
STATIC mp_obj_t Maix_apu_start_direction_detection(void);
STATIC mp_obj_t Maix_apu_dir_is_ready(void);
STATIC mp_obj_t Maix_apu_get_direction(void);
STATIC mp_obj_t Maix_apu_dir_clear_ready(void);
STATIC mp_obj_t Maix_apu_enable_voice_output(mp_obj_t direction_obj);
STATIC mp_obj_t Maix_apu_disable_voice_output(void);
STATIC mp_obj_t Maix_apu_set_source_mode(mp_obj_t mode_obj);
STATIC mp_obj_t Maix_apu_reset(void);
STATIC mp_obj_t Maix_apu_set_dir_pre_fir(mp_obj_t coefficients_obj);
STATIC mp_obj_t Maix_apu_set_dir_post_fir(mp_obj_t coefficients_obj);
STATIC mp_obj_t Maix_apu_set_voice_pre_fir(mp_obj_t coefficients_obj);
STATIC mp_obj_t Maix_apu_set_voice_post_fir(mp_obj_t coefficients_obj);
STATIC mp_obj_t Maix_apu_get_dir_pre_fir(void);
STATIC mp_obj_t Maix_apu_get_dir_post_fir(void);
STATIC mp_obj_t Maix_apu_get_voice_pre_fir(void);
STATIC mp_obj_t Maix_apu_get_voice_post_fir(void);
STATIC mp_obj_t Maix_apu_dir_set_down_size(mp_obj_t size_obj);
STATIC mp_obj_t Maix_apu_voc_set_down_size(mp_obj_t size_obj);
STATIC mp_obj_t Maix_apu_dir_set_interrupt_mask(mp_obj_t mask_obj);
STATIC mp_obj_t Maix_apu_voc_set_interrupt_mask(mp_obj_t mask_obj);
STATIC mp_obj_t Maix_apu_dir_clear_int_state(void);
STATIC mp_obj_t Maix_apu_voc_clear_int_state(void);
STATIC mp_obj_t Maix_apu_voc_reset_saturation_counter(void);
STATIC mp_obj_t Maix_apu_voc_get_saturation_counter(void);
STATIC mp_obj_t Maix_apu_voc_set_saturation_limit(mp_obj_t upper_obj, mp_obj_t bottom_obj);
STATIC mp_obj_t Maix_apu_voc_get_saturation_limit(void);
STATIC mp_obj_t Maix_apu_print_settings(void);
/*STATIC mp_obj_t Maix_apu_demo_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
STATIC mp_obj_t Maix_apu_demo_run(void);*/


// APU initialization arguments
STATIC const mp_arg_t Maix_apu_init_all_args[] = {
    { MP_QSTR_i2s_d0,   MP_ARG_INT, {.u_int = 23} },
    { MP_QSTR_i2s_d1,   MP_ARG_INT, {.u_int = 22} },
    { MP_QSTR_i2s_d2,   MP_ARG_INT, {.u_int = 21} },
    { MP_QSTR_i2s_d3,   MP_ARG_INT, {.u_int = 20} },
    { MP_QSTR_i2s_ws,   MP_ARG_INT, {.u_int = 19} },
    { MP_QSTR_i2s_sclk, MP_ARG_INT, {.u_int = 18} },
};

// LED initialization arguments
STATIC const mp_arg_t Maix_apu_init_led_args[] = {
    { MP_QSTR_sk9822_dat, MP_ARG_INT, {.u_int = 24} },
    { MP_QSTR_sk9822_clk, MP_ARG_INT, {.u_int = 25} },
};

// APU initialization function
STATIC mp_obj_t Maix_apu_init_all(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_arg_val_t args[MP_ARRAY_SIZE(Maix_apu_init_all_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(Maix_apu_init_all_args), Maix_apu_init_all_args, args);

    // Get the arguments
    int i2s_d0 = args[0].u_int;
    int i2s_d1 = args[1].u_int;
    int i2s_d2 = args[2].u_int;
    int i2s_d3 = args[3].u_int;
    int i2s_ws = args[4].u_int;
    int i2s_sclk = args[5].u_int;

    // Initialize APU with default settings
    lib_apu_init_all(i2s_d0, i2s_d1, i2s_d2, i2s_d3, i2s_ws, i2s_sclk);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_apu_init_all_obj, 0, Maix_apu_init_all);

// LED initialization function
STATIC mp_obj_t Maix_apu_init_led(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_arg_val_t args[MP_ARRAY_SIZE(Maix_apu_init_led_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(Maix_apu_init_led_args), Maix_apu_init_led_args, args);

    // Get the arguments
    int sk9822_dat = args[0].u_int;
    int sk9822_clk = args[1].u_int;

    // Initialize APU with default settings
    lib_apu_init_led(sk9822_dat, sk9822_clk);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_apu_init_led_obj, 0, Maix_apu_init_led);


STATIC mp_obj_t Maix_apu_set_led(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_degree,        MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_color,         MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_delay,         MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t degree = args[0].u_int;
    uint32_t color  = args[1].u_int;
    uint32_t delay  = args[2].u_int;

    lib_apu_set_led(degree, color, delay);    
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_apu_set_led_obj, 0, Maix_apu_set_led);

// Initialize APU clock
STATIC mp_obj_t Maix_apu_init_clock(mp_obj_t freq_obj) {
    uint32_t freq = mp_obj_get_int(freq_obj);
    lib_apu_init_clock(freq);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_init_clock_obj, Maix_apu_init_clock);

// Initialize APU device settings
STATIC mp_obj_t Maix_apu_init_apu(mp_obj_t gain_obj, mp_obj_t channels_obj) {
    uint16_t gain = mp_obj_get_int(gain_obj);
    uint8_t channels = mp_obj_get_int(channels_obj);
    lib_apu_init_apu(gain, channels);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(Maix_apu_init_apu_obj, Maix_apu_init_apu);

// Initialize PLIC for APU interrupts
STATIC mp_obj_t Maix_apu_init_plic(mp_obj_t priority_obj) {
    uint32_t priority = mp_obj_get_int(priority_obj);
    if (priority < 1 || priority > 7) {
        mp_raise_ValueError("[MAIXPY]APU: Priority must be between 1 and 7");
    }
    lib_apu_init_plic(priority);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_init_plic_obj, Maix_apu_init_plic);

// Configure APU direction detection
STATIC mp_obj_t Maix_apu_configure_direction(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_radius,          MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_mic_num_a_circle, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_center,          MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    float radius = mp_obj_get_float(args[0].u_obj);
    uint8_t mic_num = mp_obj_get_int(args[1].u_obj);
    uint8_t center = mp_obj_get_int(args[2].u_obj);

    lib_apu_configure_direction(radius, mic_num, center);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_apu_configure_direction_obj, 3, Maix_apu_configure_direction);

// Start direction detection
STATIC mp_obj_t Maix_apu_start_direction_detection(void) {
    lib_apu_start_direction_detection();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_start_direction_detection_obj, Maix_apu_start_direction_detection);

// Check if direction detection is ready
STATIC mp_obj_t Maix_apu_dir_is_ready(void) {
    return mp_obj_new_bool(lib_apu_dir_is_ready());
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_dir_is_ready_obj, Maix_apu_dir_is_ready);

// Clear direction detection ready flag
STATIC mp_obj_t Maix_apu_dir_clear_ready(void) {
    lib_apu_dir_clear_ready();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_dir_clear_ready_obj, Maix_apu_dir_clear_ready);

// Get direction with confidence level
STATIC mp_obj_t Maix_apu_get_direction(void) {
    apu_dir_result_t result = lib_apu_get_direction();
    
    // Create a new list for samples
    mp_obj_list_t *samples_list = MP_OBJ_TO_PTR(mp_obj_new_list(0, NULL));
    for(int i = 0; i < APU_DIR_CHANNEL_SIZE; i++) {
        mp_obj_list_append(samples_list, mp_obj_new_int(result.samples[i]));
    }

    // Create a new list for VOC samples
    mp_obj_list_t *voc_samples_list = MP_OBJ_TO_PTR(mp_obj_new_list(0, NULL));
    for(int i = 0; i < APU_DIR_CHANNEL_SIZE; i++) {
        mp_obj_list_append(voc_samples_list, mp_obj_new_int(result.voc_samples[i]));
    }
    
    mp_obj_t tuple[5] = {
        mp_obj_new_int(result.direction),
        mp_obj_new_int(result.power),
        mp_obj_new_int(result.voc_dir),
        MP_OBJ_FROM_PTR(samples_list),
        MP_OBJ_FROM_PTR(voc_samples_list),
        
    };
    return mp_obj_new_tuple(5, tuple);
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_get_direction_obj, Maix_apu_get_direction);

// Enable voice output for a specific direction
STATIC mp_obj_t Maix_apu_enable_voice_output(mp_obj_t direction_obj) {
    en_bf_dir_t direction = mp_obj_get_int(direction_obj);
    if (direction < APU_DIR0 || direction > APU_DIR15) {
        mp_raise_ValueError("[MAIXPY]APU: Invalid direction value");
    }
    lib_apu_enable_voice_output(direction);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_enable_voice_output_obj, Maix_apu_enable_voice_output);

// Disable voice output
STATIC mp_obj_t Maix_apu_disable_voice_output(void) {
    lib_apu_disable_voice_output();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_disable_voice_output_obj, Maix_apu_disable_voice_output);

// Set audio source mode
STATIC mp_obj_t Maix_apu_set_source_mode(mp_obj_t mode_obj) {
    uint8_t mode = mp_obj_get_int(mode_obj);
    if (mode > 1) {
        mp_raise_ValueError("[MAIXPY]APU: Invalid source mode value");
    }
    lib_apu_set_source_mode(mode);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_set_source_mode_obj, Maix_apu_set_source_mode);

// Reset APU
STATIC mp_obj_t Maix_apu_reset(void) {
    lib_apu_reset();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_reset_obj, Maix_apu_reset);

// Helper function to convert Python list to FIR coefficients
STATIC void list_to_fir_coefficients(mp_obj_t list_obj, uint16_t* coefficients) {
    size_t len;
    mp_obj_t *items;
    mp_obj_get_array(list_obj, &len, &items);
    
    if (len != 16) {
        mp_raise_ValueError("[MAIXPY]APU: FIR coefficients must be a list of 16 integers");
    }
    
    for (int i = 0; i < 16; i++) {
        coefficients[i] = mp_obj_get_int(items[i]);
    }
}

// Helper function to convert FIR coefficients to Python list
STATIC mp_obj_t fir_coefficients_to_list(const uint16_t* coefficients) {
    mp_obj_t list = mp_obj_new_list(16, NULL);
    for (int i = 0; i < 16; i++) {
        mp_obj_list_store(list, MP_OBJ_NEW_SMALL_INT(i), MP_OBJ_NEW_SMALL_INT(coefficients[i]));
    }
    return list;
}

// Set direction detection pre-FIR coefficients
STATIC mp_obj_t Maix_apu_set_dir_pre_fir(mp_obj_t coefficients_obj) {
    uint16_t coefficients[16];
    list_to_fir_coefficients(coefficients_obj, coefficients);
    lib_apu_set_dir_pre_fir(coefficients);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_set_dir_pre_fir_obj, Maix_apu_set_dir_pre_fir);

// Set direction detection post-FIR coefficients
STATIC mp_obj_t Maix_apu_set_dir_post_fir(mp_obj_t coefficients_obj) {
    uint16_t coefficients[16];
    list_to_fir_coefficients(coefficients_obj, coefficients);
    lib_apu_set_dir_post_fir(coefficients);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_set_dir_post_fir_obj, Maix_apu_set_dir_post_fir);

// Set voice output pre-FIR coefficients
STATIC mp_obj_t Maix_apu_set_voice_pre_fir(mp_obj_t coefficients_obj) {
    uint16_t coefficients[16];
    list_to_fir_coefficients(coefficients_obj, coefficients);
    lib_apu_set_voice_pre_fir(coefficients);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_set_voice_pre_fir_obj, Maix_apu_set_voice_pre_fir);

// Set voice output post-FIR coefficients
STATIC mp_obj_t Maix_apu_set_voice_post_fir(mp_obj_t coefficients_obj) {
    uint16_t coefficients[16];
    list_to_fir_coefficients(coefficients_obj, coefficients);
    lib_apu_set_voice_post_fir(coefficients);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_set_voice_post_fir_obj, Maix_apu_set_voice_post_fir);

// Get direction detection pre-FIR coefficients
STATIC mp_obj_t Maix_apu_get_dir_pre_fir(void) {
    uint16_t coefficients[16];
    lib_apu_get_dir_pre_fir(coefficients);
    return fir_coefficients_to_list(coefficients);
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_get_dir_pre_fir_obj, Maix_apu_get_dir_pre_fir);

// Get direction detection post-FIR coefficients
STATIC mp_obj_t Maix_apu_get_dir_post_fir(void) {
    uint16_t coefficients[16];
    lib_apu_get_dir_post_fir(coefficients);
    return fir_coefficients_to_list(coefficients);
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_get_dir_post_fir_obj, Maix_apu_get_dir_post_fir);

// Get voice output pre-FIR coefficients
STATIC mp_obj_t Maix_apu_get_voice_pre_fir(void) {
    uint16_t coefficients[16];
    lib_apu_get_voice_pre_fir(coefficients);
    return fir_coefficients_to_list(coefficients);
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_get_voice_pre_fir_obj, Maix_apu_get_voice_pre_fir);

// Get voice output post-FIR coefficients
STATIC mp_obj_t Maix_apu_get_voice_post_fir(void) {
    uint16_t coefficients[9];
    lib_apu_get_voice_post_fir(coefficients);
    return fir_coefficients_to_list(coefficients);
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_get_voice_post_fir_obj, Maix_apu_get_voice_post_fir);

// Set down-sizing ratio for direction searching
STATIC mp_obj_t Maix_apu_dir_set_down_size(mp_obj_t size_obj) {
    uint8_t size = mp_obj_get_int(size_obj);
    lib_apu_dir_set_down_size(size);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_dir_set_down_size_obj, Maix_apu_dir_set_down_size);

// Set down-sizing ratio for voice stream generation
STATIC mp_obj_t Maix_apu_voc_set_down_size(mp_obj_t size_obj) {
    uint8_t size = mp_obj_get_int(size_obj);
    lib_apu_voc_set_down_size(size);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_voc_set_down_size_obj, Maix_apu_voc_set_down_size);

// Set direction searching interrupt mask
STATIC mp_obj_t Maix_apu_dir_set_interrupt_mask(mp_obj_t mask_obj) {
    uint8_t mask = mp_obj_get_int(mask_obj);
    lib_apu_dir_set_interrupt_mask(mask);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_dir_set_interrupt_mask_obj, Maix_apu_dir_set_interrupt_mask);

// Set voice stream generation interrupt mask
STATIC mp_obj_t Maix_apu_voc_set_interrupt_mask(mp_obj_t mask_obj) {
    uint8_t mask = mp_obj_get_int(mask_obj);
    lib_apu_voc_set_interrupt_mask(mask);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_voc_set_interrupt_mask_obj, Maix_apu_voc_set_interrupt_mask);

// Clear direction interrupt state
STATIC mp_obj_t Maix_apu_dir_clear_int_state(void) {
    lib_apu_dir_clear_int_state();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_dir_clear_int_state_obj, Maix_apu_dir_clear_int_state);

// Clear voice interrupt state
STATIC mp_obj_t Maix_apu_voc_clear_int_state(void) {
    lib_apu_voc_clear_int_state();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_voc_clear_int_state_obj, Maix_apu_voc_clear_int_state);

// Reset saturation counter
STATIC mp_obj_t Maix_apu_voc_reset_saturation_counter(void) {
    lib_apu_voc_reset_saturation_counter();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_voc_reset_saturation_counter_obj, Maix_apu_voc_reset_saturation_counter);

// Get saturation counter
STATIC mp_obj_t Maix_apu_voc_get_saturation_counter(void) {
    return mp_obj_new_int(lib_apu_voc_get_saturation_counter());
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_voc_get_saturation_counter_obj, Maix_apu_voc_get_saturation_counter);

// Set saturation limit
STATIC mp_obj_t Maix_apu_voc_set_saturation_limit(mp_obj_t upper_obj, mp_obj_t bottom_obj) {
    uint16_t upper = mp_obj_get_int(upper_obj);
    uint16_t bottom = mp_obj_get_int(bottom_obj);
    lib_apu_voc_set_saturation_limit(upper, bottom);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(Maix_apu_voc_set_saturation_limit_obj, Maix_apu_voc_set_saturation_limit);

// Get saturation limit
STATIC mp_obj_t Maix_apu_voc_get_saturation_limit(void) {
    uint32_t limit = lib_apu_voc_get_saturation_limit();
    uint16_t upper = (limit >> 16) & 0xFFFF;
    uint16_t bottom = limit & 0xFFFF;
    mp_obj_t tuple[2] = {
        mp_obj_new_int(upper),
        mp_obj_new_int(bottom)
    };
    return mp_obj_new_tuple(2, tuple);
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_voc_get_saturation_limit_obj, Maix_apu_voc_get_saturation_limit);

// Initialize FPIOA pins
STATIC mp_obj_t Maix_apu_init_fpioa(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_i2s_d0,   MP_ARG_INT, {.u_int = 23} },
        { MP_QSTR_i2s_d1,   MP_ARG_INT, {.u_int = 22} },
        { MP_QSTR_i2s_d2,   MP_ARG_INT, {.u_int = 21} },
        { MP_QSTR_i2s_d3,   MP_ARG_INT, {.u_int = 20} },
        { MP_QSTR_i2s_ws,   MP_ARG_INT, {.u_int = 19} },
        { MP_QSTR_i2s_sclk, MP_ARG_INT, {.u_int = 18} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    int i2s_d0_pin = args[0].u_int;
    int i2s_d1_pin = args[1].u_int;
    int i2s_d2_pin = args[2].u_int;
    int i2s_d3_pin = args[3].u_int;
    int i2s_ws_pin = args[4].u_int;
    int i2s_sclk_pin = args[5].u_int;
    
    lib_apu_init_fpioa(i2s_d0_pin, i2s_d1_pin, i2s_d2_pin, i2s_d3_pin,
                       i2s_ws_pin, i2s_sclk_pin);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_apu_init_fpioa_obj, 0, Maix_apu_init_fpioa);

// Initialize I2S device
STATIC mp_obj_t Maix_apu_init_i2s(mp_obj_t sample_rate_obj) {
    uint32_t sample_rate = mp_obj_get_int(sample_rate_obj);
    lib_apu_init_i2s(sample_rate);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Maix_apu_init_i2s_obj, Maix_apu_init_i2s);

// Print current APU settings
STATIC mp_obj_t Maix_apu_print_settings(void) {
    lib_apu_print_setting();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_print_settings_obj, Maix_apu_print_settings);

// Initialize APU demo
/*STATIC mp_obj_t Maix_apu_demo_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_enable_ppl, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_enable_irq, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_reinit_irq, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_reinit_all, MP_ARG_INT, {.u_int = 1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    uint8_t enable_ppl = args[0].u_int;
    uint8_t enable_irq = args[1].u_int;
    uint8_t reinit_irq = args[2].u_int;
    uint8_t reinit_all = args[3].u_int;
    
    apu_demo_init(enable_ppl, enable_irq, reinit_irq, reinit_all);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(Maix_apu_demo_init_obj, 0, Maix_apu_demo_init);

// Print current APU settings
STATIC mp_obj_t Maix_apu_demo_run(void) {
    apu_demo_run();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(Maix_apu_demo_run_obj, Maix_apu_demo_run);*/

// APU class methods
STATIC const mp_rom_map_elem_t Maix_apu_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_APU) },
    
    // Initialization functions
    { MP_ROM_QSTR(MP_QSTR_init_all), MP_ROM_PTR(&Maix_apu_init_all_obj) },
    { MP_ROM_QSTR(MP_QSTR_init_led), MP_ROM_PTR(&Maix_apu_init_led_obj) },
    { MP_ROM_QSTR(MP_QSTR_init_clock), MP_ROM_PTR(&Maix_apu_init_clock_obj) },
    { MP_ROM_QSTR(MP_QSTR_init_fpioa), MP_ROM_PTR(&Maix_apu_init_fpioa_obj) },
    { MP_ROM_QSTR(MP_QSTR_init_i2s), MP_ROM_PTR(&Maix_apu_init_i2s_obj) },
    { MP_ROM_QSTR(MP_QSTR_init_apu), MP_ROM_PTR(&Maix_apu_init_apu_obj) },
    { MP_ROM_QSTR(MP_QSTR_init_plic), MP_ROM_PTR(&Maix_apu_init_plic_obj) },
    
    // Direction detection methods
    { MP_ROM_QSTR(MP_QSTR_set_led), MP_ROM_PTR(&Maix_apu_set_led_obj) },
    { MP_ROM_QSTR(MP_QSTR_configure_direction), MP_ROM_PTR(&Maix_apu_configure_direction_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_direction_detection), MP_ROM_PTR(&Maix_apu_start_direction_detection_obj) },
    { MP_ROM_QSTR(MP_QSTR_dir_is_ready), MP_ROM_PTR(&Maix_apu_dir_is_ready_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_direction), MP_ROM_PTR(&Maix_apu_get_direction_obj) },
    { MP_ROM_QSTR(MP_QSTR_dir_clear_ready), MP_ROM_PTR(&Maix_apu_dir_clear_ready_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_settings), MP_ROM_PTR(&Maix_apu_print_settings_obj) },

    /*{ MP_ROM_QSTR(MP_QSTR_demo_init), MP_ROM_PTR(&Maix_apu_demo_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_demo_run), MP_ROM_PTR(&Maix_apu_demo_run_obj) },*/
    
    // Voice output methods
    { MP_ROM_QSTR(MP_QSTR_enable_voice_output), MP_ROM_PTR(&Maix_apu_enable_voice_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_voice_output), MP_ROM_PTR(&Maix_apu_disable_voice_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_source_mode), MP_ROM_PTR(&Maix_apu_set_source_mode_obj) },
    
    // General control methods
    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&Maix_apu_reset_obj) },
    
    // FIR filter methods
    { MP_ROM_QSTR(MP_QSTR_set_dir_pre_fir), MP_ROM_PTR(&Maix_apu_set_dir_pre_fir_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_dir_post_fir), MP_ROM_PTR(&Maix_apu_set_dir_post_fir_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_voice_pre_fir), MP_ROM_PTR(&Maix_apu_set_voice_pre_fir_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_voice_post_fir), MP_ROM_PTR(&Maix_apu_set_voice_post_fir_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_dir_pre_fir), MP_ROM_PTR(&Maix_apu_get_dir_pre_fir_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_dir_post_fir), MP_ROM_PTR(&Maix_apu_get_dir_post_fir_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_voice_pre_fir), MP_ROM_PTR(&Maix_apu_get_voice_pre_fir_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_voice_post_fir), MP_ROM_PTR(&Maix_apu_get_voice_post_fir_obj) },
    
    // Downsampling methods
    { MP_ROM_QSTR(MP_QSTR_dir_set_down_size), MP_ROM_PTR(&Maix_apu_dir_set_down_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_voc_set_down_size), MP_ROM_PTR(&Maix_apu_voc_set_down_size_obj) },
    
    // Interrupt methods
    { MP_ROM_QSTR(MP_QSTR_dir_set_interrupt_mask), MP_ROM_PTR(&Maix_apu_dir_set_interrupt_mask_obj) },
    { MP_ROM_QSTR(MP_QSTR_voc_set_interrupt_mask), MP_ROM_PTR(&Maix_apu_voc_set_interrupt_mask_obj) },
    { MP_ROM_QSTR(MP_QSTR_dir_clear_int_state), MP_ROM_PTR(&Maix_apu_dir_clear_int_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_voc_clear_int_state), MP_ROM_PTR(&Maix_apu_voc_clear_int_state_obj) },
    
    // Saturation methods
    { MP_ROM_QSTR(MP_QSTR_voc_reset_saturation_counter), MP_ROM_PTR(&Maix_apu_voc_reset_saturation_counter_obj) },
    { MP_ROM_QSTR(MP_QSTR_voc_get_saturation_counter), MP_ROM_PTR(&Maix_apu_voc_get_saturation_counter_obj) },
    { MP_ROM_QSTR(MP_QSTR_voc_set_saturation_limit), MP_ROM_PTR(&Maix_apu_voc_set_saturation_limit_obj) },
    { MP_ROM_QSTR(MP_QSTR_voc_get_saturation_limit), MP_ROM_PTR(&Maix_apu_voc_get_saturation_limit_obj) },
};

STATIC MP_DEFINE_CONST_DICT(Maix_apu_locals_dict, Maix_apu_locals_dict_table);

const mp_obj_type_t Maix_apu_type = {
    { &mp_type_type },
    .name = MP_QSTR_APU,
    .locals_dict = (mp_obj_dict_t*)&Maix_apu_locals_dict,
};