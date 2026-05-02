#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

// Макросы с именем устройства и версией прошивки
#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

// Глобальная переменная и глобальная постоянная
uint32_t global_variable = 0;
const uint32_t constant_variable = 42;

int main() {
    // Инициализация системы ввода/вывода stdio
    stdio_init_all();
    
    // Небольшая задержка для инициализации USB
    sleep_ms(2000);
    
    // Бесконечный цикл
    while (true) {
        // 1. Вывод простой строковой константы
        printf("Hello World!\n");
        
        // 2. Вывод строки с аргументами (макросы)
        printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
        
        // 3. Счетчик микросекунд
        uint64_t timestamp = time_us_64();
        printf("system timestamp: %llu us\n", timestamp);
        
        // 4. Переменная на стеке и ее адрес в разных форматах
        uint32_t stack_variable = 8888;
        printf("stack variable | addr = 0x%X | value = %u\n", (uint32_t)&stack_variable, *(&stack_variable));
        printf("stack variable | addr = 0x%X | value = %X\n", (uint32_t)&stack_variable, *(&stack_variable));
        printf("stack variable | addr = 0x%X | value = 0x%X\n", (uint32_t)&stack_variable, *(&stack_variable));
        
        // 5. Инкремент глобальной переменной и вывод
        global_variable++;
        printf("global variable | addr = 0x%X | value = %u\n", (uint32_t)&global_variable, *(&global_variable));
        
        // 6. Переменная на куче (с ошибкой - нет free)
        uint32_t* heap_variable = (uint32_t*)malloc(sizeof(uint32_t));
        *heap_variable = 5555;
        printf("heap variable | addr = 0x%X | value = %u\n", (uint32_t)heap_variable, *heap_variable);
        
        // 7. Вывод адреса и значения постоянной
        printf("constant variable | addr = 0x%X | value = %u\n", (uint32_t)&constant_variable, *(&constant_variable));
        
        // 8. Вывод адреса и значения строковой постоянной
        printf("constant string | addr = 0x%X | value = 0x%X, [%s]\n", 
               (uint32_t)DEVICE_NAME, 
               *((uint32_t*)DEVICE_NAME), 
               DEVICE_NAME);
        
        // 9. Регистр RP2040 (регистр идентификатора чипа)
        printf("reg chip id | addr = 0x%X | value = 0x%X\n", 
               0x40000000, 
               *((uint32_t*)0x40000000));
        
        // 10. Глобальная переменная по прямому адресу
        // ВНИМАНИЕ: Адрес 0x20002278 может отличаться!
        // Лучше использовать фактический адрес global_variable
        printf("var by addr | addr = 0x%X | value = %u\n", 
               (uint32_t)&global_variable, 
               *((uint32_t*)(uint32_t)&global_variable));
        
        // 11. Адрес функции main и первые команды
        printf("main function | addr = 0x%X | value = 0x%X\n", 
               (uint32_t)main, 
               *((uint32_t*)main));
        
        // Задержка 1 секунда
        sleep_ms(1000);
        
        // Разделитель для читаемости вывода
        printf("----------------------------------------\n");
    }
    
    return 0;
}
