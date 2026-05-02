#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include "adc-task/adc-task.h"


#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

void version_callback(const char* args)
{
    printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

void led_on_callback(const char* args)
{
    printf("led_on:\n");
    led_task_state_set(LED_STATE_ON);
}

void led_off_callback(const char* args)
{
    printf("led_off:\n");
    led_task_state_set(LED_STATE_OFF);
}

void led_blink_callback(const char* args)
{
    printf("led_blink:\n");
    led_task_state_set(LED_STATE_BLINK);
}

void adc_callback(const char* args)
{
    float voltage_V=adc_task_getV();
    printf("%f\n", voltage_V);
}

void adc_t_callback(const char* args)
{
    float temp_C=adc_task_getT();
    printf("%f\n", temp_C);
}
void tm_start_callback(const char* args){
    adc_task_set_state(ADC_TASK_STATE_RUN);
    printf("Telemetry started.\n");
}
void tm_stop_callback(const char* args){
    adc_task_set_state(ADC_TASK_STATE_IDLE);
    printf("Telemetry stopped.\n");
}

api_t device_api[] =
{
 {"version", version_callback, "get device name and firmware version"},
 {"on", led_on_callback, "turns led on"},
 {"off", led_off_callback, "turns led off"},
 {"blink", led_blink_callback, "turns led in blink state"},
 {"get_adc", adc_callback, "get voltage by adc"},
 {"get_temp", adc_t_callback, "get temperature by adc"},
 {"tm_start", tm_start_callback, "start getting temp"},
 {"tm_stop", tm_stop_callback, "stop getting temp"},
 {NULL, NULL, NULL}
};

int main(){
    stdio_task_init();
    stdio_init_all();
    protocol_task_init(device_api);
    led_task_init();
    adc_task_init();
    while (1)
    {
        led_task_handle();
        char* command = stdio_task_handle();
        adc_task_handle();
        if (command != NULL)
        {
            protocol_task_handle(command);

        }
    }
}
