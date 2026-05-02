// led-task.h
#ifndef LED_TASK_H
#define LED_TASK_H

#include <stdint.h>

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON = 1,
    LED_STATE_BLINK = 2,
} led_state_t;

// Инициализация LED задачи
void led_task_init(void* api);

// Управление состоянием
void led_task_state_set(led_state_t state);
led_state_t led_task_state_get(void);

// Управление периодом мигания
void led_task_set_blink_period(uint32_t period_ms);
uint32_t led_task_get_blink_period(void);

// Новая функция: установка периода в миллисекундах (конвертирует в мкс внутри)
void led_task_set_blink_period_ms(uint32_t period_ms);

// Обработчик задачи (должен вызываться в цикле)
void led_task_handle(void* param);

#endif // LED_TASK_H

