#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task.h"
#include "led-task/led-task.h"
#include "hardware/i2c.h"
#include "bme280-driver.h"

#define BME280_I2C_ADDR 0x76
#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

void rp2040_i2c_read(uint8_t* buffer, uint16_t length)
{
    i2c_read_timeout_us(i2c1, 0x76, buffer, length, false, 100000);
}

void rp2040_i2c_write(uint8_t* data, uint16_t size)
{
    i2c_write_timeout_us(i2c1, 0x76, data, size, false, 100000);
}

void check_i2c_devices(void)
{
    printf("Scanning I2C bus...\n");
    
    uint8_t addresses[] = {0x76, 0x77};  // Возможные адреса BME280
    
    for (int i = 0; i < 2; i++) {
        uint8_t txbuf = 0;
        int result = i2c_write_timeout_us(i2c1, addresses[i], &txbuf, 0, false, 10000);
        
        if (result >= 0) {
            printf("✓ Device found at address 0x%02X\n", addresses[i]);
        } else {
            printf("✗ No device at address 0x%02X\n", addresses[i]);
        }
    }
}

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

void read_reg_callback(const char* args)
{
    unsigned int addr, count;  // используем unsigned int, чтобы вместить 0xFF

    // Пытаемся прочитать два шестнадцатеричных числа
    if (sscanf(args, "%x %x", &addr, &count) != 2) {
        printf("Usage: read_reg <addr_hex> <count_hex>\n");
        printf("Example: read_reg D0 3  (читает 3 регистра с адреса 0xD0)\n");
        return;
    }

    // Проверка допустимых значений
    if (addr > 0xFF || count == 0 || count > 0xFF || addr + count > 0x100) {
        printf("Error: addr (0x%X) and count (0x%X) must satisfy:\n", addr, count);
        printf("  addr ≤ 0xFF, count between 1 and 0xFF, addr+count ≤ 0x100\n");
        return;
    }

    uint8_t buffer[256] = {0};
    bme280_read_regs((uint8_t)addr, buffer, (uint8_t)count);

    for (int i = 0; i < (int)count; i++) {
        printf("bme280 register [0x%X] = 0x%X\n", addr + i, buffer[i]);
    }
}

void write_reg_callback(const char* args)
{
    unsigned int addr, value;  // используем unsigned int, чтобы вместить 0xFF

    // Пытаемся прочитать два шестнадцатеричных числа
    if (sscanf(args, "%x %x", &addr, &value) != 2) {
        printf("HUH\n");
        return;
    }

    // Проверка допустимых значений
    if (addr > 0xFF || value == 0 || value > 0xFF) {
        printf("Error: addr (0x%X) and value (0x%X) must satisfy:\n", addr, value);
        printf("  addr ≤ 0xFF, value between 1 and 0xFF, addr+value ≤ 0x100\n");
        return;
    }

    bme280_write_reg((uint8_t)addr, (uint8_t)value);
}
void temp_raw_callback(const char* args){
    uint16_t counts = bme280_read_temp_raw();
    printf("temp counts %d \n", counts);
}
void pres_raw_callback(const char* args){
    uint16_t counts = bme280_read_press_raw();
    printf("press counts %d \n", counts);
}
void hum_raw_callback(const char* args){
    uint16_t counts = bme280_read_hum_raw();
    printf("hum counts %d \n", counts);
}
void temp_callback(const char* args) {
    float C = bme280_read_temp_celsius();
    printf("%f", C);
}

void pres_callback(const char* args) {
    float P = bme280_read_press_pascal();
    printf("%f", P);
}

void hum_callback(const char* args) {
    float H = bme280_read_hum_percent();
    printf("%f", H);
}
api_t device_api[] =
{
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
 {NULL, NULL, NULL}
};

int main(){
    i2c_init(i2c1, 100000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    sleep_ms(100);
    stdio_task_init();
    stdio_init_all();
    check_i2c_devices();
    protocol_task_init(device_api);
    led_task_init();
    bme280_init(rp2040_i2c_read, rp2040_i2c_write);
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
