#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include "ili9341-driver.h"
#include "ili9341-display.h"
#include "ili9341-font.h"
#include "bme280-driver.h"

#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

#define ILI9341_PIN_MISO 4
#define ILI9341_PIN_CS 10
#define ILI9341_PIN_SCK 6
#define ILI9341_PIN_MOSI 7
#define ILI9341_PIN_DC 8
#define ILI9341_PIN_RESET 9

// Определения для графиков (адаптировано под экран 240x320)
#define GRAPH_POINTS 60  // 60 точек (по 1 секунде при периоде 1000мс)
#define GRAPH_WIDTH 300
#define GRAPH_HEIGHT 60   // Уменьшено для лучшего размещения
#define GRAPH_START_X 10
#define GRAPH_START_Y 150  // Позиция графика

static ili9341_display_t ili9341_display = {0};

// Глобальные переменные для измерения
typedef struct {
    float temperature;
    float pressure;
    float humidity;
    uint32_t last_measurement_ms;
    uint32_t measurement_period_ms;
    bool auto_measurement_enabled;
} measurement_data_t;

// Структура для хранения истории измерений
typedef struct {
    float temperature[GRAPH_POINTS];
    float pressure[GRAPH_POINTS];
    float humidity[GRAPH_POINTS];
    uint8_t current_index;
    bool buffer_full;
} measurement_history_t;

static measurement_data_t measurement_data = {
    .temperature = 0,
    .pressure = 0,
    .humidity = 0,
    .last_measurement_ms = 0,
    .measurement_period_ms = 1000,  // 1 секунда для плавных графиков
    .auto_measurement_enabled = true
};

static measurement_history_t history = {
    .current_index = 0,
    .buffer_full = false
};

// Прототипы функций
void mem_prot(uint32_t addr);
void wmem_prot(uint32_t addr, uint32_t value);
void rp2040_i2c_read(uint8_t* buffer, uint16_t length);
void rp2040_i2c_write(uint8_t* data, uint16_t size);
void rp2040_spi_write(const uint8_t *data, uint32_t size);
void rp2040_spi_read(uint8_t *buffer, uint32_t length);
void rp2040_gpio_cs_write(bool level);
void rp2040_gpio_dc_write(bool level);
void rp2040_gpio_reset_write(bool level);
void rp2040_sleep(uint32_t ms);
void perform_measurement(void);
void add_to_history(void);
float get_history_value(float* buffer, int index);
int normalize_to_y(float value, float min_val, float max_val, int graph_y, int graph_height);
void draw_graph_grid(int x, int y, int width, int height);
void draw_temperature_graph(int x, int y, int width, int height);
void update_gui_display(void);
void set_period_callback(const char* args);
void auto_measure_callback(const char* args);
void measure_once_callback(const char* args);
void stats_callback(const char* args);
void help_callback(const char* args);
void version_callback(const char* args);
void led_on_callback(const char* args);
void led_off_callback(const char* args);
void led_blink_callback(const char* args);
void read_reg_callback(const char* args);
void write_reg_callback(const char* args);
void temp_raw_callback(const char* args);
void pres_raw_callback(const char* args);
void hum_raw_callback(const char* args);
void temp_callback(const char* args);
void pres_callback(const char* args);
void hum_callback(const char* args);
void led_blink_set_period_ms_callback(const char* args);
void mem_callback(const char* args);
void wmem_callback(const char* args);
void disp_screen_callback(const char* args);
void disp_px_callback(const char* args);
void disp_line_callback(const char* args);
void disp_rect_callback(const char* args);
void disp_frect_callback(const char* args);
void disp_text_callback(const char* args);
void clear_history_callback(const char* args);

// Реализация функций
void rp2040_i2c_read(uint8_t* buffer, uint16_t length) {
    i2c_read_timeout_us(i2c1, 0x76, buffer, length, false, 100000);
}

void rp2040_i2c_write(uint8_t* data, uint16_t size) {
    i2c_write_timeout_us(i2c1, 0x76, data, size, false, 100000);
}

void rp2040_spi_write(const uint8_t *data, uint32_t size) {
    spi_write_blocking(spi0, data, size);
}

void rp2040_spi_read(uint8_t *buffer, uint32_t length) {
    spi_read_blocking(spi0, 0, buffer, length);
}

void rp2040_gpio_cs_write(bool level) {
    gpio_put(ILI9341_PIN_CS, level);
}

void rp2040_gpio_dc_write(bool level) {
    gpio_put(ILI9341_PIN_DC, level);
}

