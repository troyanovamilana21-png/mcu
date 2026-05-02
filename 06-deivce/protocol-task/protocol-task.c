#include "protocol-task.h"
#include "stdio.h"
#include "stdlib.h"
#include "pico/stdlib.h"
#include "string.h"
#include "stdint.h"

static api_t* api = {0};
static int commands_count = 0;

void protocol_task_init(api_t* device_api)
{
    // Сохраняем указатель на массив команд
    api = device_api;
    
    // Подсчитываем количество команд в массиве
    // Ищем конец массива по NULL-терминатору в имени команды
    commands_count = 0;
    if (api != NULL)
    {
        while (api[commands_count].command_name != NULL)
        {
            commands_count++;
        }
    }
}

void protocol_task_handle(char* command_string)
{
    // Проверка на то, что command_string не равно NULL
    if (!command_string)
    {
        // Строка команды еще не получена - выходим
        return;
    }

    // Логика обработки полученной строки. Делим ее на команду и аргументы:
    const char* command_name = command_string;
    const char* command_args = NULL;

    char* space_symbol = strchr(command_string, ' ');

    if (space_symbol)
    {
        *space_symbol = '\0';
        command_args = space_symbol + 1;
    }
    else
    {
        command_args = "";
    }

    // Вывод найденных имени команды и ее аргументов
    printf("Получена команда: '%s'\n", command_name);
    printf("Аргументы: '%s'\n", command_args);

    // В цикле проходим по массиву команд api и ищем совпадение имени команды
    for (int i = 0; i < commands_count; i++)
    {
        
        // Определяем совпадает ли команда с именем команды в массиве api
        if (strcmp(command_name, api[i].command_name) == 0)
        {
            // Мы нашли команду, вызываем callback найденной команды
            api[i].command_callback(command_args);
            return;
        }
    }
    printf("%d", 100);
    // Выводим ошибку, если команда не была найдена в списке команд
    printf("Ошибка: неизвестная команда '%s'\n", command_name);
    return;
}

void mem_prot(uint32_t addr){
    uint32_t value = *(uint32_t*)addr;
    printf("Value at 0x%08X: 0x%08X\n", addr, value);
}


void wmem_prot(uint32_t addr, uint32_t value){
    *(uint32_t*)addr = value;
    printf("Written 0x%08X to address 0x%08X\n", value, addr);
}