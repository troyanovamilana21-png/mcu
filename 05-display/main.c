#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include "ili9341-driver.h"
#include "hardware/spi.h"
#include "ili9341-display.h"
#include "ili9341-font.h"
#include <stdbool.h>


#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

#define ILI9341_PIN_MISO 4
#define ILI9341_PIN_CS 10
#define ILI9341_PIN_SCK 6
#define ILI9341_PIN_MOSI 7
#define ILI9341_PIN_DC 8
#define ILI9341_PIN_RESET 9
// #define PIN_LED -> 3.3V

static ili9341_display_t ili9341_display = {0};

void rp2040_sleep(uint32_t ms){
    sleep_ms(ms);
}
void rp2040_spi_write(const uint8_t *data, uint32_t size)
{
	spi_write_blocking(spi0, data, size);
}

void rp2040_spi_read(uint8_t *buffer, uint32_t length)
{
	spi_read_blocking(spi0, 0, buffer, length);
}

void rp2040_gpio_cs_write(bool level)
{
	gpio_put(ILI9341_PIN_CS, level);
}

void rp2040_gpio_dc_write(bool level)
{
	gpio_put(ILI9341_PIN_DC, level);
}

void rp2040_gpio_reset_write(bool level)
{
	gpio_put(ILI9341_PIN_RESET, level);
}

void help_callback(const char* args);

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

void led_blink_set_period_ms_callback(const char* args){
    uint period_ms = 0;
    sscanf(args, "%u", &period_ms);
    if (period_ms ==0){
        printf("incorrect delay for blink\n");
        return;
    }
    else{
        led_task_set_blink_period_ms(period_ms);
    }
}

void mem_callback(const char* args){
    uint32_t addr;
    if (sscanf(args, "%x", &addr) != 1) {
        printf("Error: invalid address format.\n");
        return;
    }
    mem_prot(addr);
}

void wmem_callback(const char* args){
    uint32_t addr, value;
    if (sscanf(args, "%x %x", &addr, &value) != 2) {
        printf("Error: invalid arguments.\n");
        return;
    }
    wmem_prot(addr, value);
}

void disp_screen_callback(const char* args)
{
	uint32_t c = 0;
	int result = sscanf(args, "%x", &c);
	
	uint16_t color = COLOR_BLACK;
	
	if (result == 1)
	{
		color = RGB888_2_RGB565(c);
	}
	
	ili9341_fill_screen(&ili9341_display, color);
}

void disp_px_callback(const char* args)
{
    uint32_t x = 0, y = 0, c = 0;
    
    int result = sscanf(args, "%u %u %x", &x, &y, &c);
    
    if (result < 3)
    {
        printf("Error: disp_px requires 3 arguments: x y color (hex)\n");
        printf("Example: disp_px 100 150 FF0000\n");
        return;
    }
    
    if (x >= ili9341_display.width || y >= ili9341_display.height)
    {
        printf("Error: coordinates out of bounds (max %ux%u)\n", 
               ili9341_display.width, ili9341_display.height);
        return;
    }
    
    uint16_t color = RGB888_2_RGB565(c);
    ili9341_draw_pixel(&ili9341_display, (uint16_t)x, (uint16_t)y, color);
    
    printf("Pixel drawn at (%u, %u) with color 0x%06X\n", x, y, c);
}