void rp2040_gpio_reset_write(bool level) {
    gpio_put(ILI9341_PIN_RESET, level);
}

void rp2040_sleep(uint32_t ms) {
    sleep_ms(ms);
}

// Функция для добавления нового измерения в историю
void add_to_history(void) {
    history.temperature[history.current_index] = measurement_data.temperature;
    history.pressure[history.current_index] = measurement_data.pressure;
    history.humidity[history.current_index] = measurement_data.humidity;
    
    history.current_index++;
    if (history.current_index >= GRAPH_POINTS) {
        history.current_index = 0;
        history.buffer_full = true;
    }
}

// Функция для получения значения из истории по индексу (с учетом циклического буфера)
float get_history_value(float* buffer, int index) {
    if (!history.buffer_full) {
        if (index >= history.current_index) return 0;
        return buffer[index];
    } else {
        int actual_index = (history.current_index + index) % GRAPH_POINTS;
        return buffer[actual_index];
    }
}

// Функция для нормализации значения к координате Y графика
int normalize_to_y(float value, float min_val, float max_val, int graph_y, int graph_height) {
    if (max_val <= min_val) return graph_y + graph_height / 2;
    float normalized = (value - min_val) / (max_val - min_val);
    // Инвертируем Y, так как экран начинается сверху
    int result = graph_y + graph_height - (int)(normalized * graph_height);
    // Ограничиваем значения в пределах графика
    if (result < graph_y) result = graph_y;
    if (result > graph_y + graph_height) result = graph_y + graph_height;
    return result;
}

// Функция для рисования сетки графика
void draw_graph_grid(int x, int y, int width, int height) {
    // Рисуем рамку
    ili9341_draw_rect(&ili9341_display, x, y, width, height, COLOR_WHITE);
    
    // Рисуем горизонтальные линии (4 линии)
    for (int i = 0; i <= 4; i++) {
        int line_y = y + (height * i) / 4;
        ili9341_draw_line(&ili9341_display, x + 5, line_y, x + width - 5, line_y, COLOR_GREEN);
    }
    
    // Рисуем вертикальные линии (6 линий)
    for (int i = 0; i <= 6; i++) {
        int line_x = x + (width * i) / 6;
        ili9341_draw_line(&ili9341_display, line_x, y + 5, line_x, y + height - 5, COLOR_GREEN);
    }
}

// Функция для рисования графика температуры
void draw_temperature_graph(int x, int y, int width, int height) {
    float min_temp = 100, max_temp = -100;
    int num_points = history.buffer_full ? GRAPH_POINTS : history.current_index;
    
    // Находим min/max температуры для масштабирования
    for (int i = 0; i < num_points; i++) {
        float temp = get_history_value(history.temperature, i);
        if (temp != 0) {
            if (temp < min_temp) min_temp = temp;
            if (temp > max_temp) max_temp = temp;
        }
    }
    
    // Если нет данных или min==max, устанавливаем диапазон по умолчанию
    if (num_points == 0 || min_temp > max_temp) {
        min_temp = 15;
        max_temp = 35;
    }
    if (max_temp - min_temp < 5) {
        min_temp -= 2.5;
        max_temp += 2.5;
    }
    
    // Рисуем подпись параметра
    char label[32];
    snprintf(label, sizeof(label), "Temperature (C)");
    ili9341_draw_text(&ili9341_display, x, y - 12, label, &jetbrains_font, COLOR_RED, COLOR_BLACK);
    
    // Рисуем сетку
    draw_graph_grid(x, y, width, height);
    
    // Рисуем график
    int prev_x = 0, prev_y = 0;
    bool first_point = true;
    
    for (int i = 0; i < num_points; i++) {
        float temp = get_history_value(history.temperature, i);
        if (temp != 0) {
            int current_x = x + (width * i) / (GRAPH_POINTS - 1);
            int current_y = normalize_to_y(temp, min_temp, max_temp, y, height);
            
            if (!first_point) {
                ili9341_draw_line(&ili9341_display, prev_x, prev_y, current_x, current_y, COLOR_RED);
            }
            
            // Рисуем точки
            ili9341_draw_filled_rect(&ili9341_display, current_x - 1, current_y - 1, 2, 2, COLOR_RED);
            
            prev_x = current_x;
            prev_y = current_y;
            first_point = false;
        }
    }
    
    // Отображаем min/max значения
    snprintf(label, sizeof(label), "Min:%.0fC Max:%.0fC", min_temp, max_temp);
    ili9341_draw_text(&ili9341_display, x + width - 100, y + height + 2, label, &jetbrains_font, COLOR_GREEN, COLOR_BLACK);
}

