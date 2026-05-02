#include "hardware/adc.h"
#include "adc-task.h"
#include "pico/stdlib.h"
#include "stdio.h"
#include "stdlib.h"
#include "hardware/gpio.h"

static adc_task_state_t adc_state = ADC_TASK_STATE_IDLE;
uint64_t last_measure_us = 0;
uint ADC_TASK_MEAS_PERIOD_US = 100000;

const int adc_pin = 26;
const int adc_ch = 0;
const int adc_temp_ch = 4;
float voltage;
float temp_C;


void adc_task_init(){
    adc_init();
    adc_gpio_init(adc_pin);
    adc_set_temp_sensor_enabled(true);
}

float adc_task_getV(){
    adc_select_input(adc_ch);
    uint16_t voltage_counts = adc_read();
    voltage=3.3/4096*voltage_counts;
    return voltage;
}

float adc_task_getT(){
    adc_select_input(adc_temp_ch);
    uint16_t voltage_counts = adc_read();
    voltage=3.3/4096*voltage_counts;
    temp_C = 27.0f - (voltage - 0.706f) / 0.001721f;
    return temp_C;
}

void adc_task_set_state(adc_task_state_t state) {
    adc_state = state;
    if (state == ADC_TASK_STATE_RUN) {
        last_measure_us = time_us_64();
    }
}

void adc_task_handle(void) {
    if (adc_state != ADC_TASK_STATE_RUN) {
        return;
    }

    uint64_t now_us = time_us_64();
    if (now_us - last_measure_us < ADC_TASK_MEAS_PERIOD_US) {
        return;
    }
    last_measure_us = now_us;

    uint64_t uptime_ms = now_us / 1000;

    float voltage = adc_task_getV();
    float temperature = adc_task_getT();

    printf("%f %f\n", voltage, temperature);
}
