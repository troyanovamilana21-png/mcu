#include "led-task.h"
#include "stdint.h"
#include "pico/stdlib.h"
#include <stdio.h>

// Константы
#define LED_PIN 25  // Встроенный LED на Raspberry Pi Pico
#define DEFAULT_BLINK_PERIOD_MS 500

// Статические переменные для хранения состояния
static led_state_t current_state = LED_STATE_OFF;
static uint32_t blink_period_ms = DEFAULT_BLINK_PERIOD_MS;
static uint32_t LED_BLINK_PERIOD_US = DEFAULT_BLINK_PERIOD_MS * 1000;
static uint32_t last_toggle_time = 0;
static bool led_state = false;

void led_task_init(void* api)
{
    // Инициализация пина LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
    
    printf("LED task initialized\n");
}

void led_task_state_set(led_state_t state)
{
    current_state = state;
    
    // Сбрасываем таймер при смене состояния
    last_toggle_time = time_us_32() / 1000; // Переводим в миллисекунды
    
    // Если перешли в режим OFF или ON, сразу устанавливаем соответствующее состояние LED
    if (state == LED_STATE_OFF) {
        gpio_put(LED_PIN, 0);
        led_state = false;
    } else if (state == LED_STATE_ON) {
        gpio_put(LED_PIN, 1);
        led_state = true;
    }
}

led_state_t led_task_state_get(void)
{
    return current_state;
}

void led_task_set_blink_period(uint32_t period_ms)
{
    if (period_ms > 0) {
        blink_period_ms = period_ms;
        printf("Blink period updated to %u ms\n", blink_period_ms);
    } else {
        printf("Invalid period value: %u ms. Keeping current period: %u ms\n",
               period_ms, blink_period_ms);
    }
}

uint32_t led_task_get_blink_period(void)
{
    return blink_period_ms;
}

void led_task_set_blink_period_ms(uint32_t period_ms)
{
    LED_BLINK_PERIOD_US = period_ms * 1000;
    blink_period_ms = period_ms;
}

void led_task_handle(void* param)
{
    static uint32_t last_print_time = 0;
    uint32_t current_time = time_us_32() / 1000; // Текущее время в миллисекундах
    
    switch (current_state) {
        case LED_STATE_BLINK:
            // Проверяем, прошло ли достаточно времени для переключения
            if (current_time - last_toggle_time >= blink_period_ms) {
                // Переключаем состояние LED
                led_state = !led_state;
                gpio_put(LED_PIN, led_state);
                last_toggle_time = current_time;
                
                // Для отладки - выводим информацию каждую секунду
                if (current_time - last_print_time >= 1000) {
                    printf("Blinking with period: %u ms, LED: %s\n",
                           blink_period_ms, led_state ? "ON" : "OFF");
                    last_print_time = current_time;
                }
            }
            break;
            
        case LED_STATE_ON:
            // Уже установлено в led_task_state_set
            break;
            
        case LED_STATE_OFF:
            // Уже установлено в led_task_state_set
            break;
            
        default:
            break;
    }
}