void perform_measurement(void) {
    measurement_data.temperature = bme280_read_temp_celsius();
    measurement_data.pressure = bme280_read_press_pascal() * 100;
    measurement_data.humidity = bme280_read_hum_percent();
    
    // Добавляем измерение в историю
    add_to_history();
}

void update_gui_display(void) {
    char text_buffer[64];
    int bar_height;
    uint16_t bar_color;
    
    // Очищаем верхнюю часть экрана
    ili9341_draw_filled_rect(&ili9341_display, 0, 0, 320, 80, COLOR_BLACK);
    
    // Температура
    snprintf(text_buffer, sizeof(text_buffer), "Temp: %.2f C", measurement_data.temperature);
    ili9341_draw_text(&ili9341_display, 10, 75, text_buffer, &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
    
    bar_height = (int)((measurement_data.temperature / 50.0f) * 60);
    if (bar_height > 60) bar_height = 60;
    if (bar_height < 0) bar_height = 0;
    
    if (measurement_data.temperature < 10) bar_color = COLOR_BLUE;
    else if (measurement_data.temperature < 25) bar_color = COLOR_GREEN;
    else if (measurement_data.temperature < 35) bar_color = COLOR_YELLOW;
    else bar_color = COLOR_RED;
    
    ili9341_draw_filled_rect(&ili9341_display, 10, 70 - bar_height, 30, bar_height, bar_color);
    ili9341_draw_rect(&ili9341_display, 10, 10, 30, 60, COLOR_WHITE);
    
    // Давление
    snprintf(text_buffer, sizeof(text_buffer), "Press: %.0f Pa", measurement_data.pressure);
    ili9341_draw_text(&ili9341_display, 10, 100, text_buffer, &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
    
    float pressure_norm = (measurement_data.pressure - 98000) / 5000.0f;
    if (pressure_norm > 1.0f) pressure_norm = 1.0f;
    if (pressure_norm < 0) pressure_norm = 0;
    bar_height = (int)(pressure_norm * 60);
    
    ili9341_draw_filled_rect(&ili9341_display, 50, 70 - bar_height, 30, bar_height, COLOR_CYAN);
    ili9341_draw_rect(&ili9341_display, 50, 10, 30, 60, COLOR_WHITE);
    
    // Влажность
    snprintf(text_buffer, sizeof(text_buffer), "Hum: %.1f %%", measurement_data.humidity);
    ili9341_draw_text(&ili9341_display, 10, 125, text_buffer, &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
    
    bar_height = (int)((measurement_data.humidity / 100.0f) * 60);
    if (bar_height > 60) bar_height = 60;
    
    if (measurement_data.humidity < 30) bar_color = COLOR_YELLOW;
    else if (measurement_data.humidity < 70) bar_color = COLOR_GREEN;
    else bar_color = COLOR_BLUE;
    
    ili9341_draw_filled_rect(&ili9341_display, 90, 70 - bar_height, 30, bar_height, bar_color);
    ili9341_draw_rect(&ili9341_display, 90, 10, 30, 60, COLOR_WHITE);
    
    // Статус автоизмерения
    if (measurement_data.auto_measurement_enabled) {
        snprintf(text_buffer, sizeof(text_buffer), "Auto: %d ms", measurement_data.measurement_period_ms);
        ili9341_draw_text(&ili9341_display, 200, 5, text_buffer, &jetbrains_font, COLOR_GREEN, COLOR_BLACK);
        
    } 
    else {
        ili9341_draw_text(&ili9341_display, 200, 5, "Auto: OFF", &jetbrains_font, COLOR_RED, COLOR_BLACK);
    }
    
    snprintf(text_buffer, sizeof(text_buffer), "Period: %d ms", measurement_data.measurement_period_ms);
    ili9341_draw_text(&ili9341_display, 200, 30, text_buffer, &jetbrains_font, COLOR_WHITE, COLOR_BLACK);

    // Очищаем область графиков
    ili9341_draw_filled_rect(&ili9341_display, 0, 140, 320, 80, COLOR_BLACK);
    
    // Рисуем график температуры
    draw_temperature_graph(GRAPH_START_X, GRAPH_START_Y, GRAPH_WIDTH, GRAPH_HEIGHT);
}

// Коллбэк для очистки истории
void clear_history_callback(const char* args) {
    history.current_index = 0;
    history.buffer_full = false;
    memset(history.temperature, 0, sizeof(history.temperature));
    memset(history.pressure, 0, sizeof(history.pressure));
    memset(history.humidity, 0, sizeof(history.humidity));
    printf("Measurement history cleared\n");
}

// Коллбэки для измерений
void set_period_callback(const char* args) {
    uint32_t period_ms;
    if (sscanf(args, "%u", &period_ms) == 1 && period_ms >= 100) {
        measurement_data.measurement_period_ms = period_ms;
        printf("Measurement period set to %d ms\n", period_ms);
    } else {
        printf("Error: period must be >= 100 ms\n");
        printf("Usage: set_period_ms <ms>\n");
    }
}

void auto_measure_callback(const char* args) {
    if (args == NULL || args[0] == '\0') {
        measurement_data.auto_measurement_enabled = !measurement_data.auto_measurement_enabled;
        printf("Auto measurement %s\n", measurement_data.auto_measurement_enabled ? "ON" : "OFF");
    } else if (strcmp(args, "on") == 0) {
        measurement_data.auto_measurement_enabled = true;
        printf("Auto measurement ON\n");
    } else if (strcmp(args, "off") == 0) {
        measurement_data.auto_measurement_enabled = false;
        printf("Auto measurement OFF\n");
    } else {
        printf("Usage: auto_measure [on|off] - toggle or set auto measurement\n");
    }
}

void measure_once_callback(const char* args) {
    perform_measurement();
    update_gui_display();
    
    printf("Measurement: Temp=%.2f C, Press=%.0f Pa, Hum=%.1f %%\n", 
           measurement_data.temperature, 
           measurement_data.pressure, 
           measurement_data.humidity);
}

void stats_callback(const char* args) {
    static float min_temp = 100, max_temp = -100;
    static float min_press = 100000, max_press = 0;
    static float min_hum = 100, max_hum = 0;
    
    if (measurement_data.temperature < min_temp) min_temp = measurement_data.temperature;
    if (measurement_data.temperature > max_temp) max_temp = measurement_data.temperature;
    if (measurement_data.pressure < min_press) min_press = measurement_data.pressure;
    if (measurement_data.pressure > max_press) max_press = measurement_data.pressure;
    if (measurement_data.humidity < min_hum) min_hum = measurement_data.humidity;
    if (measurement_data.humidity > max_hum) max_hum = measurement_data.humidity;
    
    printf("\n=== Measurement Statistics ===\n");
    printf("Temperature: Min=%.2f C, Max=%.2f C\n", min_temp, max_temp);
    printf("Pressure:    Min=%.0f Pa, Max=%.0f Pa\n", min_press, max_press);
    printf("Humidity:    Min=%.1f %%, Max=%.1f %%\n", min_hum, max_hum);
    printf("Current period: %d ms\n", measurement_data.measurement_period_ms);
    printf("Auto mode: %s\n", measurement_data.auto_measurement_enabled ? "ON" : "OFF");
    printf("History points: %d\n", history.buffer_full ? GRAPH_POINTS : history.current_index);
    printf("================================\n");
}

// Остальные коллбэки
void help_callback(const char* args);

void version_callback(const char* args) {
    printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

void led_on_callback(const char* args) {
    printf("led_on:\n");
    led_task_state_set(LED_STATE_ON);
}

void led_off_callback(const char* args) {
    printf("led_off:\n");
    led_task_state_set(LED_STATE_OFF);
}

void led_blink_callback(const char* args) {
    printf("led_blink:\n");
    led_task_state_set(LED_STATE_BLINK);
}

void led_blink_set_period_ms_callback(const char* args) {
    uint period_ms = 0;
    sscanf(args, "%u", &period_ms);
    if (period_ms == 0) {
        printf("incorrect delay for blink\n");
        return;
    } else {
        led_task_set_blink_period_ms(period_ms);
    }
}

void read_reg_callback(const char* args) {
    unsigned int addr, count;
    if (sscanf(args, "%x %x", &addr, &count) != 2) {
        printf("Usage: read_reg <addr_hex> <count_hex>\n");
        return;
    }
    if (addr > 0xFF || count == 0 || count > 0xFF || addr + count > 0x100) {
        printf("Error: invalid address range\n");
        return;
    }
    uint8_t buffer[256] = {0};
    bme280_read_regs((uint8_t)addr, buffer, (uint8_t)count);
    for (int i = 0; i < (int)count; i++) {
        printf("bme280 register [0x%X] = 0x%X\n", addr + i, buffer[i]);
    }
}

void write_reg_callback(const char* args) {
    unsigned int addr, value;
    if (sscanf(args, "%x %x", &addr, &value) != 2) {
        printf("Usage: write_reg <addr_hex> <value_hex>\n");
        return;
    }
    if (addr > 0xFF || value > 0xFF) {
        printf("Error: invalid address or value\n");
        return;
    }
    bme280_write_reg((uint8_t)addr, (uint8_t)value);
}

void temp_raw_callback(const char* args) {
    uint16_t counts = bme280_read_temp_raw();
    printf("temp counts %d \n", counts);
}

void pres_raw_callback(const char* args) {
    uint16_t counts = bme280_read_press_raw();
    printf("press counts %d \n", counts);
}

void hum_raw_callback(const char* args) {
    uint16_t counts = bme280_read_hum_raw();
    printf("hum counts %d \n", counts);
}

void temp_callback(const char* args) {
    float C = bme280_read_temp_celsius();
    printf("%f\n", C);
}

void pres_callback(const char* args) {
    float P = bme280_read_press_pascal() * 100;
    printf("%f\n", P);
}

void hum_callback(const char* args) {
    float H = bme280_read_hum_percent();
    printf("%f\n", H);
}

void mem_callback(const char* args) {
    uint32_t addr;
    if (sscanf(args, "%x", &addr) != 1) {
        printf("Error: invalid address format.\n");
        return;
    }
    mem_prot(addr);
}

void wmem_callback(const char* args) {
    uint32_t addr, value;
    if (sscanf(args, "%x %x", &addr, &value) != 2) {
        printf("Error: invalid arguments.\n");
        return;
    }
    wmem_prot(addr, value);
}

void disp_screen_callback(const char* args) {
    uint32_t c = 0;
    int result = sscanf(args, "%x", &c);
    uint16_t color = COLOR_BLACK;
    if (result == 1) {
        color = RGB888_2_RGB565(c);
    }
    ili9341_fill_screen(&ili9341_display, color);
}

void disp_px_callback(const char* args) {
    uint32_t x = 0, y = 0, c = 0;
    int result = sscanf(args, "%u %u %x", &x, &y, &c);
    if (result < 3) {
        printf("Error: disp_px requires 3 arguments: x y color (hex)\n");
        return;
    }
    uint16_t color = RGB888_2_RGB565(c);
    ili9341_draw_pixel(&ili9341_display, (uint16_t)x, (uint16_t)y, color);
}

void disp_line_callback(const char* args) {
    uint32_t c;
    int x1, y1, x2, y2;
    int result = sscanf(args, "%d %d %d %d %x", &x1, &y1, &x2, &y2, &c);
    if (result != 5) {
        printf("Error: disp_line requires 5 arguments\n");
        return;
    }
    uint16_t color = RGB888_2_RGB565(c);
    ili9341_draw_line(&ili9341_display, (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, color);
}

void disp_rect_callback(const char* args) {
    uint32_t c;
    int x1, y1, x2, y2;
    int result = sscanf(args, "%d %d %d %d %x", &x1, &y1, &x2, &y2, &c);
    if (result != 5) {
        printf("Error: disp_rect requires 5 arguments\n");
        return;
    }
    uint16_t color = RGB888_2_RGB565(c);
    ili9341_draw_rect(&ili9341_display, (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, color);
}

void disp_frect_callback(const char* args) {
    uint32_t c;
    int x1, y1, x2, y2;
    int result = sscanf(args, "%d %d %d %d %x", &x1, &y1, &x2, &y2, &c);
    if (result != 5) {
        printf("Error: disp_frect requires 5 arguments\n");
        return;
    }
    uint16_t color = RGB888_2_RGB565(c);
    ili9341_draw_filled_rect(&ili9341_display, (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, color);
}

void disp_text_callback(const char* args) {
    int x, y;
    char text[64];
    uint32_t color, bg_color;
    int result = sscanf(args, "%d %d \"%63[^\"]\" %x %x", &x, &y, text, &color, &bg_color);
    if (result == 5) {
        uint16_t color_565 = RGB888_2_RGB565(color);
        uint16_t bg_565 = RGB888_2_RGB565(bg_color);
        ili9341_draw_text(&ili9341_display, x, y, text, &jetbrains_font, color_565, bg_565);
    } else {
        printf("Error: format: <x> <y> \"<text>\" <color> <bg_color>\n");
    }
}

// Массив API
api_t device_api[] = {
    {"version", version_callback, "get device name and firmware version"},
    {"on", led_on_callback, "turns led on"},
    {"off", led_off_callback, "turns led off"},
    {"blink", led_blink_callback, "turns led in blink state"},
    {"read_reg", read_reg_callback, "read registers on BME280"},
    {"write_reg", write_reg_callback, "write registers on BME280"},
    {"temp_raw", temp_raw_callback, "get temp in counts"},
    {"pres_raw", pres_raw_callback, "get pres in counts"},
    {"hum_raw", hum_raw_callback, "get hum in counts"},
    {"temp", temp_callback, "get temp"},
    {"pres", pres_callback, "get pres"},
    {"hum", hum_callback, "get hum"},
    {"set_period", led_blink_set_period_ms_callback, "set delay time for blink"},
    {"help", help_callback, "show all commands"},
    {"mem", mem_callback, "read from address"},
    {"wmem", wmem_callback, "write to address"},
    {"disp_screen", disp_screen_callback, "filled screen color"},
    {"disp_px", disp_px_callback, "set pixel color"},
    {"disp_line", disp_line_callback, "draw line"},
    {"disp_rect", disp_rect_callback, "draw rectangle"},
    {"disp_frect", disp_frect_callback, "draw filled rectangle"},
    {"disp_text", disp_text_callback, "draw text"},
    {"measure", measure_once_callback, "perform single measurement"},
    {"auto_measure", auto_measure_callback, "toggle auto measurement"},
    {"set_period_ms", set_period_callback, "set measurement period"},
    {"stats", stats_callback, "show measurement statistics"},
    {"clear_history", clear_history_callback, "clear measurement history"},
    {NULL, NULL, NULL}
};

void help_callback(const char* args) {
    printf("Available commands:\n");
    for (int i = 0; device_api[i].command_name != NULL; i++) {
        printf("  %s: %s\n", device_api[i].command_name, device_api[i].command_help);
    }
}

int main() {
    i2c_init(i2c1, 100000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    
    stdio_task_init();
    stdio_init_all();
    protocol_task_init(device_api);
    led_task_init();
    
    spi_init(spi0, 62500000);
    gpio_set_function(ILI9341_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_SCK, GPIO_FUNC_SPI);
    
    gpio_init(ILI9341_PIN_DC);
    gpio_init(ILI9341_PIN_CS);
    gpio_init(ILI9341_PIN_RESET);
    gpio_set_dir(ILI9341_PIN_DC, GPIO_OUT);
    gpio_set_dir(ILI9341_PIN_CS, GPIO_OUT);
    gpio_set_dir(ILI9341_PIN_RESET, GPIO_OUT);
    gpio_put(ILI9341_PIN_CS, 1);
    gpio_put(ILI9341_PIN_DC, 0);
    gpio_put(ILI9341_PIN_RESET, 0);
    
    ili9341_hal_t ili9341_hal = {
        .spi_write = rp2040_spi_write,
        .spi_read = rp2040_spi_read,
        .gpio_cs_write = rp2040_gpio_cs_write,
        .gpio_dc_write = rp2040_gpio_dc_write,
        .gpio_reset_write = rp2040_gpio_reset_write,
        .delay_ms = rp2040_sleep
    };
    
    ili9341_init(&ili9341_display, &ili9341_hal);
    ili9341_set_rotation(&ili9341_display, ILI9341_ROTATION_90);
    ili9341_fill_screen(&ili9341_display, COLOR_BLACK);
    
    sleep_ms(300);
    
    bme280_init(rp2040_i2c_read, rp2040_i2c_write);
    
    perform_measurement();
    update_gui_display();
    
    measurement_data.last_measurement_ms = to_ms_since_boot(get_absolute_time());
    
    while (1) {
        led_task_handle();
        char* command = stdio_task_handle();
        
        if (command != NULL) {
            protocol_task_handle(command);
        }
        
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (measurement_data.auto_measurement_enabled && 
            (current_time - measurement_data.last_measurement_ms) >= measurement_data.measurement_period_ms) {
            
            perform_measurement();
            update_gui_display();
            measurement_data.last_measurement_ms = current_time;
            
            printf("[Auto] Temp=%.2f C, Press=%.0f Pa, Hum=%.1f %%\n", 
                   measurement_data.temperature, 
                   measurement_data.pressure, 
                   measurement_data.humidity);
        }
    }
}