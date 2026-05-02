#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"

#define DEVICE_NAME "RP2040 Device"
#define DEVICE_VRSN "1.0.0"

// Callback для получения версии устройства
void version_callback(const char* args)
{
    printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

// Callback для установки периода мигания LED
void led_set_period_callback(const char* args)
{
    if (args == NULL || args[0] == '\0') {
        printf("Error: Period value not specified\n");
        printf("Usage: set_period <period_ms>\n");
        return;
    }
    
    uint32_t period_ms = 0;
    int result = sscanf(args, "%u", &period_ms);
    
    if (result != 1 || period_ms == 0) {
        printf("Error: Invalid period value. Please specify positive number (ms)\n");
        return;
    }
    
    led_task_set_blink_period(period_ms);
    printf("LED blink period set to %u ms\n", period_ms);
    
    // Если LED сейчас мигает, покажем текущий период
    if (led_task_state_get() == LED_STATE_BLINK) {
        printf("LED is currently blinking with new period %u ms\n", period_ms);
    }
}

// Callback для включения LED
void led_on_callback(const char* args)
{
    led_task_state_set(LED_STATE_ON);
    printf("LED turned ON\n");
}

// Callback для выключения LED
void led_off_callback(const char* args)
{
    led_task_state_set(LED_STATE_OFF);
    printf("LED turned OFF\n");
}

// Callback для мигания LED
void led_blink_callback(const char* args)
{
    // Проверяем, передан ли параметр с периодом мигания
    if (args != NULL && args[0] != '\0') {
        uint32_t period_ms = 0;
        int result = sscanf(args, "%u", &period_ms);
        
        if (result == 1 && period_ms > 0) {
            led_task_set_blink_period(period_ms);
            printf("LED blink period set to %u ms\n", period_ms);
        } else {
            printf("Warning: Invalid period value. Using current period: %u ms\n",
                   led_task_get_blink_period());
        }
    } else {
        printf("Using current blink period: %u ms\n", led_task_get_blink_period());
    }
    
    // Включаем режим мигания
    led_task_state_set(LED_STATE_BLINK);
    printf("LED blinking started with period %u ms\n", led_task_get_blink_period());
}

// Callback для получения текущего состояния LED
void led_status_callback(const char* args)
{
    led_state_t current_state = led_task_state_get();
    uint32_t current_period = led_task_get_blink_period();
    
    printf("\n=== LED Status ===\n");
    printf("State: ");
    switch(current_state) {
        case LED_STATE_OFF:
            printf("OFF\n");
            break;
        case LED_STATE_ON:
            printf("ON\n");
            break;
        case LED_STATE_BLINK:
            printf("BLINKING\n");
            printf("Blink period: %u ms\n", current_period);
            break;
        default:
            printf("UNKNOWN\n");
    }
    printf("==================\n");
}

// Прототипы — нужны, так как device_api объявлен раньше реализаций
void help_callback(const char* args);
void mem_callback(const char* args);
void wmem_callback(const char* args);

api_t device_api[] =
{
    {"help",       help_callback,            "show this help"},
    {"version",    version_callback,         "get device name and firmware version"},
    {"on",         led_on_callback,          "turn LED on"},
    {"off",        led_off_callback,         "turn LED off"},
    {"blink",      led_blink_callback,       "make LED blink [period_ms]"},
    {"set_period", led_set_period_callback,  "set LED blink period in ms"},
    {"status",     led_status_callback,      "get current LED status"},
    {"mem",        mem_callback,             "read word from address: mem <addr_hex>"},
    {"wmem",       wmem_callback,            "write word to address: wmem <addr_hex> <val_hex>"},
    {NULL, NULL, NULL},
};

// Callback для помощи — выводит список команд из device_api
void help_callback(const char* args)
{
    printf("\nAvailable commands:\n");
    for (int i = 0; device_api[i].command_name != NULL; i++)
    {
        printf("  %-12s - %s\n", device_api[i].command_name, device_api[i].command_help);
    }
    printf("\n");
}

// Callback команды mem — чтение слова из адресного пространства МК
// Использование: mem <addr_hex>
// Пример:        mem 20000000
void mem_callback(const char* args)
{
    if (args == NULL || args[0] == '\0') {
        printf("Error: address not specified\n");
        printf("Usage: mem <addr_hex>\n");
        return;
    }

    uint32_t addr = 0;
    int result = sscanf(args, "%x", &addr);

    if (result != 1) {
        printf("Error: invalid address '%s'\n", args);
        return;
    }

    volatile uint32_t* ptr = (volatile uint32_t*)addr;
    uint32_t value = *ptr;

    printf("mem[0x%08X] = 0x%08X (%u)\n", addr, value, value);
}

// Callback команды wmem — запись слова в адресное пространство МК
// Использование: wmem <addr_hex> <value_hex>
// Пример:        wmem D0000014 02000000
void wmem_callback(const char* args)
{
    if (args == NULL || args[0] == '\0') {
        printf("Error: arguments not specified\n");
        printf("Usage: wmem <addr_hex> <value_hex>\n");
        return;
    }

    uint32_t addr  = 0;
    uint32_t value = 0;
    int result = sscanf(args, "%x %x", &addr, &value);

    if (result != 2) {
        printf("Error: expected two hex arguments, got %d\n", result);
        printf("Usage: wmem <addr_hex> <value_hex>\n");
        return;
    }

    volatile uint32_t* ptr = (volatile uint32_t*)addr;
    *ptr = value;

    printf("wmem[0x%08X] <- 0x%08X: OK\n", addr, value);
}

int main()
{
    stdio_init_all();
    stdio_task_init();
    protocol_task_init(device_api);
    led_task_init(device_api);
    
    printf("\n========================================\n");
    printf("Device started: %s v%s\n", DEVICE_NAME, DEVICE_VRSN);
    printf("========================================\n");
    
    // Показываем помощь при старте
    help_callback(NULL);
    printf("\nEnter command: ");
    
    while (1)
    {
        char* command_string = stdio_task_handle();
        
        if (command_string != NULL)
        {
            protocol_task_handle(command_string);
            printf("\nEnter command: ");
            fflush(stdout);
        }
        
        led_task_handle(NULL);
        
        sleep_ms(10);
    }
    
    return 0;
}