void disp_line_callback(const char* args)
{
    uint32_t c;
    int x1, y1;
    int x2, y2;
    int result = sscanf(args, "%d %d %d %d %x", &x1, &y1, &x2, &y2, &c);
    if (result != 5) {
        printf("Error: disp_px requires 5 arguments: <x> <y> <x> <y> <hex_color>\n");
        return;
    }
    
    
    uint16_t color = COLOR_BLACK;
    color = RGB888_2_RGB565(c);
    
    ili9341_draw_line(&ili9341_display, (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, color);
}

void disp_rect_callback(const char* args)
{
    uint32_t c;
    int x1, y1;
    int x2, y2;
    int result = sscanf(args, "%d %d %d %d %x", &x1, &y1, &x2, &y2, &c);
    if (result != 5) {
        printf("Error: disp_px requires 5 arguments: <x> <y> <x> <y> <hex_color>\n");
        return;
    }
    
    
    uint16_t color = COLOR_BLACK;
    color = RGB888_2_RGB565(c);
    
    ili9341_draw_rect(&ili9341_display, (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, color);
}

void disp_frect_callback(const char* args)
{
    uint32_t c;
    int x1, y1;
    int x2, y2;
    int result = sscanf(args, "%d %d %d %d %x", &x1, &y1, &x2, &y2, &c);
    if (result != 5) {
        printf("Error: disp_px requires 5 arguments: <x> <y> <x> <y> <hex_color>\n");
        return;
    }
    
    
    uint16_t color = COLOR_BLACK;
    color = RGB888_2_RGB565(c);
    
    ili9341_draw_filled_rect(&ili9341_display, (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, color);
}

void disp_text_callback(const char* args) {
    int x, y;
    char text[64];  
    uint32_t color, bg_color;
    // Пример: disp_text 20 100 "Hello" 0xFF0000 0x000000
        int result = sscanf(args, "%d %d \"%127[^\"]\" %x %x", &x, &y, text, &color, &bg_color);
    
    if (result == 5) {
        
        uint16_t color_565 = RGB888_2_RGB565(color);
        uint16_t bg_565 = RGB888_2_RGB565(bg_color);
        
        ili9341_draw_text(&ili9341_display, x, y, text, &jetbrains_font, color_565, bg_565);
    } else {
        printf("Error: format: <x> <y> \"<text>\" <color> <bg_color>\n");
    }
}

api_t device_api[] =
{
 {"version", version_callback, "get device name and firmware version"},
 {"on", led_on_callback, "turns led on"},
 {"off", led_off_callback, "turns led off"},
 {"blink", led_blink_callback, "turns led in blink state"},
 {"set_period", led_blink_set_period_ms_callback, "set delay time for blink"},
 {"command_help", help_callback, "show all commands"},
 {"mem", mem_callback, "read from address"},
 {"wmem", wmem_callback, "write to address"},
 {"disp_screen", disp_screen_callback, "filled screen color in 0xRRGGBB format"},
 {"disp_px", disp_px_callback, "set pixel color in 0xRRGGBB format"},
 {"disp_line", disp_line_callback, "line"},
 {"disp_rect", disp_rect_callback, "rectangle"},
 {"disp_frect", disp_frect_callback, "filled rectangle"},
 {"disp_text", disp_text_callback, "filled rectangle"},
 {NULL, NULL, NULL}
};

void help_callback(const char* args) {
    printf("Available commands:\n");
    for (int i = 0; device_api[i].command_name != NULL; i++) {
        printf("  %s: %s\n", device_api[i].command_name, device_api[i].command_help);
    }
}

int main(){
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
    /* 2. Coloured rectangles */
    ili9341_draw_filled_rect(&ili9341_display, 10, 10, 100, 60, COLOR_RED);
    ili9341_draw_filled_rect(&ili9341_display, 120, 10, 100, 60, COLOR_GREEN);
    ili9341_draw_filled_rect(&ili9341_display, 230, 10, 80, 60, COLOR_BLUE);
    /* 3. Hollow rectangle outline */
    ili9341_draw_rect(&ili9341_display, 10, 90, 300, 80, COLOR_WHITE);
    /* 4. Diagonal lines */
    ili9341_draw_line(&ili9341_display, 0, 0, 319, 239, COLOR_YELLOW);
    ili9341_draw_line(&ili9341_display, 319, 0, 0, 239, COLOR_CYAN);
    ili9341_draw_text(&ili9341_display, 20, 100, "Hello, ILI9341!", &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
    ili9341_draw_text(&ili9341_display, 20, 116, "RP2040 / Pico SDK", &jetbrains_font, COLOR_YELLOW, COLOR_BLACK);
    while (1)
    {
        led_task_handle();
        char* command = stdio_task_handle();
        
        if (command != NULL)
        {                   
            protocol_task_handle(command);

        }
    }
}
